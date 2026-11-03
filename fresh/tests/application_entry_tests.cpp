#include "pal4/application_entry.h"
#if defined(_WIN32) && UINTPTR_MAX == UINT32_MAX
#include "pal4/win32_startup_threads.h"
#endif

#include <algorithm>
#include <atomic>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IApplicationRuntime {
public:
    pal4::GameRenderState* game = nullptr;
    pal4::WindowMessageConfig* config = nullptr;
    std::string_view expected_window_class = "rwc";
    std::string_view expected_window_title = "PAL4-Application";
    std::uint32_t* thread_count = nullptr;
    std::uint32_t* parameter = nullptr;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> random_values{1u, 11u, 23u, 5u};
    std::vector<std::uint32_t> seeds;
    std::vector<std::uint32_t> sleeps;
    std::vector<std::pair<bool, pal4::ApplicationMessage>> messages;
    std::size_t random_index = 0u;
    std::size_t message_index = 0u;
    std::uint32_t allocated_bytes = 0u;
    std::uint32_t workers = 0u;
    std::uint32_t dispatched = 0u;
    std::uint32_t frame_updates = 0u;
    std::uint32_t frame_callback = 0x5c7f30u;
    std::vector<std::uint32_t> frame_callbacks;
    std::string logged_message;
    float center_x = 0.0f;
    float center_y = 0.0f;
    bool existing_window = false;
    bool config_success = true;
    bool window_success = true;
    bool activate_window = true;
#if defined(_WIN32) && UINTPTR_MAX == UINT32_MAX
    pal4::Win32StartupThreads* native_threads = nullptr;
