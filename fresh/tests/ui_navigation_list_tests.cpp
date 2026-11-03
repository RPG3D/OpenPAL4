#include "pal4/ui_navigation_list.h"
#include "pal4/mission_window_input.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IUiNavigationRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::unordered_map<std::uint32_t, bool> disabled;
    std::unordered_map<std::uint32_t, bool> visible;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> deselected;
    std::vector<std::uint32_t> redrawn;
    std::uint32_t mouse_window = 0u;
    std::vector<std::uint32_t> allocations{0x5000u, 0x5010u};
    std::vector<std::string> subscriptions;
    std::vector<std::uint32_t> subscription_handlers;
    std::uint32_t hover_handler_array = 0u;
    std::uint32_t forwarded_slot = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.emplace_back("read_byte");
        return bytes[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.push_back("write_byte:" + std::to_string(address));
        bytes[address] = value;
    }
    void SetButtonSelected(std::uint32_t window,
                           bool selected) override {
        calls.emplace_back("button_selected");
        Check(!selected, "select-current first clears button selection");
        deselected.push_back(window);
    }
    void RequestRedraw(std::uint32_t window) override {
        calls.emplace_back("redraw");
        redrawn.push_back(window);
    }
    bool IsDisabled(std::uint32_t window) override {
        calls.emplace_back("disabled");
        return disabled[window];
    }
    bool IsVisible(std::uint32_t window) override {
        calls.emplace_back("visible");
        return visible[window];
    }
    std::uint32_t InvokeCycle(std::uint32_t list, std::uint32_t slot,
                              std::uint32_t previous) override {
        calls.push_back("cycle:" + std::to_string(slot));
        if (slot == 24u)
            return pal4::UiNavigationList::CycleNext(list, previous, *this);
        Check(slot == 28u, "previous virtual slot");
        return pal4::UiNavigationList::CyclePrevious(list, previous, *this);
    }
    void MouseAtCenter(std::uint32_t window) override {
        calls.emplace_back("mouse_center");
        mouse_window = window;
    }
    void AppendWindow(std::uint32_t list,
                      std::uint32_t window) override {
        calls.emplace_back("append_window");
        Check(list == 0x1004u && window == 0x3000u,
              "mouse subscription appends target window");
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.emplace_back("allocate");
        Check(bytes == 12u && !allocations.empty(),
              "callback record allocation");
        const auto address = allocations.front();
        allocations.erase(allocations.begin());
        return address;
    }
    std::uint32_t Subscribe(std::uint32_t window,
                            std::string_view event_name,
                            std::uint32_t handler) override {
        calls.emplace_back("subscribe");
        Check(window == 0x3000u, "subscribed window");
        subscriptions.emplace_back(event_name);
        subscription_handlers.push_back(handler);
        return 0x6000u + static_cast<std::uint32_t>(subscriptions.size());
    }
    std::uint32_t ReleaseSubscription(std::uint32_t reference) override {
        calls.emplace_back("release_subscription");
        Check(reference == 0x6000u + subscriptions.size(),
              "subscription reference release order");
        return reference + 0x100u;
    }
    std::uint32_t UiManager() override {
        calls.emplace_back("manager");
        return 0x7000u;
    }
    void NotifyHoverHandlers(std::uint32_t handler_array) override {
        calls.emplace_back("notify_hover");
        hover_handler_array = handler_array;
    }
    std::uint32_t InvokeList(std::uint32_t list,
                             std::uint32_t slot) override {
        calls.emplace_back("invoke_list");
        Check(list == 0x1000u, "forwarded navigation list");
        forwarded_slot = slot;
        return 0x87654321u;
    }
};

