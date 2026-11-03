#include "pal4/system_setting_visibility.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ISystemSettingVisibilityRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x4024u, 0x8000u}, {0x40b8u, 0x8100u},
        {0x7108u, 0u}};
    std::vector<std::string> calls;
    std::vector<std::tuple<std::uint32_t, std::uint32_t,
                           std::uint32_t>> events;
    std::vector<std::uint32_t> reads;
    std::vector<bool> enabled;
    bool throw_callback = false;

    void SetParent(std::uint32_t frame, std::uint8_t visible,
                   std::uint32_t parent) override {
        Check(frame == 0x4000u && parent == 0x9000u &&
                  (visible == 0u || visible == 0x81u),
              "parent and raw visibility forwarded");
        calls.emplace_back("parent");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x6000u;
    }
    void ProcessUiEvent(std::uint32_t tree,
                        std::uint32_t key,
                        std::uint32_t first,
                        std::uint32_t second) override {
        Check(tree == 0x6104u, "UI event tree offset");
        calls.emplace_back("event");
        events.emplace_back(key, first, second);
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        reads.push_back(address);
        calls.emplace_back("read");
        return words.at(address);
    }
    void ShowSystemFrame(std::uint32_t frame) override {
        Check(frame == 0x4000u, "system frame show target");
        calls.emplace_back("show-system");
    }
    void UpdateStats(std::uint32_t frame) override {
        Check(frame == 0x4000u, "menu stats frame");
        calls.emplace_back("stats");
    }
    void InsertActive(std::uint32_t collection,
                      std::uint32_t member) override {
        Check(collection == 0x6134u && member == 0x4028u,
              "active insertion addresses");
        calls.emplace_back("insert");
    }
    void RemoveActive(std::uint32_t collection,
                      std::uint32_t member) override {
        Check(collection == 0x6134u && member == 0x4028u,
              "active removal addresses");
        calls.emplace_back("remove");
    }
    void SetEnabled(std::uint32_t window,
                    bool value) override {
        Check(window == 0x8100u, "enabled window at +0xb8");
        calls.emplace_back(value ? "enable" : "disable");
        enabled.push_back(value);
    }
    std::uint32_t ArenaManager() override {
        calls.emplace_back("arena-manager");
        return 0x7000u;
    }
    std::uint32_t CreateEventArgs() override {
        calls.emplace_back("create-args");
        return 0xa000u;
    }
    void DestroyEventArgs(std::uint32_t event) override {
        Check(event == 0xa000u, "event args cleanup handle");
        calls.emplace_back("destroy-args");
    }
    void InvokeLoadData(std::uint32_t frame,
                        std::uint32_t event) override {
        Check(frame == 0x4000u && event == 0xa000u,
              "load callback event");
        calls.emplace_back("load-data");
        if (throw_callback) throw std::runtime_error("load");
    }
    void InvokeStartNewGame(std::uint32_t frame,
                            std::uint32_t event) override {
        Check(frame == 0x4000u && event == 0xa000u,
              "new-game callback event");
        calls.emplace_back("start-game");
        if (throw_callback) throw std::runtime_error("start");
    }
    void UpdateSystemDialog(std::uint32_t manager,
                            std::uint32_t key) override {
        Check(manager == 0x6000u && key == 7u,
              "tutorial dialog update key");
        calls.emplace_back("dialog");
    }
    std::uint32_t InitializeGlobalData() override {
        calls.emplace_back("global-data");
        return 0xb000u;
    }
    void SaveSettings(std::uint32_t global_data) override {
        Check(global_data == 0xb000u, "settings save receives global data");
        calls.emplace_back("save");
    }
};

void CheckEvents(const Runtime& runtime, std::uint32_t visibility,
                 bool includes_help) {
    auto expected = std::vector<std::tuple<std::uint32_t,
                                           std::uint32_t,
                                           std::uint32_t>>{
        {pal4::SystemSettingVisibility::kShowFrameTool,
         visibility, 0u},
        {pal4::SystemSettingVisibility::kShowGameInfo,
         visibility, 0x8000u}};
    if (includes_help) {
        expected.emplace_back(pal4::SystemSettingVisibility::kHelpTips,
                              13u, 0u);
    }
    Check(runtime.events == expected,
          "show_frame_tool, show_game_info and optional help event payloads");
}

void CheckHidden() {
    Runtime runtime;
    pal4::SystemSettingVisibility::SetVisible(
        0x4000u, 0u, 0x9000u, runtime);
    CheckEvents(runtime, 0u, false);
    Check(runtime.calls == std::vector<std::string>{
                               "parent", "manager", "event", "read",
                               "manager", "event", "show-system",
                               "global-data", "save", "manager", "remove"},
          "hide path saves settings then removes active member");
    Check(runtime.reads == std::vector<std::uint32_t>{0x4024u},
          "hide path does not query arena or enabled window");
}

void CheckShownWithoutArena() {
    Runtime runtime;
    pal4::SystemSettingVisibility::SetVisible(
        0x4000u, 0x81u, 0x9000u, runtime);
    CheckEvents(runtime, 0x81u, true);
    Check(runtime.enabled == std::vector<bool>{true} &&
              runtime.calls.back() == "dialog" &&
              runtime.calls[runtime.calls.size() - 3u] ==
                  "destroy-args" &&
              runtime.reads == std::vector<std::uint32_t>{
                                   0x4024u, 0x40b8u, 0x7108u},
          "no arena starts new game, releases args and updates dialog");
}

void CheckArenaModes() {
    Runtime load;
    load.words[0x7108u] = 0x7200u;
    load.words[0x7234u] = 2u;
    pal4::SystemSettingVisibility::SetVisible(
        0x4000u, 0x81u, 0x9000u, load);
    Check(load.enabled == std::vector<bool>{true, false} &&
              load.calls[load.calls.size() - 4u] == "load-data" &&
              load.calls.back() == "dialog" &&
              load.reads.back() == 0x7234u,
          "arena mode two disables window and invokes load callback");

    Runtime start;
    start.words[0x7108u] = 0x7200u;
    start.words[0x7234u] = 3u;
    pal4::SystemSettingVisibility::SetVisible(
        0x4000u, 0x81u, 0x9000u, start);
    Check(start.enabled == std::vector<bool>{true} &&
              start.calls[start.calls.size() - 4u] == "start-game",
          "other arena mode keeps window enabled and starts new game");
}

void CheckCallbackFailure() {
    Runtime runtime;
    runtime.throw_callback = true;
    bool caught = false;
    try {
        pal4::SystemSettingVisibility::SetVisible(
            0x4000u, 0x81u, 0x9000u, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls.back() == "destroy-args" &&
              runtime.calls.size() >= 2u &&
              runtime.calls[runtime.calls.size() - 2u] == "start-game",
          "callback exception destroys event args and skips dialog update");
}

}  // namespace

int main() {
    CheckHidden();
    CheckShownWithoutArena();
    CheckArenaModes();
    CheckCallbackFailure();
}