#endif

    void SetExceptionLogPath(std::string_view path) override {
        Check(path == "pal4Exception.log", "exception log path");
        calls.emplace_back("exception-log");
    }
    void SetExceptionHandlerOptions(std::uint32_t a, std::uint32_t b,
                                 std::uint32_t c, std::uint32_t d,
                                 std::uint32_t e) override {
        Check(a == 1u && b == 1u && c == 1u && d == 0u && e == 15u,
              "exception handler option arguments");
        calls.emplace_back("exception-options");
    }
    std::uint32_t InitializeLogger() override {
        calls.emplace_back("logger");
        return 0x123u;
    }
    void SetLogFilePath(std::uint32_t logger,
                        std::string_view path) override {
        Check(logger == 0x123u && path == "pal4Logfile.log",
              "logger path");
        calls.emplace_back("log-path");
    }
    std::uint32_t FindWindow(std::string_view class_name,
                             std::string_view title) override {
        Check(class_name == expected_window_class &&
                  title == expected_window_title,
              "window identifiers");
        calls.emplace_back("find-window");
        return existing_window ? 0x777u : 0u;
    }
    bool IsWindow(std::uint32_t window) override {
        Check(window == (existing_window ? 0x777u : 0u),
              "existing window handle");
        calls.emplace_back("is-window");
        return existing_window;
    }
    void ShowWindow(std::uint32_t window, std::int32_t command) override {
        Check((window == 0x777u && command == 1) ||
                  (window == 0xabcu && command == 5),
              "show window arguments");
        calls.emplace_back("show");
    }
    std::string FormatStartupMessage() override {
        calls.emplace_back("format");
        return "done old word\n";
    }
    std::uint32_t Time() override {
        calls.emplace_back("time");
        return 100u;
    }
    void SeedRandom(std::uint32_t seed) override {
        calls.emplace_back("seed");
        seeds.push_back(seed);
    }
    std::uint32_t Random() override {
        calls.emplace_back("random");
        Check(random_index < random_values.size(), "random call count");
        return random_values[random_index++];
    }
    void CreateCounterThread() override {
        calls.emplace_back("counter-thread");
        ++workers;
#if defined(_WIN32) && UINTPTR_MAX == UINT32_MAX
        if (native_threads)
            Check(native_threads->CreateCounterThread(0u),
                  "native counter thread starts");
#endif
    }
    void CreateRandomizedThread(std::uint32_t& value) override {
        calls.emplace_back("random-thread");
        parameter = &value;
#if defined(_WIN32) && UINTPTR_MAX == UINT32_MAX
        if (native_threads) {
            Check(native_threads->CreateRandomizedThread(value, 0u),
                  "native randomized thread starts");
            return;
        }
#endif
        *thread_count = 1u;
    }
    void StopStartupThreads() override {
        Check(parameter != nullptr, "startup parameter survives worker shutdown");
#if defined(_WIN32) && UINTPTR_MAX == UINT32_MAX
        if (native_threads)
            Check(native_threads->StopAndJoin(),
                  "native startup workers finish before parameter expires");
#endif
        calls.emplace_back("stop-threads");
        parameter = nullptr;
    }
    void Sleep(std::uint32_t milliseconds) override {
        calls.emplace_back("sleep");
        sleeps.push_back(milliseconds);
        if (game->ReadShared(0x28u) != 0u && parameter != nullptr)
            std::atomic_ref(*parameter).store(1u, std::memory_order_release);
#if defined(_WIN32) && UINTPTR_MAX == UINT32_MAX
        if (native_threads) std::this_thread::yield();
#endif
    }
    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        calls.emplace_back("allocate");
        allocated_bytes = bytes;
        return 0x100000u;
    }
    void BindAllocatedGlobals(std::uint32_t config_address,
                              std::uint32_t game_address) override {
        Check(config_address == 0x10000bu && game_address == 0x1000beu,
              "randomized global addresses reach runtime binding");
        calls.emplace_back("bind-globals");
    }
    bool InitializeGameConfig(std::uint32_t instance) override {
        Check(instance == 0x111u, "config instance");
        calls.emplace_back("config");
        (*config)[0] = 800u;
        (*config)[1] = 600u;
        game->Write(8u, 1u);
        return config_success;
    }
    std::uint32_t ScanDecimal(std::string_view text) override {
        calls.emplace_back("scan");
        std::uint32_t value = 0u;
        const auto result = std::from_chars(text.data(), text.data()+text.size(),
                                            value);
        Check(result.ec == std::errc{}, "decimal command argument");
        return value;
    }
    std::uint32_t GetRenderSystem() override {
        calls.emplace_back("render-system");
        return 0x321u;
    }
    std::uint32_t LoadIcon(std::uint32_t instance,
                           std::uint32_t resource) override {
        Check(instance == 0x111u && resource == 0x66u, "icon resource");
        calls.emplace_back("icon");
        return 0x456u;
    }
    bool CreateGameWindow(std::uint32_t render_system,
                          std::string_view class_name,
                          std::string_view title, bool windowed,
                          std::uint32_t icon) override {
        Check(render_system == 0x321u &&
                  class_name == expected_window_class &&
                  title == expected_window_title && icon == 0x456u,
              "window creation arguments");
        Check(windowed == (game->Read(8u) == 0u), "windowed mode flag");
        calls.emplace_back("create-window");
        if (window_success) {
            game->Write(0u, 0xabcu);
            game->Write(0x20u, activate_window ? 1u : 0u);
            game->Write(0x24u, 1u);
        }
        return window_success;
    }
    void InitializeGameSystem() override { calls.emplace_back("game-init"); }
    void LogStartupMessage(std::uint32_t logger,
                           std::string_view message) override {
        Check(logger == 0x123u, "startup logger");
        calls.emplace_back("startup-log");
        logged_message = message;
    }
    void UpdateWindow(std::uint32_t window) override {
        Check(window == 0xabcu, "updated window");
        calls.emplace_back("update-window");
    }
    void SetViewportCenter(float x, float y) override {
        calls.emplace_back("center");
        center_x = x;
        center_y = y;
    }
    bool PeekMessage(pal4::ApplicationMessage& message,
                     std::uint32_t flags) override {
        Check(flags == 3u && message_index < messages.size(),
              "peek message flags and sequence");
        calls.emplace_back("peek");
        const auto& next = messages[message_index++];
        message = next.second;
        return next.first;
    }
    void TranslateMessage(const pal4::ApplicationMessage&) override {
        calls.emplace_back("translate");
    }
    void DispatchMessage(const pal4::ApplicationMessage&) override {
        calls.emplace_back("dispatch");
        ++dispatched;
    }
    std::uint32_t GetFrameCallback() override {
        calls.emplace_back("frame-callback");
        return frame_callback;
    }
    void UpdateGameFrame(std::uint32_t callback) override {
        Check(callback == frame_callback, "frame callback forwarding");
        calls.emplace_back("frame");
        frame_callbacks.push_back(callback);
        ++frame_updates;
        if (frame_updates == 1u) frame_callback = 0x53df00u;
    }
    void WaitMessage() override {
        calls.emplace_back("wait-message");
        (*config)[3] = 1u;
    }
    void CleanupGame() override { calls.emplace_back("cleanup-game"); }
    void DestroyGameWindow(std::uint32_t render_system) override {
        Check(render_system == 0x321u, "game window render system");
        calls.emplace_back("destroy-window");
    }
    void ReleaseAllocatorRegions() override {
        calls.emplace_back("release-allocator-regions");
    }
};

