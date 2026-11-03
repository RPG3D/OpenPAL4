#include "pal4/world_loading_state.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IWorldLoadingStateRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    unsigned initialized = 0;
    unsigned fail_at = 0;

    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write " + std::to_string(address) + "=" +
                        std::to_string(value));
    }
    void InitializeString(std::uint32_t address) override {
        calls.push_back("init " + std::to_string(address));
        if (++initialized == fail_at) throw std::runtime_error("injected init failure");
    }
    std::uint32_t CleanupString(std::uint32_t address) override {
        calls.push_back("cleanup " + std::to_string(address));
        return address + 1u;
    }
    void DestroyBase(std::uint32_t object) override {
        Check(object == 0x1000u, "base destruction object");
        calls.emplace_back("base destructor");
    }
};

struct ConfigRuntime final : pal4::IWorldLoadingConfigRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::string> strings;
    std::map<std::string, std::optional<std::string>> attributes;
    std::map<std::string, std::optional<std::uint32_t>> integers;
    std::vector<std::string> calls;
    std::optional<std::uint32_t> type;

    std::uint32_t GameState() override { calls.emplace_back("game-state"); return 0x2000u; }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write:" + std::to_string(address));
    }
    std::uint32_t CameraManager() override { calls.emplace_back("camera"); return 3u; }
    std::uint32_t DefaultCameraName() override { return 4u; }
    void ActivateCamera(std::uint32_t manager, std::uint32_t name) override {
        Check(manager == 3u && name == 4u, "loading camera arguments");
        calls.emplace_back("activate-camera");
    }
    std::uint32_t UIFrameManager() override { calls.emplace_back("ui"); return 5u; }
    std::uint32_t LoadingWindow(std::uint32_t manager) override {
        Check(manager == 5u, "loading window manager");
        calls.emplace_back("loading-window"); return 6u;
    }
    void SetLoadingWindowVisible(std::uint32_t window, bool visible) override {
        Check(window == 6u && visible, "loading window visibility");
        calls.emplace_back("show-loading");
    }
    std::optional<std::uint32_t> ReadType(
        std::uint32_t config, std::string_view key) override {
        Check(config == 0x3000u && key == "PALGAMEIV_LOADING_TYPE",
              "type attribute lookup");
        calls.emplace_back("read-type"); return type;
    }
    std::optional<std::string> ReadString(
        std::uint32_t config, std::string_view key) override {
        Check(config == 0x3000u, "string attribute config");
        calls.push_back("read:" + std::string(key));
        return attributes[std::string(key)];
    }
    void AssignString(std::uint32_t destination,
                      std::string_view value) override {
        strings[destination] = value;
        calls.push_back("assign:" + std::to_string(destination));
    }
    std::optional<std::uint32_t> ReadTreeInteger(
        std::uint32_t config, std::string_view key) override {
        Check(config == 0x3000u, "integer attribute config");
        calls.push_back("integer:" + std::string(key));
        return integers[std::string(key)];
    }
};

struct FlowRuntime final : pal4::IWorldLoadingFlowRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    FlowRuntime() {
        words[0x1000u + 12u] = 11u;
        words[0x1000u + 24u] = 22u;
        words[0x1000u + 36u] = 33u;
        words[0x1000u + 48u] = 44u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write:" + std::to_string(address));
    }
    std::uint32_t UIFrameManager() override { calls.emplace_back("ui"); return 5u; }
    std::uint32_t RenderUI(std::uint32_t manager, std::uint32_t frame) override {
        Check(manager == 5u && frame == 0x5000u, "loading UI render args");
        calls.emplace_back("render-ui"); return 0x99u;
    }
    std::uint32_t LoadingWindow(std::uint32_t manager) override {
        Check(manager == 5u, "loading window args");
        calls.emplace_back("loading-window"); return 6u;
    }
    void SetLoadingWindowVisible(std::uint32_t window, bool visible) override {
        Check(window == 6u && !visible, "loading window hide args");
        calls.emplace_back("hide-loading");
    }
    void LoadSave(std::uint32_t slot) override {
        Check(slot == 44u, "save slot"); calls.emplace_back("load-save");
    }
    std::uint32_t GameInstance() override { calls.emplace_back("game"); return 7u; }
    std::uint32_t InitializeStateAndUI(std::uint32_t game,
                                        std::uint32_t state) override {
        Check(game == 7u && state == 1u, "game state init args");
        calls.emplace_back("initialize-game"); return 0x77u;
    }
    std::uint32_t ArenaManager() override { calls.emplace_back("arena"); return 0x2000u; }
    void LoadSceneResources(std::uint32_t arena, std::uint32_t scene,
                            std::uint32_t area) override {
        Check(arena == 0x2000u && scene == 11u && area == 22u,
              "scene resource args");
        calls.emplace_back("load-resources");
    }
    void LoadSceneRuntime(std::uint32_t arena, std::uint32_t resources) override {
        Check(arena == 0x2000u && resources == 33u, "scene runtime args");
        calls.emplace_back("load-runtime");
    }
    void InitializePlayerScene(std::uint32_t arena) override {
        Check(arena == 0x2000u, "player scene arena");
        calls.emplace_back("initialize-player");
    }
};

