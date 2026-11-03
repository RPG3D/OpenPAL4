#include "pal4/application_entry.h"

#include <atomic>
#include <utility>

namespace pal4 {
namespace {

constexpr std::string_view kExceptionLogPath = "pal4Exception.log";
constexpr std::string_view kLogFilePath = "pal4Logfile.log";

}  // namespace

void ApplicationEntry::ApplyCommandLine(
    const std::string_view command_line, IApplicationRuntime& runtime) {
    for (const auto [keyword, config_index] :
         {std::pair{"width:", 0u}, std::pair{"height:", 1u}}) {
        const auto found = command_line.find(keyword);
        if (found == std::string_view::npos) continue;
        config_[config_index] = runtime.ScanDecimal(
            command_line.substr(found + std::string_view(keyword).size()));
        game_.Write(0x590u, 1u);
        game_.Write(8u, 1u);
    }
    if (command_line.find("win") != std::string_view::npos) {
        game_.Write(8u, 0u);
        game_.Write(0x590u, 1u);
    }
}

std::uint32_t ApplicationEntry::Run(
    const std::uint32_t instance, const std::string_view command_line,
    const std::int32_t show_command, IApplicationRuntime& runtime,
    const ApplicationWindowIdentity identity) {
    runtime.SetExceptionLogPath(kExceptionLogPath);
    runtime.SetExceptionHandlerOptions(1u, 1u, 1u, 0u, 15u);
    runtime.SetLogFilePath(runtime.InitializeLogger(), kLogFilePath);

    const auto existing = runtime.FindWindow(identity.class_name,
                                             identity.title);
    if (runtime.IsWindow(existing)) {
        runtime.ShowWindow(existing, 1);
        return 0u;
    }

    auto startup_message = runtime.FormatStartupMessage();
    runtime.SeedRandom(runtime.Time());
    auto thread_count = std::atomic_ref(thread_count_);
    auto threads_initialized = std::atomic_ref(threads_initialized_);
    thread_count.store(runtime.Random() % 8u + 1u, std::memory_order_relaxed);
    threads_initialized.store(0u, std::memory_order_release);
    do {
        runtime.CreateCounterThread();
        thread_count.fetch_sub(1u, std::memory_order_relaxed);
    } while (thread_count.load(std::memory_order_relaxed) != 0u);
    std::uint32_t parameter = 0u;
    auto shared_parameter = std::atomic_ref(parameter);
    runtime.CreateRandomizedThread(parameter);
    while (thread_count.load(std::memory_order_relaxed) == 0u)
        runtime.Sleep(1000u);

    const auto space = startup_message.find_last_of(' ');
    if (space != std::string::npos && space > 0u) {
        startup_message[space] = '\t';
        startup_message[space - 1u] = ' ';
    }
    runtime.SeedRandom(runtime.Time());
    const auto config_offset = runtime.Random() % 2048u;
    runtime.SeedRandom(config_offset + runtime.Time());
    const auto game_offset = runtime.Random() % 2048u;
    const auto allocation = runtime.AllocateMemory(
        config_offset + game_offset + 1604u);
    config_address_ = allocation + config_offset;
    game_address_ = config_address_ + game_offset + 156u;
    runtime.BindAllocatedGlobals(config_address_, game_address_);
    shared_parameter.store(0u, std::memory_order_release);
    threads_initialized.store(1u, std::memory_order_release);
    runtime.SeedRandom(runtime.Time() +
                       thread_count.load(std::memory_order_relaxed) + 1u);
    auto first_frame_count =
        thread_count.load(std::memory_order_relaxed) + runtime.Random();
    if (first_frame_count == 0u) first_frame_count = 1u;

    if (!runtime.InitializeGameConfig(instance)) {
        runtime.StopStartupThreads();
        return 0u;
    }
    ApplyCommandLine(command_line, runtime);
    const auto render_system = runtime.GetRenderSystem();
    const auto icon = runtime.LoadIcon(instance, 0x66u);
    if (!runtime.CreateGameWindow(render_system, identity.class_name,
                                  identity.title,
                                  game_.Read(8u) == 0u, icon)) {
        runtime.StopStartupThreads();
        return 0u;
    }

    runtime.InitializeGameSystem();
    runtime.LogStartupMessage(runtime.InitializeLogger(), startup_message);
    const auto window = game_.Read(0u);
    runtime.ShowWindow(window, show_command);
    runtime.UpdateWindow(window);
    runtime.SetViewportCenter(
        static_cast<float>(static_cast<std::int32_t>(config_[0])) * 0.5f,
        static_cast<float>(static_cast<std::int32_t>(config_[1])) * 0.5f);

    while ((config_[3] & 0xffu) == 0u) {
        ApplicationMessage message;
        if (runtime.PeekMessage(message, 3u)) {
            if (message.message == 0x12u) break;
            runtime.TranslateMessage(message);
            runtime.DispatchMessage(message);
        } else if (game_.Read(0x20u) != 0u) {
            if (game_.Read(0x24u) != 0u) {
                runtime.UpdateGameFrame(runtime.GetFrameCallback());
                if (game_.ReadShared(0x28u) == 0u) {
                    game_.WriteShared(0x28u, first_frame_count);
                    while (shared_parameter.load(std::memory_order_acquire) ==
                           0u)
                        runtime.Sleep(1000u);
                }
            }
        } else {
            runtime.WaitMessage();
        }
    }
    runtime.StopStartupThreads();
    runtime.CleanupGame();
    runtime.DestroyGameWindow(runtime.GetRenderSystem());
    runtime.ReleaseAllocatorRegions();
    return 0u;
}

}  // namespace pal4