struct Fixture {
    pal4::GameRenderState game{0x800000u};
    pal4::WindowMessageConfig config{};
    std::uint8_t initialized = 9u;
    std::uint32_t thread_count = 99u;
    pal4::ApplicationEntry entry{game, config, initialized, thread_count};
    Runtime runtime;
    Fixture() {
        runtime.game = &game;
        runtime.config = &config;
        runtime.thread_count = &thread_count;
    }
    std::uint32_t Run(std::string_view command = {}) {
        return entry.Run(0x111u, command, 5, runtime);
    }
};

void TestExistingWindow() {
    Fixture f;
    f.runtime.existing_window = true;
    Check(f.Run() == 0u && f.runtime.calls ==
              std::vector<std::string>{"exception-log", "exception-options",
                                       "logger", "log-path", "find-window",
                                       "is-window", "show"},
          "existing instance is shown before startup work");
    Check(f.initialized == 9u && f.thread_count == 99u,
          "existing instance preserves thread globals");
}

void TestCustomWindowIdentity() {
    Fixture f;
    f.runtime.expected_window_class = "rwc_rebuilt";
    f.runtime.expected_window_title = "PAL4-Rebuilt";
    f.runtime.messages = {{true, {0x12u, 0u, 0u}}};
    Check(f.entry.Run(0x111u, "width:800 height:600 win", 5,
                      f.runtime,
                      {f.runtime.expected_window_class,
                       f.runtime.expected_window_title}) == 0u &&
              std::find(f.runtime.calls.begin(), f.runtime.calls.end(),
                        "find-window") != f.runtime.calls.end() &&
              std::find(f.runtime.calls.begin(), f.runtime.calls.end(),
                        "create-window") != f.runtime.calls.end(),
          "custom window identity is used for lookup and creation");
}

void TestConfigAndWindowFailures() {
    Fixture f;
    f.runtime.config_success = false;
    Check(f.Run() == 0u && f.runtime.workers == 2u &&
              f.runtime.allocated_bytes == 1638u &&
              f.entry.ConfigAddress() == 0x10000bu &&
              f.entry.GameAddress() == 0x1000beu &&
              f.initialized == 1u && f.thread_count == 1u &&
              f.runtime.seeds ==
                  std::vector<std::uint32_t>{100u, 100u, 111u, 102u},
          "startup thread and allocation setup precedes config gate");
    Check(f.runtime.calls[f.runtime.calls.size() - 2] == "config" &&
              f.runtime.calls.back() == "stop-threads",
          "config failure joins workers before returning");
    const auto binding = std::find(f.runtime.calls.begin(),
                                   f.runtime.calls.end(), "bind-globals");
    const auto allocation = std::find(f.runtime.calls.begin(),
                                      f.runtime.calls.end(), "allocate");
    const auto configuration = std::find(f.runtime.calls.begin(),
                                         f.runtime.calls.end(), "config");
    Check(allocation != f.runtime.calls.end() &&
              binding != f.runtime.calls.end() &&
              configuration != f.runtime.calls.end() &&
              allocation + 1 == binding && binding < configuration,
          "global storage binds before configuration writes");

    Fixture other;
    other.runtime.window_success = false;
    Check(other.Run("width:1024 height:768 win") == 0u &&
              other.config[0] == 1024u && other.config[1] == 768u &&
              other.game.Read(8u) == 0u && other.game.Read(0x590u) == 1u,
          "command-line dimensions and windowed override");
    Check(other.runtime.calls[other.runtime.calls.size() - 2] ==
              "create-window" &&
              other.runtime.calls.back() == "stop-threads",
          "window creation failure joins workers before system init");
}

