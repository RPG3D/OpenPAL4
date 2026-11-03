#include "pal4/native_renderware_plugin_runtime.h"
#include "pal4/rw_plugin_8449_native.h"
#include "pal4/rw_plugin_8450_native.h"
#include "pal4/rw_plugin_8451_native.h"
#include "pal4/rw_plugin_8452_native.h"
#include "pal4/native_renderware_dialog_mode.h"
#include "support/native_test_display.h"

#include <windows.h>
#include <rwcore.h>
#include <rpworld.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <array>
#include <cstring>
#include <exception>
#include <string>
#include <string_view>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(EXIT_FAILURE);
    }
}

struct Logger final : pal4::IGameConfigLogger {
    std::string format;
    std::uint32_t line = 0u;
    std::uint32_t count = 0u;

    std::uint32_t InitializeLogger() override { return 0x123u; }
    void AppendLog(const std::uint32_t logger, const std::string_view text,
                   const std::string_view, const std::uint32_t source_line) override {
        Check(logger == 0x123u, "logger identity");
        format = text;
        line = source_line;
        ++count;
    }
};

struct CountingRuntime final : pal4::IRenderWarePluginRuntime {
    pal4::NativeRenderWarePluginRuntime native;
    std::uint32_t count = 0u;
    pal4::RenderWarePlugin last = pal4::RenderWarePlugin::Count;

    explicit CountingRuntime(Logger& logger) : native(logger) {}

    std::uint32_t Attach(const pal4::RenderWarePlugin plugin) override {
        last = plugin;
        ++count;
        const auto result = native.Attach(plugin);
        Check(result != 0u, "native plugin attach");
        return result;
    }
    std::uint32_t InitializeLogger() override {
        return native.InitializeLogger();
    }
    void AppendLog(const std::uint32_t logger, const std::string_view format,
                   const std::string_view source_file,
                   const std::uint32_t source_line) override {
        native.AppendLog(logger, format, source_file, source_line);
    }
};

}  // namespace

