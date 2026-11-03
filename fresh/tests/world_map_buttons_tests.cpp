#include "pal4/world_map_buttons.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool value, const char* message) {
    if (!value) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IWorldMapButtonsRuntime {
    static constexpr std::uint32_t kFrame = 0x5000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t interpreter = 0x7000u;
    bool module_added = true;
    std::uint32_t engine_calls = 0u;

    Runtime() {
        for (std::uint32_t i = 0u; i < 21u; ++i) {
            words[kFrame + 0x40u + i * 4u] = 0x6000u + i;
        }
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        Check(value == 1u && address >= kFrame + 0x1e4u &&
                  address <= kFrame + 0x234u &&
                  (address - kFrame - 0x1e4u) % 4u == 0u,
              "button flag offset and value");
        calls.emplace_back("write");
        words[address] = value;
    }
    void SetVisible(const std::uint32_t window,
                    const bool visible) override {
        Check(window >= 0x6000u && window <= 0x6014u && !visible,
              "button hidden");
        calls.emplace_back("hide");
    }
    void SetEnabled(const std::uint32_t window,
                    const bool enabled) override {
        Check(window >= 0x6000u && window <= 0x6014u && enabled,
              "button enabled");
        calls.emplace_back("enable");
    }
    void InitializeScripts() override {
        calls.emplace_back("script init");
    }
    std::uint32_t ScriptMode() override {
        calls.emplace_back("mode");
        return 3u;
    }
    std::uint32_t CreateInterpreter(
        const std::uint32_t mode, const std::string_view name,
        const std::string_view entry, const std::string_view full_path,
        const std::string_view file, const std::uint32_t reader) override {
        Check(mode == 3u && name == "WorldMapIniter" && entry == "init" &&
                  full_path == "init" &&
                  file == "gamedata\\editData\\script\\worldMap.csb" &&
                  reader == 0x600d50u,
              "world-map script interpreter arguments");
        calls.emplace_back("create");
        return interpreter;
    }
    std::uint32_t ScriptEngine() override {
        calls.emplace_back("engine");
        return 0x8000u + engine_calls++;
    }
    bool AddScriptModule(const std::uint32_t context,
                 const std::uint32_t script) override {
        Check(context == 0x8054u && script == interpreter,
              "script module context");
        calls.emplace_back("add module");
        return module_added;
    }
    void ExecuteImmediate(const std::uint32_t engine,
                         const std::uint32_t script) override {
        Check(engine == 0x8001u && script == interpreter,
              "second engine lookup for synchronous execution");
        calls.emplace_back("execute immediate");
    }
    void ReleaseInterpreter(const std::uint32_t script) override {
        Check(script == interpreter, "initializer interpreter release");
        calls.emplace_back("release");
    }
    std::uint32_t CleanupAllScripts() override {
        calls.emplace_back("cleanup");
        return 0x1234u;
    }
};

struct StateRuntime final : pal4::IWorldMapButtonStateRuntime {
    static constexpr std::uint32_t kFrame = 0x5000u;
    std::map<std::uint32_t, std::uint32_t> words{{0x7104u, 0x7200u}};
    std::map<std::uint32_t, std::string> texts{{0x7200u, "m17"}};
    std::vector<std::string> calls;
    bool fail_visibility = false;
    std::uint32_t comparisons = 0u;

    StateRuntime() {
        for (std::uint32_t i = 0u; i < 21u; ++i) {
            words[kFrame + 0x40u + i * 4u] = 0x6000u + i;
            texts[kFrame + 0x94u + i * 0x10u] =
                "Q" + std::to_string(i);
        }
        texts[kFrame + 0x94u + 5u * 0x10u] = "M17";
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    std::uint32_t MakeUppercaseName(const std::uint32_t source) override {
        calls.emplace_back("uppercase");
        auto text = source == 0u ? std::string{} : texts.at(source);
        std::transform(text.begin(), text.end(), text.begin(),
                       [](unsigned char c) { return std::toupper(c); });
        texts[0x7300u] = text;
        return 0x7300u;
    }
    std::string_view NameText(const std::uint32_t name) override {
        Check(name == 0x7300u, "temporary uppercase name");
        calls.emplace_back("name text");
        return texts.at(name);
    }
    std::string_view RecordText(const std::uint32_t record) override {
        ++comparisons;
        calls.emplace_back("compare");
        return texts.at(record);
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        Check(address >= kFrame + 0x1e4u &&
                  address <= kFrame + 0x234u,
              "matched button state address");
        calls.emplace_back("write");
        words[address] = value;
    }
    void SetVisible(const std::uint32_t window,
                    const bool visible) override {
        Check(window >= 0x6000u && window <= 0x6014u,
              "matched button window");
        calls.emplace_back(visible ? "show" : "hide");
        if (fail_visibility) throw std::runtime_error("visibility");
    }
    std::uint32_t CleanupName(const std::uint32_t name) override {
        Check(name == 0x7300u, "temporary name cleanup");
        calls.emplace_back("cleanup");
        return 0x1234u;
    }
};

struct ExternalRuntime final : pal4::IWorldMapExternalButtonRuntime {
    std::vector<std::string> calls;
    std::uint32_t frame = StateRuntime::kFrame;
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x4000u;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        Check(address == 0x8b089cu, "world-map name source");
        calls.emplace_back("name");
        return 0x8000u;
    }
    std::uint32_t FindFrame(const std::uint32_t manager,
                            const std::uint32_t name) override {
        Check(manager == 0x4000u && name == 0x8000u,
              "world-map frame lookup");
        calls.emplace_back("find");
        return frame;
    }
};