void TestMessageLoopAndShutdown() {
    Fixture f;
    f.runtime.messages = {{true, {0x100u, 1u, 2u}},
                          {false, {}}, {true, {0x12u, 0u, 0u}}};
    Check(f.Run() == 0u && f.runtime.dispatched == 1u &&
              f.runtime.frame_updates == 1u &&
              f.game.Read(0x28u) == 6u &&
              f.runtime.sleeps == std::vector<std::uint32_t>{1000u},
          "message dispatch and first-frame thread gate");
    Check(f.runtime.center_x == 400.0f && f.runtime.center_y == 300.0f,
          "viewport center follows config dimensions");
    Check(f.runtime.logged_message == "done ol \tword\n",
          "startup log replaces final space and preceding byte");
    Check(f.runtime.calls[f.runtime.calls.size()-5] == "stop-threads" &&
              f.runtime.calls[f.runtime.calls.size()-4] == "cleanup-game" &&
              f.runtime.calls[f.runtime.calls.size()-3] == "render-system" &&
              f.runtime.calls[f.runtime.calls.size()-2] == "destroy-window" &&
              f.runtime.calls.back() == "release-allocator-regions",
          "shutdown order after quit message");
    const auto frame = std::find(f.runtime.calls.begin(), f.runtime.calls.end(),
                                 "frame");
    Check(frame != f.runtime.calls.begin() &&
              frame != f.runtime.calls.end() &&
              *(frame - 1) == "frame-callback",
          "frame update reads current callback first");
}

void TestInactiveWait() {
    Fixture f;
    f.runtime.activate_window = false;
    f.runtime.messages = {{false, {}}};
    Check(f.Run() == 0u && f.config[3] == 1u &&
              f.runtime.frame_updates == 0u &&
              f.runtime.sleeps.empty(),
          "inactive loop waits for a message until exit flag changes");
}

void TestFrameCallbackIsReadEachFrame() {
    Fixture f;
    f.runtime.messages = {{false, {}}, {false, {}},
                          {true, {0x12u, 0u, 0u}}};
    Check(f.Run() == 0u &&
              f.runtime.frame_callbacks ==
                  std::vector<std::uint32_t>{0x5c7f30u, 0x53df00u},
          "each frame reads the current game callback");
}

#if defined(_WIN32) && UINTPTR_MAX == UINT32_MAX
void TestNativeStartupThreadDelegation() {
    Fixture f;
    pal4::Win32StartupThreads threads{f.game, f.initialized, f.thread_count};
    f.runtime.native_threads = &threads;
    f.runtime.messages = {{false, {}}, {true, {0x12u, 0u, 0u}}};
    Check(f.Run() == 0u && f.runtime.workers == 2u &&
              f.runtime.frame_updates == 1u &&
              f.game.ReadShared(0x28u) != 0u &&
              f.runtime.parameter == nullptr &&
              f.runtime.calls[f.runtime.calls.size() - 5] == "stop-threads",
          "WinMain model delegates CreateThread and joins before shutdown");
}
#endif

}  // namespace

int main() {
    TestExistingWindow();
    TestCustomWindowIdentity();
    TestConfigAndWindowFailures();
    TestMessageLoopAndShutdown();
    TestInactiveWait();
    TestFrameCallbackIsReadEachFrame();
#if defined(_WIN32) && UINTPTR_MAX == UINT32_MAX
    TestNativeStartupThreadDelegation();
#endif
    return EXIT_SUCCESS;
}
