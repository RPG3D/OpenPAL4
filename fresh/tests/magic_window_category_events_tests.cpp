#include "pal4/magic_window_category_events.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::MagicWindowCategoryEventsRuntimeStub {
    std::vector<std::string> calls;
    std::vector<std::uint32_t> removed;
    std::vector<std::uint32_t> inserted;
    std::vector<std::pair<std::uint32_t, bool>> selected;
    std::map<std::uint32_t, std::uint32_t> button_keys;
    std::int32_t display_index{-1};
    std::int32_t magic_detail_index{-1};
    std::int32_t stunt_detail_index{-1};
    std::uint32_t looked_up_key{};
    std::uint32_t selection_payload{};
    std::uint32_t selection_record{};

    std::uint32_t ButtonKey(std::uint32_t button) override {
        calls.push_back("button-key");
        return button_keys.at(button);
    }
    void RemoveActive(std::uint32_t member) override {
        removed.push_back(member);
        calls.push_back("remove:" + std::to_string(member));
    }
    void InsertActive(std::uint32_t member) override {
        inserted.push_back(member);
        calls.push_back("insert:" + std::to_string(member));
    }
    void SetSelected(std::uint32_t button, bool value) override {
        selected.emplace_back(button, value);
        calls.push_back("selected:" + std::to_string(button));
    }
    void UpdateCharacterDisplay(std::uint32_t object,
                                std::int32_t index) override {
        Check(object == 0x3000u, "display owner");
        display_index = index;
        calls.push_back("display");
    }
    void UpdateMagicDetails(std::uint32_t object,
                            std::int32_t index) override {
        Check(object == 0x3000u, "magic detail owner");
        magic_detail_index = index;
        calls.push_back("magic-details");
    }
    void UpdateStuntDetails(std::uint32_t object,
                            std::int32_t index) override {
        Check(object == 0x3000u, "stunt detail owner");
        stunt_detail_index = index;
        calls.push_back("stunt-details");
    }
    std::uint32_t FindStuntRecord(std::uint32_t key) override {
        looked_up_key = key;
        calls.push_back("find-stunt");
        return 0x8800u;
    }
    void HandleMagicSelection(std::uint32_t object,
                              std::uint32_t payload,
                              std::uint32_t record) override {
        Check(object == 0x3000u, "magic selection owner");
        selection_payload = payload;
        selection_record = record;
        calls.push_back("magic-selection");
    }
};

pal4::MagicWindowPanelState Panel() {
    pal4::MagicWindowPanelState panel;
    for (std::uint32_t offset = 0; offset < 0x170u; offset += 4u)
        panel.At(offset) = 0x1000u + offset;
    return panel;
}

void CheckSelect(Runtime& runtime, std::uint32_t category) {
    Check(runtime.removed == std::vector<std::uint32_t>{0x304cu, 0x3040u} &&
              runtime.inserted == std::vector<std::uint32_t>{
                  category == 0u ? 0x304cu : 0x3040u},
          "remove both active members and reinsert selected branch");
    Check(runtime.selected.size() == 7u && runtime.display_index == 2,
          "six deselections, selected category and character refresh");
    for (std::uint32_t index = 0; index < 6u; ++index)
        Check(runtime.selected[index] ==
                  std::pair<std::uint32_t, bool>{0x10a8u + 4u * index, false},
              "category deselection order");
    Check(runtime.selected.back() ==
              std::pair<std::uint32_t, bool>{0x10a8u + 4u * category, true} &&
              runtime.calls[runtime.calls.size() - 2] == "display" &&
              runtime.calls.back() ==
                  "insert:" + std::to_string(runtime.inserted.back()),
          "category selection, display and active insert order");
}

void TestStuntClickAndSelect() {
    auto panel = Panel();
    Runtime runtime;
    runtime.button_keys[0x5000u] = 3u;
    pal4::MagicWindowState state;
    state.selected_character_index = 2;
    Check(pal4::MagicWindowCategoryEvents::HandleStuntClick(
              0x3000u, 0x5000u, state, panel, runtime) &&
              state.category_position == 3u && runtime.calls.front() == "button-key",
          "stunt click stores button index before selection");
    CheckSelect(runtime, 3u);

    runtime = Runtime{};
    state.category_position = 0u;
    Check(pal4::MagicWindowCategoryEvents::SelectStunt(
              0x3000u, state, panel, runtime), "direct stunt selection return");
    CheckSelect(runtime, 0u);
}

void TestHoverForwarding() {
    Runtime runtime;
    runtime.button_keys[0x6000u] = 5u;
    Check(pal4::MagicWindowCategoryEvents::HandleMagicHover(
              0x3000u, 0x6000u, runtime) && runtime.magic_detail_index == 5 &&
              runtime.calls == std::vector<std::string>{"button-key", "magic-details"},
          "magic hover forwards button field directly");

    const auto panel = Panel();
    for (std::int32_t row = 0; row < 7; ++row) {
        runtime = Runtime{};
        Check(pal4::MagicWindowCategoryEvents::HandleStuntHover(
                  0x3000u, panel.At(0x108u + 4u * row), panel, runtime) &&
                  runtime.stunt_detail_index == row &&
                  runtime.calls == std::vector<std::string>{"stunt-details"},
              "stunt hover locates each of seven row windows");
    }
    runtime = Runtime{};
    Check(pal4::MagicWindowCategoryEvents::HandleStuntHover(
              0x3000u, 0xdeadbeefu, panel, runtime) && runtime.calls.empty(),
          "unmatched stunt hover returns without detail update");
}

void TestMagicButtonClick() {
    Runtime runtime;
    runtime.button_keys[0x6000u] = 71u;
    Check(pal4::MagicWindowCategoryEvents::HandleMagicButtonClick(
              0x3000u, 0x7000u, 0x6000u, 1u, runtime) &&
              runtime.calls.empty(), "nonzero event guard suppresses click");
    Check(pal4::MagicWindowCategoryEvents::HandleMagicButtonClick(
              0x3000u, 0x7000u, 0x6000u, 0u, runtime) &&
              runtime.looked_up_key == 71u && runtime.selection_payload == 0x700cu &&
              runtime.selection_record == 0x8800u &&
              runtime.calls == std::vector<std::string>{
                  "button-key", "find-stunt", "magic-selection"},
          "unguarded click forwards button key, GameDB record and event payload");
}

}  // namespace

int main() {
    TestStuntClickAndSelect();
    TestHoverForwarding();
    TestMagicButtonClick();
}