void TestInitializerExecution() {
    Runtime runtime;
    Check(pal4::WorldMapButtons::Initialize(Runtime::kFrame, runtime) ==
              0x1234u, "script cleanup result");
    Check(runtime.calls.size() == 21u * 5u + 9u,
          "all button and script operations recorded");
    for (std::uint32_t i = 0u; i < 21u; ++i) {
        const auto offset = static_cast<std::size_t>(i) * 5u;
        Check(runtime.calls[offset] == "read" &&
                  runtime.calls[offset + 1u] == "write" &&
                  runtime.calls[offset + 2u] == "hide" &&
                  runtime.calls[offset + 3u] == "read" &&
                  runtime.calls[offset + 4u] == "enable",
              "button operations and repeated window read");
        Check(runtime.words.at(Runtime::kFrame + 0x1e4u + i * 4u) == 1u,
              "button state flag");
    }
    Check(std::vector<std::string>(runtime.calls.end() - 9,
                                   runtime.calls.end()) ==
              std::vector<std::string>{"script init", "mode", "create",
                                       "engine", "add module", "engine",
                                       "execute immediate", "release", "cleanup"},
          "module registration and synchronous execution order");
}

void TestModuleRegistrationFailure() {
    Runtime runtime;
    runtime.module_added = false;
    (void)pal4::WorldMapButtons::Initialize(Runtime::kFrame, runtime);
    Check(runtime.engine_calls == 1u &&
              std::vector<std::string>(runtime.calls.end() - 6,
                                       runtime.calls.end()) ==
                  std::vector<std::string>{"script init", "mode", "create",
                                           "engine", "add module", "cleanup"},
          "failed module registration skips initializer execution");
}

void TestMissingInterpreter() {
    Runtime runtime;
    runtime.interpreter = 0u;
    (void)pal4::WorldMapButtons::Initialize(Runtime::kFrame, runtime);
    Check(runtime.engine_calls == 0u &&
              std::vector<std::string>(runtime.calls.end() - 4,
                                       runtime.calls.end()) ==
                  std::vector<std::string>{"script init", "mode", "create",
                                           "cleanup"},
          "null interpreter still cleans up scripts");
}

void TestSetVisibility() {
    StateRuntime runtime;
    Check(pal4::WorldMapButtons::SetVisibility(StateRuntime::kFrame,
                                                0x7100u, 9u, runtime) ==
              0x1234u &&
              runtime.comparisons == 6u &&
              runtime.words.at(StateRuntime::kFrame + 0x1e4u + 5u * 4u) ==
                  9u &&
              runtime.calls[runtime.calls.size() - 3u] == "read" &&
              runtime.calls[runtime.calls.size() - 2u] == "show" &&
              runtime.calls.back() == "cleanup",
          "uppercase match stores raw state and shows button");

    runtime = StateRuntime{};
    Check(pal4::WorldMapButtons::SetVisibility(StateRuntime::kFrame,
                                                0x7100u, 0u, runtime) ==
              0x1234u && runtime.calls[runtime.calls.size() - 2u] == "hide",
          "zero state hides matched button");

    runtime = StateRuntime{};
    runtime.texts[0x7200u] = "unknown";
    (void)pal4::WorldMapButtons::SetVisibility(StateRuntime::kFrame,
                                                0x7100u, 1u, runtime);
    Check(runtime.comparisons == 21u && runtime.calls.back() == "cleanup" &&
              runtime.words.find(StateRuntime::kFrame + 0x1e4u) ==
                  runtime.words.end(),
          "unknown name leaves all button states untouched");

    runtime = StateRuntime{};
    runtime.words[0x7104u] = 0u;
    (void)pal4::WorldMapButtons::SetVisibility(StateRuntime::kFrame,
                                                0x7100u, 1u, runtime);
    Check(runtime.comparisons == 21u && runtime.calls.back() == "cleanup",
          "null source becomes empty name");
}

void TestStateExceptionCleanup() {
    StateRuntime runtime;
    runtime.fail_visibility = true;
    bool caught = false;
    try {
        (void)pal4::WorldMapButtons::SetVisibility(StateRuntime::kFrame,
                                                     0x7100u, 1u, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls.back() == "cleanup",
          "visibility exception cleans temporary name");
}

void TestExternalVisibility() {
    ExternalRuntime external;
    StateRuntime buttons;
    Check(pal4::WorldMapButtons::DispatchExternalVisibility(
              0x7100u, 7u, external, buttons) == 0x1234u &&
              external.calls == std::vector<std::string>{
                  "manager", "name", "find"} &&
              buttons.words.at(StateRuntime::kFrame + 0x1e4u + 5u * 4u) ==
                  7u,
          "external handler composes named-button state update");

    external = ExternalRuntime{};
    external.frame = 0u;
    buttons = StateRuntime{};
    Check(pal4::WorldMapButtons::DispatchExternalVisibility(
              0x7100u, 7u, external, buttons) == 0u &&
              buttons.calls.empty(),
          "missing world-map frame returns null without button access");
}

}  // namespace

int main() {
    TestInitializerExecution();
    TestModuleRegistrationFailure();
    TestMissingInterpreter();
    TestSetVisibility();
    TestStateExceptionCleanup();
    TestExternalVisibility();
    return EXIT_SUCCESS;
}