Runtime ThreeItems() {
    Runtime runtime;
    runtime.words[0x1008u] = 0x2000u;
    runtime.words[0x100cu] = 0x200cu;
    runtime.words[0x1014u] = 0u;
    runtime.words[0x2000u] = 0x3000u;
    runtime.words[0x2004u] = 0x3100u;
    runtime.words[0x2008u] = 0x3200u;
    for (const auto window : {0x3000u, 0x3100u, 0x3200u})
        runtime.visible[window] = true;
    return runtime;
}

void CheckConstruct() {
    Runtime runtime;
    Check(pal4::UiNavigationList::Construct(0x12345678u, runtime) ==
              0x12345678u, "constructor returns self");
    Check(runtime.bytes[0x1234567cu] == 0x56u &&
          runtime.words[0x12345678u] ==
              pal4::UiNavigationList::kVtable,
          "constructor vtable and high address byte");
    for (const auto offset : {8u, 12u, 16u, 20u})
        Check(runtime.words[0x12345678u + offset] == 0u,
              "constructor zero fields");
    Check(runtime.calls.back() == "write:305419916",
          "constructor writes selection index last");
}

void CheckSelection() {
    auto runtime = ThreeItems();
    runtime.words[0x1014u] = 1u;
    Check(pal4::UiNavigationList::SelectCurrent(0x1000u, runtime) == 1u,
          "selection always returns true");
    Check(runtime.deselected == std::vector<std::uint32_t>{
              0x3000u, 0x3100u, 0x3200u},
          "all buttons deselected first");
    Check(runtime.bytes[0x3000u + 1201u] == 0u &&
          runtime.bytes[0x3100u + 1201u] == 1u &&
          runtime.bytes[0x3200u + 1201u] == 0u,
          "only current window receives selected byte");
    Check(runtime.redrawn == std::vector<std::uint32_t>{
              0x3000u, 0x3100u, 0x3200u, 0x3100u},
          "selection redraws all and then current");

    runtime.redrawn.clear();
    Check(pal4::UiNavigationList::RedrawAll(0x1000u, runtime) == 1u &&
          runtime.redrawn == std::vector<std::uint32_t>{
              0x3000u, 0x3100u, 0x3200u},
          "redraw-all traverses every window");
    Check(runtime.bytes[0x3100u + 1201u] == 0u,
          "redraw-all clears current marker");

    auto empty = ThreeItems();
    empty.words[0x100cu] = empty.words[0x1008u];
    Check(pal4::UiNavigationList::SelectCurrent(0x1000u, empty) == 1u &&
          empty.redrawn.empty(), "allocated empty selection");
}

void CheckKeyInput() {
    auto runtime = ThreeItems();
    runtime.words[0x1014u] = 1u;
    runtime.words[0x4010u] = 203u;
    Check(pal4::UiNavigationList::HandleKey(0x1000u, 0x4000u,
                                              runtime) == 1u &&
          runtime.words[0x1014u] == 0u &&
          runtime.bytes[0x3000u + 1201u] == 1u,
          "left key cycles backward then selects");
    runtime.words[0x4010u] = 205u;
    Check(pal4::UiNavigationList::HandleKey(0x1000u, 0x4000u,
                                              runtime) == 1u &&
          runtime.words[0x1014u] == 1u &&
          runtime.bytes[0x3100u + 1201u] == 1u,
          "right key cycles forward then selects");
    runtime.words[0x4010u] = 28u;
    Check(pal4::UiNavigationList::HandleKey(0x1000u, 0x4000u,
                                              runtime) == 1u &&
          runtime.mouse_window == 0x3100u,
          "enter key sends centered mouse event");
    runtime.words[0x4010u] = 99u;
    Check(pal4::UiNavigationList::HandleKey(0x1000u, 0x4000u,
                                              runtime) == 0u,
          "unrecognized key falls through");
    runtime.words[0x100cu] = runtime.words[0x1008u];
    Check(pal4::UiNavigationList::HandleKey(0x1000u, 0x4000u,
                                              runtime) == 0u,
          "empty list ignores key");
}

