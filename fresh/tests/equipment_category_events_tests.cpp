#include "pal4/equipment_category_events.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::EquipmentCategoryEventsRuntimeStub {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t disabled_index{99u};
    bool base_result{};

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x5010u, "category event word offset");
        calls.emplace_back("event");
        return words[address];
    }
    std::uint32_t ControlWindow(pal4::EquipmentWindowState& state,
                                std::uint32_t index) override {
        calls.emplace_back("control:" + std::to_string(index));
        return state.Read(0x98u + 4u * index);
    }
    std::uint32_t NavigationWindow(pal4::EquipmentWindowState& state,std::uint32_t index) override {
        calls.emplace_back("navigation:"+std::to_string(index));
        return EquipmentCategoryEventsRuntimeStub::NavigationWindow(state,index);
    }
    bool IsDisabled(std::uint32_t window) override {
        calls.emplace_back("disabled:" + std::to_string(window));
        return window == 0x9800u + disabled_index;
    }
    void SetHoverFlag(std::uint32_t window, bool hovered) override {
        calls.emplace_back("hover:" + std::to_string(window) +
                           (hovered ? ":1" : ":0"));
    }
    void RequestRedraw(std::uint32_t window) override {
        calls.emplace_back("redraw:" + std::to_string(window));
    }
    void RemoveCategoryMember() override {
        calls.emplace_back("remove-category");
    }
    void RemovePrimaryMember() override {
        calls.emplace_back("remove-primary");
    }
    void InsertPrimaryMember() override {
        calls.emplace_back("insert-primary");
    }
    void SetEquipmentVisible(pal4::EquipmentWindowState&,
                             bool visible) override {
        Check(!visible, "close hides equipment controls");
        calls.emplace_back("hide-equipment");
    }
    void HandleMouseEvent(std::uint32_t window) override {
        calls.emplace_back("mouse:" + std::to_string(window));
    }
    bool HandleBaseKey(pal4::EquipmentCategoryMemberState&,
                       std::uint32_t event) override {
        Check(event == 0x5000u, "base key receives original event");
        calls.emplace_back("base-key");
        return base_result;
    }
};

pal4::EquipmentWindowState MakeState() {
    pal4::EquipmentWindowState state{};
    for (std::uint32_t index = 0u; index < 6u; ++index)
        state.Write(0x98u + 4u * index, 0x9800u + index);
    return state;
}

Runtime MakeRuntime(std::uint32_t code) {
    Runtime runtime;
    runtime.words[0x5010u] = code;
    return runtime;
}

bool Handle(pal4::EquipmentWindowState& state,
            pal4::EquipmentCategoryMemberState& member,
            Runtime& runtime) {
    return pal4::EquipmentCategoryEvents::Handle(
        state, member, 0x5000u, runtime);
}

void TestCloseAndLeaveFocus() {
    auto state = MakeState();
    pal4::EquipmentCategoryMemberState member{2u};
    auto runtime = MakeRuntime(1u);
    Check(Handle(state, member, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "event", "remove-category", "remove-primary",
                  "hide-equipment"},
          "close removes two members before hiding controls");

    runtime = MakeRuntime(15u);
    runtime.disabled_index = 2u;
    Check(Handle(state, member, runtime), "leave-focus returns true");
    Check(runtime.calls.front() == "event" &&
              runtime.calls.back() == "remove-category" &&
              runtime.calls.size() == 18u,
          "leave-focus scans six controls then removes category member");
    Check(runtime.calls[1] == "disabled:38912" &&
              runtime.calls[4] == "disabled:38913" &&
              runtime.calls[7] == "disabled:38914" &&
              runtime.calls[8] == "disabled:38915",
          "disabled control skips hover and redraw without stopping scan");
}

