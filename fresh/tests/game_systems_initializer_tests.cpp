#include "pal4/game_systems_initializer.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

enum class Failure {
    None,
    EngineInit,
    Plugins,
    Keyboard,
    Mouse,
    Open,
    SelectDevice,
    Start,
    CharsetOpen,
    FileSystemManagerOpen,
    FileSystem,
    DirectInput,
};

class Runtime final : public pal4::IGameSystemsRuntime {
public:
    void InitializeAllocator(const std::uint32_t table) override {
        events.emplace_back("allocator");
        Check(table == 0x8e3d98u, "allocator table global address");
    }
    std::uint32_t EngineInit(const std::uint32_t table,
                              const std::uint32_t flags,
                              const std::uint32_t resource_arena_size) override {
        events.emplace_back("engine-init");
        Check(table == 0x8e3d98u && flags == 0u && resource_arena_size == 4u * 1024u * 1024u,
              "RenderWare engine initialization: allocator, flags, 4 MiB arena");
        return failure != Failure::EngineInit;
    }
    std::uint32_t AttachPlugins() override {
        events.emplace_back("plugins");
        return failure != Failure::Plugins;
    }
    std::uint32_t BindInputCallback(const std::uint32_t selector,
                                     const std::uint32_t callback) override {
        events.emplace_back(selector == 0u ? "keyboard" : "mouse");
        Check((selector == 0u && callback == 0x409630u) ||
                  (selector == 1u && callback == 0x409670u),
              "keyboard and mouse dispatcher addresses");
        return !((selector == 0u && failure == Failure::Keyboard) ||
                 (selector == 1u && failure == Failure::Mouse));
    }
    std::uint32_t OpenEngine(const std::uint32_t window) override {
        events.emplace_back("open");
        Check(window == 0x1234u, "engine receives game window handle");
        return failure != Failure::Open;
    }
    void ShutdownEngine() override { events.emplace_back("shutdown"); }
    std::uint32_t SelectDevice() override {
        events.emplace_back("select-device");
        return failure != Failure::SelectDevice;
    }
    void SetRefreshRate(const std::uint32_t rate) override {
        events.emplace_back("refresh");
        Check(rate == 0u, "refresh rate is zero");
    }
    void EnableDialogBoxMode(const std::uint32_t enabled) override {
        events.emplace_back("dialog-mode");
        Check(enabled == 1u, "dialog-box mode is enabled before engine Start");
    }
    std::uint32_t StartEngine() override {
        events.emplace_back("start");
        return failure != Failure::Start;
    }
    void CloseEngine() override { events.emplace_back("close"); }
    void StopEngine() override { events.emplace_back("stop"); }
    std::uint32_t GetD3D9Caps() override {
        events.emplace_back("d3d-caps");
        return 0x7777u;
    }
    std::uint32_t GetCurrentD3DDevice() override {
        events.emplace_back("d3d-device");
        return 0x8888u;
    }
    std::uint32_t InitializeImageHandlers() override {
        events.emplace_back("image-handlers");
        return image_handlers_result;
    }
    std::uint32_t OpenCharset() override {
        events.emplace_back("charset-open");
        return failure != Failure::CharsetOpen;
    }
    std::uint32_t OpenFileSystemManager(
        const std::uint32_t sentinel) override {
        events.emplace_back("fs-manager-open");
        Check(sentinel == 0xffffffffu, "filesystem manager max-file sentinel");
        return failure != Failure::FileSystemManagerOpen;
    }
    std::uint32_t InitializeVirtualFileSystem() override {
        events.emplace_back("filesystem");
        return failure != Failure::FileSystem;
    }
    std::uint32_t InitializeDirectInput(
        const std::uint32_t instance, const std::uint32_t window,
        const std::uint32_t flags) override {
        events.emplace_back("direct-input");
        Check(instance == 0x5678u && window == 0x1234u && flags == 0u,
              "DirectInput receives instance, window and zero");
        return failure != Failure::DirectInput;
    }
    void SetInputParameters(const std::uint32_t first,
                             const std::uint32_t second,
                             const std::uint32_t width,
                             const std::uint32_t height) override {
        events.emplace_back("input-parameters");
        Check(first == 0u && second == 0u &&
                  width == 1280u && height == 720u,
              "input parameters use zero origin and configured dimensions");
    }
    void SetInputScalar(const std::uint32_t bits) override {
        events.emplace_back("input-scalar");
        Check(bits == 0x3f800000u, "input scalar is float 1.0 bits");
    }
    std::uint32_t InitializeLogger() override {
        events.emplace_back("logger-init");
        return 0x9999u;
    }
    void AppendLog(const std::uint32_t logger,
                   const std::string_view format,
                   const std::string_view source,
                   const std::uint32_t line) override {
        events.emplace_back("logger-append");
        Check(logger == 0x9999u &&
                  source ==
                      R"(D:\\VcProject\\Pal4\\project\\Sourcecode\\RwCasing\\rwc_Initialize.cpp)",
              "logger and original source path");
        logs.emplace_back(std::string(format));
        lines.push_back(line);
    }