int main() {
    SetUnhandledExceptionFilter([](EXCEPTION_POINTERS* details) -> LONG {
        HMODULE module{};
        char path[MAX_PATH]{};
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           static_cast<LPCSTR>(
                               details->ExceptionRecord->ExceptionAddress),
                           &module);
        GetModuleFileNameA(module, path, MAX_PATH);
        std::fprintf(stderr, "plugin exception %08lX at %p: %s + %zx\n",
                     details->ExceptionRecord->ExceptionCode,
                     details->ExceptionRecord->ExceptionAddress, path,
                     reinterpret_cast<std::size_t>(
                         details->ExceptionRecord->ExceptionAddress) -
                         reinterpret_cast<std::size_t>(module));
        return EXCEPTION_EXECUTE_HANDLER;
    });
    try {
    pal4::NativeRenderWareDialogMode dialog_mode;
    Check(dialog_mode.PrepareBeforeOpen(), "dialog mode hook prepare");
    Check(RwEngineInit(nullptr, 0u, 0x400000u) != 0,
          "RenderWare engine init");
    Logger logger;
    CountingRuntime runtime(logger);
    pal4::RenderWarePluginInitializer initializer;
    Check(initializer.Initialize(runtime) == 1u && runtime.count == 22u &&
              runtime.last == pal4::RenderWarePlugin::GamePlugin8452 &&
              logger.count == 1u && logger.line == 0x152u &&
              logger.format.find("RWCAttachPlugins success") !=
                  std::string::npos,
          "complete game plugin attach order");
    const auto& plugin = pal4::NativeRwPlugin8449Lifecycle::State();
    Check(plugin.engine_offset == RwEngineGetPluginOffset(8449u) &&
              plugin.geometry_offset == RpAtomicGetPluginOffset(8449u) &&
              plugin.engine_offset >= 0 && plugin.geometry_offset > 0,
          "game plugin offsets match native SDK registration");
    const auto& plugin8450 = pal4::NativeRwPlugin8450Lifecycle::State();
    Check(plugin8450.engine_offset == RwEngineGetPluginOffset(8450u) &&
              plugin8450.atomic_offset == RpAtomicGetPluginOffset(8450u) &&
              plugin8450.atomic_offset > 0,
          "8450 offsets match native SDK registration");
    const auto& plugin8451 = pal4::NativeRwPlugin8451Lifecycle::State();
    const auto& plugin8452 = pal4::NativeRwPlugin8452Lifecycle::State();
    Check(plugin8451.engine_offset == RwEngineGetPluginOffset(8451u) &&
              plugin8451.geometry_offset == RpAtomicGetPluginOffset(8451u) &&
              plugin8451.geometry_offset > 0 &&
              plugin8452.engine_offset == RwEngineGetPluginOffset(8452u) &&
              plugin8452.geometry_offset == RpAtomicGetPluginOffset(8452u) &&
              plugin8452.geometry_offset > 0,
          "remaining game offsets match native SDK registration");
    const auto display = pal4::test::TestDisplay();
    const auto window = CreateWindowA("STATIC", "PAL4 plugin boundary",
                                      WS_OVERLAPPEDWINDOW,
                                      display.bounds.left, display.bounds.top, 64, 64,
                                      nullptr, nullptr, GetModuleHandleA(nullptr),
                                      nullptr);
    Check(window != nullptr, "native plugin test window");
    RwEngineOpenParams params{window};
    Check(RwEngineOpen(&params) != 0, "native plugin engine open");
    Check(dialog_mode.EnableBeforeStart(true), "dialog mode enable");
    Check(RwEngineStart() != 0, "native plugin engine start");
    Check(dialog_mode.FinishAfterStart(), "dialog mode startup verification");
    auto* atomic = RpAtomicCreate();
    Check(atomic != nullptr, "native plugin atomic create");
    auto words = [offset = plugin.geometry_offset](const RpAtomic* object) {
        std::array<std::uint32_t, 3> value{};
        std::memcpy(value.data(), reinterpret_cast<const char*>(object) + offset,
                    sizeof(value));
        return value;
    };
    Check(words(atomic) == std::array<std::uint32_t, 3>{0u, 0u, 0u},
          "8449 constructor clears atomic extension");
    std::array<std::uint32_t, 3> plugin8450_words{};
    std::memcpy(plugin8450_words.data(),
                reinterpret_cast<const char*>(atomic) + plugin8450.atomic_offset,
                sizeof(plugin8450_words));
    Check(plugin8450_words == std::array<std::uint32_t, 3>{0u, 0u, 1u},
          "8450 constructor runs alongside 8449");
    std::array<std::uint32_t, 2> plugin8451_words{};
    std::array<std::uint32_t, 3> plugin8452_words{};
    std::memcpy(plugin8451_words.data(),
                reinterpret_cast<const char*>(atomic) + plugin8451.geometry_offset,
                sizeof(plugin8451_words));
    std::memcpy(plugin8452_words.data(),
                reinterpret_cast<const char*>(atomic) + plugin8452.geometry_offset,
                sizeof(plugin8452_words));
    Check(plugin8451_words == std::array<std::uint32_t, 2>{0u, 0u} &&
              plugin8452_words == std::array<std::uint32_t, 3>{0u, 0u, 0u},
          "8451 and 8452 constructors run alongside earlier game plugins");
    const std::array<std::uint32_t, 3> payload{1u, 2u, 3u};
    std::memcpy(reinterpret_cast<char*>(atomic) + plugin.geometry_offset,
                payload.data(), sizeof(payload));
    const std::array<std::uint32_t, 2> hook_payload{4u, 5u};
    const std::array<std::uint32_t, 3> vis_payload{6u, 7u, 8u};
    std::memcpy(reinterpret_cast<char*>(atomic) + plugin8451.geometry_offset,
                hook_payload.data(), sizeof(hook_payload));
    std::memcpy(reinterpret_cast<char*>(atomic) + plugin8452.geometry_offset,
                vis_payload.data(), sizeof(vis_payload));
    auto* clone = RpAtomicClone(atomic);
    Check(clone != nullptr && words(clone) == payload,
          "8449 copy preserves atomic extension");
    std::memcpy(plugin8451_words.data(),
                reinterpret_cast<const char*>(clone) + plugin8451.geometry_offset,
                sizeof(plugin8451_words));
    std::memcpy(plugin8452_words.data(),
                reinterpret_cast<const char*>(clone) + plugin8452.geometry_offset,
                sizeof(plugin8452_words));
    Check(plugin8451_words == hook_payload && plugin8452_words == vis_payload,
          "8451 and 8452 copies preserve atomic extensions");
    Check(RpAtomicDestroy(clone) != 0 &&
              RpAtomicDestroy(atomic) != 0,
          "8449 atomic destruction");
    Check(RwEngineStop() != 0 && RwEngineClose() != 0,
          "native plugin engine shutdown");
    Check(DestroyWindow(window) != 0, "native plugin test window shutdown");
    Check(RwEngineTerm() != 0, "RenderWare engine term");
    std::puts("native plugin boundary passed");
    return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "plugin exception: %s\n", error.what());
        return EXIT_FAILURE;
    }
}