void TestHorizontalNavigation() {
    auto state = MakeState();
    for (const auto start : {3u, 4u, 5u}) {
        constexpr unsigned order[]{0u,1u,4u,3u,2u,5u};
        pal4::EquipmentCategoryMemberState member{start};
        auto runtime = MakeRuntime(203u);
        Check(Handle(state, member, runtime) &&
                  member.selected_index == start - 3u,
              "left maps second column to first");
        Check(runtime.calls == std::vector<std::string>{
                  "event", "navigation:" + std::to_string(start - 3u),
                  "disabled:" + std::to_string(0x9800u + order[start - 3u]),
                  "navigation:" + std::to_string(start),
                  "hover:" + std::to_string(0x9800u + order[start]) + ":0",
                  "redraw:" + std::to_string(0x9800u + order[start]),
                  "hover:" + std::to_string(0x9800u + order[start - 3u]) + ":1",
                  "redraw:" + std::to_string(0x9800u + order[start - 3u])},
              "left clears source before highlighting target");
    }
    for (const auto start : {0u, 1u, 2u}) {
        pal4::EquipmentCategoryMemberState member{start};
        auto runtime = MakeRuntime(205u);
        Check(Handle(state, member, runtime) &&
                  member.selected_index == start + 3u,
              "right maps first column to second");
    }

    pal4::EquipmentCategoryMemberState member{1u};
    auto runtime = MakeRuntime(203u);
    Check(Handle(state, member, runtime) &&
              runtime.calls == std::vector<std::string>{"event"},
          "left from first column does nothing");
    member.selected_index = 4u;
    runtime = MakeRuntime(205u);
    Check(Handle(state, member, runtime) &&
              runtime.calls == std::vector<std::string>{"event"},
          "right from second column does nothing");

    member.selected_index = 3u;
    runtime = MakeRuntime(203u);
    runtime.disabled_index = 0u;
    Check(Handle(state, member, runtime) && member.selected_index == 3u &&
              runtime.calls == std::vector<std::string>{
                  "event", "navigation:0", "disabled:38912"},
          "disabled destination leaves selected index and hover unchanged");
}

void TestEnterAndBaseFallback() {
    auto state = MakeState();
    pal4::EquipmentCategoryMemberState member{5u};
    auto runtime = MakeRuntime(28u);
    Check(Handle(state, member, runtime) &&
              member.selected_index == 0u &&
              runtime.calls[0] == "event" &&
              runtime.calls[1] == "control:5" &&
              runtime.calls[2] == "mouse:38917" &&
              runtime.calls.back() == "redraw:38912",
          "enter activates selected control then selects first enabled");
    runtime = MakeRuntime(99u);
    Check(!Handle(state, member, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "event", "base-key"},
          "unhandled key returns base handler result");
    runtime = MakeRuntime(99u);
    runtime.base_result = true;
    Check(Handle(state, member, runtime),
          "base handler true result propagates");
}

void TestSelectionAndClear() {
    auto state = MakeState();
    pal4::EquipmentCategoryMemberState member{4u};
    auto runtime = MakeRuntime(0u);
    runtime.disabled_index = 0u;
    Check(pal4::EquipmentCategoryEvents::SelectFirstEnabled(
              state, member, runtime) && member.selected_index == 1u,
          "selection picks first enabled control");
    Check(runtime.calls[0] == "disabled:38912" &&
              runtime.calls[1] == "disabled:38913" &&
              runtime.calls[2] == "hover:38913:0" &&
              runtime.calls[3] == "redraw:38913" &&
              runtime.calls[runtime.calls.size() - 3u] ==
                  "disabled:38913" &&
              runtime.calls[runtime.calls.size() - 2u] ==
                  "hover:38913:1" &&
              runtime.calls.back() == "redraw:38913",
          "selection clears all enabled controls before first rehighlight");

    runtime = MakeRuntime(0u);
    runtime.disabled_index = 99u;
    Check(pal4::EquipmentCategoryEvents::ClearHighlight(state, runtime) &&
              runtime.calls.size() == 18u &&
              runtime.calls[0] == "disabled:38912" &&
              runtime.calls[1] == "hover:38912:0" &&
              runtime.calls.back() == "redraw:38917",
          "clear-highlight visits all six controls in order");

    runtime = MakeRuntime(0u);
    runtime.disabled_index = 99u;
    for (std::uint32_t index = 0u; index < 6u; ++index)
        state.Write(0x98u + 4u * index, 0x9800u + 99u);
    Check(pal4::EquipmentCategoryEvents::SelectFirstEnabled(
              state, member, runtime) &&
              runtime.calls[runtime.calls.size() - 2u] ==
                  "remove-category" &&
              runtime.calls.back() == "insert-primary",
          "all disabled controls move focus back to primary member");
}

}  // namespace

int main() {
    TestCloseAndLeaveFocus();
    TestHorizontalNavigation();
    TestEnterAndBaseFallback();
    TestSelectionAndClear();
}