void TestNormal() {
    Runtime runtime;
    Check(pal4::WorldLoadingState::Construct(0x1000u, runtime) == 0x1000u,
          "constructor return");
    Check(runtime.words[0x1000u] == pal4::WorldLoadingState::kVtable &&
          runtime.words[0x1004u] == 0u && runtime.words[0x1008u] == 0u &&
          runtime.words[0x1030u] == 0u && runtime.words[0x1034u] == 0u,
          "vtable and four cleared fields");
    Check(runtime.calls.size() == 9u &&
          runtime.calls.front() == "write 4096=" +
              std::to_string(pal4::WorldLoadingState::kBaseVtable) &&
          runtime.calls[1] == "init 4108" &&
          runtime.calls[2] == "init 4120" &&
          runtime.calls[3] == "init 4132" &&
          runtime.calls[4] == "write 4104=0" &&
          runtime.calls[5] == "write 4144=0" &&
          runtime.calls[6] == "write 4148=0" &&
          runtime.calls[7] == "write 4100=0" &&
          runtime.calls.back() == "write 4096=" +
              std::to_string(pal4::WorldLoadingState::kVtable),
          "constructor write order");
    runtime.calls.clear();
    Check(pal4::WorldLoadingState::Destroy(0x1000u, runtime) == 4109u,
          "destructor forwards first-buffer cleanup result");
    Check(runtime.calls.size() == 5u &&
          runtime.calls[1] == "cleanup 4132" &&
          runtime.calls[2] == "cleanup 4120" &&
          runtime.calls[3] == "cleanup 4108" &&
          runtime.words[0x1000u] == pal4::WorldLoadingState::kBaseVtable,
          "destructor reverse cleanup and base vtable");
}

void TestFailure(unsigned fail_at,
                 const std::vector<std::string>& expected) {
    Runtime runtime;
    runtime.fail_at = fail_at;
    bool failed = false;
    try {
        static_cast<void>(pal4::WorldLoadingState::Construct(0x1000u, runtime));
    } catch (const std::runtime_error&) {
        failed = true;
    }
    Check(failed && runtime.calls == expected,
          "constructor exception cleans completed strings in reverse order");
}

void TestConfig() {
    ConfigRuntime runtime;
    Check(pal4::WorldLoadingState::LoadConfig(
              0x1000u, 99u, 0u, runtime) == 0u && runtime.calls.empty(),
          "null config has no side effects");
    runtime.attributes["PALGAMEIV_LOADING_ARENA"] = "arena";
    runtime.attributes["PALGAMEIV_LOADING_AREA_RES"] = "resources";
    runtime.integers["PALGAMEIV_LOADING_AUTOREADY"] = 1u;
    Check(pal4::WorldLoadingState::LoadConfig(
              0x1000u, 99u, 0x3000u, runtime) == 1u,
          "default type loads world scene");
    Check(runtime.words[0x2000u + 40u] == 0u &&
          runtime.words[0x1000u + 8u] == 0u &&
          runtime.words[0x1000u + 52u] == 1u &&
          runtime.words[0x1000u + 4u] == 0u &&
          runtime.strings[0x1000u + 12u] == "arena" &&
          runtime.strings[0x1000u + 24u].empty() &&
          runtime.strings[0x1000u + 36u] == "resources",
          "scene fields and missing-string fallback");
    Check(runtime.calls == std::vector<std::string>{
        "game-state", "write:8232", "camera", "activate-camera", "ui",
        "loading-window", "show-loading", "read-type", "write:4104",
        "read:PALGAMEIV_LOADING_ARENA", "assign:4108",
        "read:PALGAMEIV_LOADING_AREA", "assign:4120",
        "read:PALGAMEIV_LOADING_AREA_RES", "assign:4132",
        "integer:PALGAMEIV_LOADING_AUTOREADY", "write:4148", "write:4100"},
        "scene initialization order");
    runtime.calls.clear(); runtime.type = 1u;
    runtime.integers["PALGAMEIV_LOADING_ARCHIVE"] = 42u;
    Check(pal4::WorldLoadingState::LoadConfig(
              0x1000u, 0u, 0x3000u, runtime) == 1u &&
          runtime.words[0x1000u + 8u] == 1u &&
          runtime.words[0x1000u + 48u] == 42u &&
          runtime.calls.size() == 12u,
          "archive branch and numeric attribute");
    runtime.calls.clear(); runtime.type = 1u;
    runtime.integers["PALGAMEIV_LOADING_ARCHIVE"] = std::nullopt;
    Check(pal4::WorldLoadingState::LoadConfig(
              0x1000u, 0u, 0x3000u, runtime) == 1u &&
          runtime.words[0x1000u + 48u] == 0u,
          "missing archive defaults to zero");
    runtime.calls.clear(); runtime.type = 2u;
    Check(pal4::WorldLoadingState::LoadConfig(
              0x1000u, 0u, 0x3000u, runtime) == 1u &&
          runtime.words[0x1000u + 8u] == 2u &&
          runtime.calls.size() == 10u &&
          runtime.calls.back() == "write:4100",
          "other type skips scene and archive fields");
}

