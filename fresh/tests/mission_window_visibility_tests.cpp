#include "pal4/mission_window_visibility.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IMissionWindowVisibilityRuntime {
    std::vector<std::string> calls;

    void SetParent(pal4::MissionWindowState&, std::uint8_t visible,
                   std::uint32_t parent) override {
        Check(parent == 0x9000u && visible == expected_visible,
              "parent and visibility forwarded");
        calls.emplace_back("parent");
    }
    void ProcessUiEvent(std::uint32_t event, std::uint32_t first,
                        std::uint32_t second) override {
        calls.emplace_back("event:" + std::to_string(event) + ":" +
                           std::to_string(first) + ":" +
                           std::to_string(second));
    }
    void SetVisible(std::uint32_t window, bool visible) override {
        if (window == 0x8400u) {
            Check(!visible, "mask hidden on open");
            calls.emplace_back("mask");
            return;
        }
        Check(window == 0x7000u || window == 0x7800u ||
                  window == 0x6c00u || window == 0x7400u,
              "character panel target");
        calls.emplace_back("visible:" + std::to_string(window) + ":" +
                           std::to_string(visible));
    }
    void SetAlwaysOnTop(std::uint32_t window, bool value) override {
        Check(window == 0x5400u && value, "mission list on top");
        calls.emplace_back("top");
    }
    void SetImage(std::uint32_t window, std::string_view image_set,
                  std::string_view image_name) override {
        Check(window == 0x7c00u && image_set == "zi0" &&
                  image_name == "renwu", "character image");
        calls.emplace_back("image");
    }
    void SetListHeaderByte(std::uint32_t list, std::uint32_t offset,
                           std::uint8_t value) override {
        Check(offset == 1239u && value == 0u, "list byte cleared");
        calls.emplace_back("header:" + std::to_string(list));
    }
    void SetSelected(std::uint32_t button, bool value) override {
        Check(!value, "button unselected");
        calls.emplace_back("selected:" + std::to_string(button));
    }
    void InsertActiveMember(pal4::MissionWindowState&,
                            std::uint32_t offset) override {
        calls.emplace_back("insert:" + std::to_string(offset));
    }
    void RemoveActiveMember(pal4::MissionWindowState&,
                            std::uint32_t offset) override {
        calls.emplace_back("remove:" + std::to_string(offset));
    }
    void PopulateMissionList(pal4::MissionWindowState&) override {
        calls.emplace_back("populate");
    }
    void UpdateSystemDialogVisibility(std::uint32_t key) override {
        Check(key == 6u, "tutorial dialog key");
        calls.emplace_back("dialog");
    }
    std::uint8_t expected_visible{};
};

pal4::MissionWindowState MakeState() {
    pal4::MissionWindowState state{};
    state.Write(0x24u, 0x2400u);
    state.Write(0x54u, 0x5400u);
    state.Write(0x58u, 0x5800u);
    state.Write(0x60u, 0x6000u);
    state.Write(0x64u, 0x6400u);
    state.Write(0x6cu, 0x6c00u);
    state.Write(0x70u, 0x7000u);
    state.Write(0x74u, 0x7400u);
    state.Write(0x78u, 0x7800u);
    state.Write(0x7cu, 0x7c00u);
    state.Write(0x84u, 0x8400u);
    return state;
}

void Test(std::uint8_t visible) {
    auto state = MakeState();
    Runtime runtime;
    runtime.expected_visible = visible;
    pal4::MissionWindowVisibility::Set(state, visible, 0x9000u, runtime);
    const auto event = [](std::uint32_t type, std::uint32_t first,
                          std::uint32_t second) {
        return "event:" + std::to_string(type) + ":" +
               std::to_string(first) + ":" + std::to_string(second);
    };
    std::vector<std::string> expected{
        "parent", event(0x8b1394u, visible, 0u),
        event(0x8b13a0u, visible, 0x2400u)};
    if (visible != 0u) {
        expected.push_back(event(0x8b13a4u, 12u, 0u));
        expected.push_back("mask");
        expected.push_back("top");
    }
    expected.insert(expected.end(), {
        "visible:28672:0", "visible:30720:0", "visible:27648:0",
        "visible:29696:0", "image", "header:22528", "header:21504",
        "selected:25600", "selected:24576"});
    if (visible != 0u) {
        expected.insert(expected.end(), {
            "insert:40", "insert:64", "populate", "dialog"});
    } else {
        expected.push_back("remove:40");
    }
    Check(runtime.calls == expected, "visibility operation sequence");
}

void TestCharacterPanel(bool visible) {
    auto state = MakeState();
    Runtime runtime;
    pal4::MissionWindowVisibility::SetCharacterPanel(state, visible, runtime);
    const auto suffix = visible ? "1" : "0";
    Check(runtime.calls == std::vector<std::string>{
              std::string("visible:28672:") + suffix,
              std::string("visible:30720:") + suffix,
              std::string("visible:27648:") + suffix,
              std::string("visible:29696:") + suffix,
              "image"}, "character panel targets and image order");
}

void TestMissionPanel(bool visible) {
    auto state = MakeState();
    Runtime runtime;
    pal4::MissionWindowVisibility::SetMissionPanel(state, visible, runtime);
    const auto enabled = visible ? "1" : "0";
    const auto disabled = visible ? "0" : "1";
    Check(runtime.calls == std::vector<std::string>{
              std::string("visible:28672:") + enabled,
              std::string("visible:30720:") + enabled,
              std::string("visible:27648:") + disabled,
              "visible:29696:1"},
          "mission panel picture, description, scenario and list order");
}

}  // namespace

int main() {
    Test(0u);
    Test(1u);
    Test(2u);
    TestCharacterPanel(false);
    TestCharacterPanel(true);
    TestMissionPanel(false);
    TestMissionPanel(true);
    std::cout << "Mission window visibility tests passed\n";
}