    Failure failure = Failure::None;
    std::uint32_t image_handlers_result = 1u;
    std::vector<std::string> events;
    std::vector<std::string> logs;
    std::vector<std::uint32_t> lines;
};

struct Fixture {
    pal4::GameConfigWords config{};
    pal4::GameRenderState state{0x9000u};
    Runtime runtime;

    Fixture() {
        config[0] = 1280u;
        config[1] = 720u;
        state.Write(0u, 0x1234u);
        state.Write(4u, 0x5678u);
        state.Write(0x55cu, 0xa5a5u);
        state.Write(0x560u, 0xb6b6u);
    }

    std::uint32_t Initialize() {
        return pal4::GameSystemsInitializer(config, state).Initialize(runtime);
    }
};

constexpr std::string_view kLogPrefix =
    "FILE:  [%s]\nLINE:  [%d]\nINFO:  ";

void CheckLog(const Runtime& runtime, const std::string_view message,
              const std::uint32_t line) {
    Check(runtime.logs.size() == 1u && runtime.lines.size() == 1u &&
              runtime.logs[0] ==
                  std::string(kLogPrefix) + std::string(message) + "\n\n\n" &&
              runtime.lines[0] == line,
          "error log text and source line");
}

void EarlyFailures() {
    struct Case {
        Failure failure;
        std::vector<std::string> events;
    };
    for (const auto& entry : {
             Case{Failure::EngineInit,
                  {"allocator", "engine-init", "logger-init",
                   "logger-append"}},
             Case{Failure::Plugins,
                  {"allocator", "engine-init", "plugins"}},
             Case{Failure::Keyboard,
                  {"allocator", "engine-init", "plugins", "keyboard"}},
             Case{Failure::Mouse,
                  {"allocator", "engine-init", "plugins", "keyboard",
                   "mouse"}},
         }) {
        Fixture fixture;
        fixture.runtime.failure = entry.failure;
        Check(fixture.Initialize() == 0u &&
                  fixture.runtime.events == entry.events &&
                  fixture.state.Read(0x55cu) == 0xa5a5u,
              "early failure stops without engine cleanup");
        if (entry.failure == Failure::EngineInit) {
            CheckLog(fixture.runtime,
                     "Error[RWCInitializeRW failed <MemoryFunctions error>]",
                     0x403u);
        } else {
            Check(fixture.runtime.logs.empty(),
                  "plugin and callback failures do not log here");
        }
    }
}

void DeviceAndStartFailures() {
    const std::vector<std::string> prefix{
        "allocator", "engine-init", "plugins", "keyboard", "mouse",
        "open"};
    {
        Fixture fixture;
        fixture.runtime.failure = Failure::Open;
        auto expected = prefix;
        expected.insert(expected.end(),
                        {"logger-init", "logger-append", "shutdown"});
        Check(fixture.Initialize() == 0u &&
                  fixture.runtime.events == expected,
              "engine open failure logs and shuts down only");
        CheckLog(fixture.runtime,
                 "Error[RWCInitializeRW failed <RwEngineOpen error>]",
                 0x418u);
    }
    {
        Fixture fixture;
        fixture.runtime.failure = Failure::SelectDevice;
        auto expected = prefix;
        expected.insert(expected.end(),
                        {"select-device", "close", "shutdown"});
        Check(fixture.Initialize() == 0u &&
                  fixture.runtime.events == expected &&
                  fixture.runtime.logs.empty(),
              "device selection failure closes then shuts down");
    }
    {
        Fixture fixture;
        fixture.runtime.failure = Failure::Start;
        auto expected = prefix;
        expected.insert(expected.end(),
                        {"select-device", "refresh", "dialog-mode", "start",
                         "logger-init", "logger-append", "close", "shutdown"});
        Check(fixture.Initialize() == 0u &&
                  fixture.runtime.events == expected,
              "engine start failure logs, closes and shuts down");
        CheckLog(fixture.runtime,
                 "Error[RWCInitializeRW failed <RwEngineStart error>]",
                 0x43du);
    }
}