void TestFlow() {
    FlowRuntime runtime;
    Check(pal4::WorldLoadingState::RenderUI(0x5000u, runtime) == 0x99u &&
          runtime.calls == std::vector<std::string>{"ui", "render-ui"},
          "render wrapper forwards UI return");
    runtime.calls.clear();
    Check(pal4::WorldLoadingState::HideLoadingWindow(999u, runtime) == 1u &&
          runtime.calls == std::vector<std::string>{
              "ui", "loading-window", "hide-loading"},
          "loading window hide wrapper");
    runtime.calls.clear(); runtime.words[0x1000u + 4u] = 9u;
    Check(pal4::WorldLoadingState::Advance(0x1000u, runtime) == 9u &&
          runtime.calls == std::vector<std::string>{"read:4100"},
          "already loaded path returns current flag");
    runtime.calls.clear(); runtime.words[0x1000u + 4u] = 0u;
    runtime.words[0x1000u + 8u] = 1u;
    Check(pal4::WorldLoadingState::Advance(0x1000u, runtime) == 0x77u &&
          runtime.words[0x1000u + 4u] == 1u &&
          runtime.calls == std::vector<std::string>{
              "read:4100", "read:4104", "read:4144", "load-save",
              "game", "initialize-game", "write:4100"},
          "archive transition loads save then initializes game");
    runtime.calls.clear(); runtime.words[0x1000u + 4u] = 0u;
    runtime.words[0x1000u + 8u] = 0u;
    Check(pal4::WorldLoadingState::Advance(0x1000u, runtime) == 0u &&
          runtime.words[0x1000u + 4u] == 1u &&
          runtime.calls == std::vector<std::string>{
              "read:4100", "read:4104", "read:4108", "read:4120",
              "arena", "load-resources", "read:4132", "arena",
              "load-runtime", "arena", "initialize-player", "game",
              "initialize-game", "read:4148", "write:4100"},
          "scene transition loads resources, runtime and player in order");
    runtime.calls.clear(); runtime.words[0x1000u + 4u] = 0u;
    runtime.words[0x1000u + 52u] = 1u;
    Check(pal4::WorldLoadingState::Advance(0x1000u, runtime) == 0x2000u &&
          runtime.words[0x2000u + 196u] == 1u &&
          runtime.words[0x1000u + 4u] == 1u,
          "auto-ready enables arena and returns it");
    runtime.calls.clear(); runtime.words[0x1000u + 4u] = 0u;
    runtime.words[0x1000u + 8u] = 3u;
    Check(pal4::WorldLoadingState::Advance(0x1000u, runtime) == 2u &&
          runtime.calls == std::vector<std::string>{
              "read:4100", "read:4104", "write:4100"},
          "unsupported type returns type minus one and marks loaded");
}

}  // namespace

int main() {
    try {
        TestNormal();
        const auto base = "write 4096=" +
            std::to_string(pal4::WorldLoadingState::kBaseVtable);
        TestFailure(1u, {base, "init 4108", "base destructor"});
        TestFailure(2u, {base, "init 4108", "init 4120",
                         "cleanup 4108", "base destructor"});
        TestFailure(3u, {base, "init 4108", "init 4120", "init 4132",
                         "cleanup 4120", "cleanup 4108", "base destructor"});
        TestConfig();
        TestFlow();
        std::cout << "world loading state tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