void CheckCycles() {
    auto next = ThreeItems();
    next.disabled[0x3100u] = true;
    Check(pal4::UiNavigationList::CycleNext(0x1000u, 0u,
                                              next) == 2u &&
          next.words[0x1014u] == 2u,
          "next skips disabled item through virtual recursion");
    Check(pal4::UiNavigationList::CycleNext(0x1000u, 2u,
                                              next) == 0u &&
          next.words[0x1014u] == 0u,
          "next wraps to start");

    auto previous = ThreeItems();
    previous.bytes[0x3200u + 1202u] = 1u;
    Check(pal4::UiNavigationList::CyclePrevious(0x1000u, 0u,
                                                  previous) == 1u &&
          previous.words[0x1014u] == 1u,
          "previous wraps and skips flagged item");
    previous.visible[0x3000u] = false;
    Check(pal4::UiNavigationList::CyclePrevious(0x1000u, 1u,
                                                  previous) == 1u,
          "all unavailable alternatives stop at original index");
}

void CheckMouseSubscriptions() {
    Runtime runtime;
    Check(pal4::UiNavigationList::SetupMouseHandlers(
              0x1000u, 0x3000u, runtime) == 0x6102u,
          "second subscription cleanup result");
    Check(runtime.subscriptions == std::vector<std::string>{
              "MouseMove", "MouseButtonUp"} &&
          runtime.subscription_handlers == std::vector<std::uint32_t>{
              0x5000u, 0x5010u},
          "both mouse events subscribed in order");
    Check(runtime.words[0x5000u] == 0x844100u &&
          runtime.words[0x5004u] == 0x4af760u &&
          runtime.words[0x5008u] == 0x1000u &&
          runtime.words[0x5010u] == 0x844100u &&
          runtime.words[0x5014u] == 0x4af770u &&
          runtime.words[0x5018u] == 0x1000u,
          "callback record vtable, target and owner fields");

    Runtime failed;
    failed.allocations = {0u, 0u};
    (void)pal4::UiNavigationList::SetupMouseHandlers(
        0x1000u, 0x3000u, failed);
    Check(failed.subscription_handlers ==
              std::vector<std::uint32_t>{0u, 0u},
          "null callback allocations still subscribe");
}

void CheckHover() {
    auto notify = ThreeItems();
    notify.words[0x4008u] = 0x3100u;
    Check(pal4::UiNavigationList::OnHoveredAndNotify(
              0x1000u, 0x4000u, notify) == 1u,
          "hover notify result");
    Check(notify.redrawn == std::vector<std::uint32_t>{
              0x3000u, 0x3200u} &&
          notify.hover_handler_array == 0x7134u,
          "hover clears other windows and notifies manager +308");

    auto flagged = ThreeItems();
    flagged.words[0x4008u] = 0x3100u;
    flagged.bytes[0x3100u + 1202u] = 1u;
    Check(pal4::UiNavigationList::OnHoveredAndNotify(
              0x1000u, 0x4000u, flagged) == 1u &&
          flagged.redrawn.empty() && flagged.hover_handler_array == 0u,
          "flagged hover is ignored");

    auto tracked = ThreeItems();
    tracked.words[0x4008u] = 0x3200u;
    Check(pal4::UiNavigationList::OnHoveredAndTrack(
              0x1000u, 0x4000u, tracked) == 1u &&
          tracked.words[0x1014u] == 2u &&
          tracked.redrawn == std::vector<std::uint32_t>{
              0x3000u, 0x3100u},
          "tracking hover updates index and redraws other windows");
    tracked.redrawn.clear();
    tracked.disabled[0x3200u] = true;
    Check(pal4::UiNavigationList::OnHoveredAndTrack(
              0x1000u, 0x4000u, tracked) == 1u &&
          tracked.redrawn.empty(),
          "disabled tracking hover is ignored");
}