void LateFailures() {
    const std::vector<std::string> prefix{
        "allocator", "engine-init", "plugins", "keyboard", "mouse", "open",
        "select-device", "refresh", "dialog-mode", "start", "d3d-caps",
        "d3d-device", "image-handlers", "charset-open"};
    {
        Fixture fixture;
        fixture.runtime.failure = Failure::CharsetOpen;
        auto expected = prefix;
        expected.insert(expected.end(),
                        {"logger-init", "logger-append", "stop", "close",
                         "shutdown"});
        Check(fixture.Initialize() == 0u &&
                  fixture.runtime.events == expected &&
                  fixture.state.Read(0x55cu) == 0x7777u &&
                  fixture.state.Read(0x560u) == 0x8888u,
              "charset open failure preserves state writes and full teardown");
        CheckLog(fixture.runtime,
                 "Error[RWCInitializeRW failed <RtCharsetOpen error>]",
                 0x454u);
    }
    for (const Failure failure :
         {Failure::FileSystemManagerOpen, Failure::FileSystem}) {
        Fixture fixture;
        fixture.runtime.failure = failure;
        auto expected = prefix;
        expected.push_back("fs-manager-open");
        if (failure == Failure::FileSystem) {
            expected.push_back("filesystem");
        }
        Check(fixture.Initialize() == 0u &&
                  fixture.runtime.events == expected &&
                  fixture.runtime.logs.empty(),
              "filesystem manager and VFS failures return without teardown");
    }
}

void SuccessAndIgnoredFailures() {
    const std::vector<std::string> success{
        "allocator", "engine-init", "plugins", "keyboard", "mouse", "open",
        "select-device", "refresh", "dialog-mode", "start", "d3d-caps",
        "d3d-device", "image-handlers", "charset-open", "fs-manager-open",
        "filesystem", "direct-input", "input-parameters", "input-scalar",
        "logger-init", "logger-append"};
    {
        Fixture fixture;
        fixture.runtime.image_handlers_result = 0u;
        Check(fixture.Initialize() == 1u &&
                  fixture.runtime.events == success &&
                  fixture.state.Read(0x55cu) == 0x7777u &&
                  fixture.state.Read(0x560u) == 0x8888u,
              "image registration result is ignored on full success path");
        CheckLog(fixture.runtime, "RWCInitializeRW success", 0x475u);
    }
    {
        Fixture fixture;
        fixture.runtime.failure = Failure::DirectInput;
        auto expected = success;
        expected.insert(expected.begin() + 17,
                        {"logger-init", "logger-append"});
        Check(fixture.Initialize() == 1u &&
                  fixture.runtime.events == expected &&
                  fixture.runtime.logs.size() == 2u &&
                  fixture.runtime.lines ==
                      std::vector<std::uint32_t>{0x46cu, 0x475u} &&
                  fixture.runtime.logs[0] ==
                      std::string(kLogPrefix) +
                          "Error[RWCInitializeRW failed <GameInput error>]\n\n\n" &&
                  fixture.runtime.logs[1] ==
                      std::string(kLogPrefix) +
                          "RWCInitializeRW success\n\n\n",
              "DirectInput failure logs but input setup and success continue");
    }
}

}  // namespace

int main() {
    EarlyFailures();
    DeviceAndStartFailures();
    LateFailures();
    SuccessAndIgnoredFailures();
    std::cout << "12 game system initialization paths passed\n";
}