void CheckVerticalKeyAndForward() {
    auto runtime = ThreeItems();
    runtime.words[0x1014u] = 1u;
    runtime.words[0x4010u] = 200u;
    Check(pal4::UiNavigationList::HandleVerticalKey(
              0x1000u, 0x4000u, runtime) == 1u &&
          runtime.words[0x1014u] == 0u,
          "up key cycles previous");
    runtime.words[0x4010u] = 208u;
    Check(pal4::UiNavigationList::HandleVerticalKey(
              0x1000u, 0x4000u, runtime) == 1u &&
          runtime.words[0x1014u] == 1u,
          "down key cycles next");
    for (const auto key : {203u, 205u}) {
        runtime.words[0x4010u] = key;
        Check(pal4::UiNavigationList::HandleVerticalKey(
                  0x1000u, 0x4000u, runtime) == 0u,
              "vertical list rejects horizontal key");
    }
    runtime.words[0x4010u] = 28u;
    Check(pal4::UiNavigationList::HandleVerticalKey(
              0x1000u, 0x4000u, runtime) == 1u &&
          runtime.mouse_window == 0x3100u,
          "vertical list delegates enter key to base handler");
    Check(pal4::UiNavigationList::ForwardMouseMove(
              0x1000u, runtime) == 0x87654321u &&
          runtime.forwarded_slot == 32u,
          "mouse move forwards virtual slot +32");
    Check(pal4::UiNavigationList::ForwardMouseButtonUp(
              0x1000u, runtime) == 0x87654321u &&
          runtime.forwarded_slot == 36u,
          "mouse button up forwards virtual slot +36");
}

void CheckMissionTabNavigation() {
    struct Tabs final : pal4::IMissionWindowTabRuntime {
        std::vector<std::string> calls;
        void HideMissionWindow(pal4::MissionWindowState&) override {
            calls.emplace_back("hide");
        }
        void DispatchTabEvent(std::uint32_t key, std::uint32_t first,
                              std::uint32_t second) override {
            Check(key == 0x8a80acu && first == 1u && second == 0u,
                  "escape restores original system toolbar event");
            calls.emplace_back("toolbar");
        }
    } tabs;
    pal4::MissionWindowState state;
    auto runtime = ThreeItems();
    runtime.words[0x1014u] = 1u;
    runtime.words[0x4010u] = 28u;
    Check(pal4::MissionWindowInput::TabKey(state, 0x1000u, 0x4000u,
          runtime, tabs) == 1u && runtime.mouse_window == 0x3100u,
          "mission tab enter clicks current navigation item");
    runtime.words[0x4010u] = 200u;
    Check(pal4::MissionWindowInput::TabKey(state, 0x1000u, 0x4000u,
          runtime, tabs) == 1u && runtime.words[0x1014u] == 0u,
          "mission tab up reuses navigation cycle");
    runtime.words[0x4010u] = 208u;
    Check(pal4::MissionWindowInput::TabKey(state, 0x1000u, 0x4000u,
          runtime, tabs) == 1u && runtime.words[0x1014u] == 1u,
          "mission tab down reuses navigation cycle");
    for (const auto code : {203u, 205u, 2u, 3u}) {
        runtime.words[0x4010u] = code;
        Check(pal4::MissionWindowInput::TabKey(state, 0x1000u, 0x4000u,
              runtime, tabs) == 0u, "tab primary handler leaves other keys unhandled");
    }
    runtime.words[0x4010u] = 1u;
    Check(pal4::MissionWindowInput::TabKey(state, 0x1000u, 0x4000u,
          runtime, tabs) == 1u &&
          tabs.calls == std::vector<std::string>{"hide", "toolbar"},
          "mission tab escape hides before toolbar restoration");
}

}  // namespace

int main() {
    try {
        CheckConstruct();
        CheckSelection();
        CheckKeyInput();
        CheckCycles();
        CheckMouseSubscriptions();
        CheckHover();
        CheckVerticalKeyAndForward();
        CheckMissionTabNavigation();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
