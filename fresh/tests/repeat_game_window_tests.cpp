#include "pal4/repeat_game_window.h"

#include <cstdint>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Subscription {
    std::uint32_t window;
    pal4::RepeatGameWindowEvent event;
    std::uint32_t callback;
};

struct Runtime final : pal4::IRepeatGameWindowRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {0x7024u, 0x8000u}};
    std::vector<std::string> calls;
    std::vector<Subscription> subscriptions;
    std::vector<std::uint32_t> released;
    std::string requested_name;
    std::uint32_t allocation_count = 0u;
    bool fail_allocation = false;
    bool fail_name = false;

    void LoadLayout(std::uint32_t object,
                    std::uint32_t layout_name) override {
        Check(object == 0x7000u && layout_name == 0x9000u,
              "layout arguments");
        calls.emplace_back("layout");
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    std::string_view WindowName(std::uint32_t object) override {
        Check(object == 0x7000u, "base window name object");
        calls.emplace_back("name");
        if (fail_name) throw std::runtime_error("name");
        return "RepeatGameWindow/";
    }
    std::uint32_t GetWindow(std::uint32_t object,
                            std::string_view name) override {
        Check(object == 0x7000u, "GetWindow object");
        calls.emplace_back("window");
        requested_name = name;
        return 0xa000u;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0xcu, "callback size");
        calls.emplace_back("allocate");
        ++allocation_count;
        return fail_allocation ? 0u : 0xb000u + allocation_count * 0x10u;
    }
    std::uint32_t Subscribe(std::uint32_t window,
                            pal4::RepeatGameWindowEvent event,
                            std::uint32_t callback) override {
        calls.emplace_back("subscribe");
        subscriptions.push_back({window, event, callback});
        return 0xc000u + static_cast<std::uint32_t>(subscriptions.size());
    }
    void ReleaseSubscription(std::uint32_t connection) override {
        calls.emplace_back("release");
        released.push_back(connection);
    }
};

void CheckConstruct() {
    Runtime runtime;
    Check(pal4::RepeatGameWindow::Construct(0x7000u, 0x9000u,
                                             runtime) == 0x7000u,
          "constructor return");
    Check(runtime.words.at(0x7000u) ==
              pal4::RepeatGameWindow::kVtable &&
              runtime.words.at(0x7034u) == 0u &&
              runtime.words.at(0x7028u) == 0xa000u &&
              runtime.words.at(0x702cu) == 0x41600000u &&
              runtime.words.at(0x7030u) == 0x43868000u &&
              runtime.requested_name == "RepeatGameWindow/ImgPointer",
          "frame fields and image pointer lookup");
    Check(runtime.words.at(0xb014u) == 0x4a21b0u &&
              runtime.words.at(0xb018u) == 0x7000u &&
              runtime.words.at(0xb010u) ==
                  pal4::RepeatGameWindow::kCallbackVtable &&
              runtime.words.at(0xb024u) == 0x4a2190u &&
              runtime.words.at(0xb028u) == 0x7000u &&
              runtime.words.at(0xb020u) ==
                  pal4::RepeatGameWindow::kCallbackVtable &&
              runtime.subscriptions.size() == 2u &&
              runtime.subscriptions[0].window == 0x8000u &&
              runtime.subscriptions[0].event ==
                  pal4::RepeatGameWindowEvent::KeyDown &&
              runtime.subscriptions[0].callback == 0xb010u &&
              runtime.subscriptions[1].event ==
                  pal4::RepeatGameWindowEvent::MouseClick &&
              runtime.subscriptions[1].callback == 0xb020u &&
              runtime.released ==
                  (std::vector<std::uint32_t>{0xc001u, 0xc002u}),
          "callbacks and subscription ownership");
    Check(runtime.calls == (std::vector<std::string>{
              "layout", "write", "write", "name", "window",
              "write", "write", "write", "allocate", "write",
              "write", "write", "read", "subscribe", "release",
              "allocate", "write", "write", "write", "read",
              "subscribe", "release"}),
          "constructor operation order");
}

void CheckNullCallbacksAndFailure() {
    Runtime empty;
    empty.fail_allocation = true;
    Check(pal4::RepeatGameWindow::Construct(0x7000u, 0x9000u,
                                             empty) == 0x7000u &&
              empty.subscriptions.size() == 2u &&
              empty.subscriptions[0].callback == 0u &&
              empty.subscriptions[1].callback == 0u &&
              empty.allocation_count == 2u,
          "null callback allocation still subscribes twice");
    Runtime failed;
    failed.fail_name = true;
    bool caught = false;
    try {
        (void)pal4::RepeatGameWindow::Construct(0x7000u, 0x9000u,
                                                 failed);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && failed.allocation_count == 0u &&
              failed.words.at(0x7034u) == 0u,
          "name failure stops before callback setup");
}

struct LifecycleRuntime final : pal4::IRepeatGameWindowLifecycleRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {0x7024u, 0x8000u}, {0x7028u, 0xa000u},
        {0x901cu, 0u}, {0x9010u, 28u}};
    std::unordered_map<std::uint32_t, float> floats{
        {0x702cu, 14.0f}, {0x7030u, 269.0f},
        {0x7034u, 0.0f}};
    std::vector<std::string> calls;
    std::uint32_t closed = 0u;
    std::uint32_t modal_window = 0u;
    std::uint32_t parent = 0u;
    bool parent_visible = false;
    bool hiding = false;
    bool modal = false;
    bool visible = false;
    float x = 0.0f;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read-word");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write-word");
        words[address] = value;
    }
    float ReadFloat(std::uint32_t address) override {
        calls.emplace_back("read-float");
        return floats.at(address);
    }
    void WriteFloat(std::uint32_t address, float value) override {
        calls.emplace_back("write-float");
        floats[address] = value;
    }
    std::uint32_t DestroyBase(std::uint32_t object) override {
        calls.emplace_back("destroy-base");
        Check(object == 0x7000u, "base destructor object");
        return 0x55u;
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x4000u;
    }
    void ProcessUiEvents(std::uint32_t manager,
                         bool is_hiding) override {
        calls.emplace_back("process-events");
        Check(manager == 0x4000u, "UI manager");
        hiding = is_hiding;
    }
    void SetParent(std::uint32_t object, bool is_visible,
                   std::uint32_t next_parent) override {
        calls.emplace_back("parent");
        Check(object == 0x7000u, "set parent object");
        parent_visible = is_visible;
        parent = next_parent;
    }
    void SetModal(std::uint32_t window,
                  bool is_modal) override {
        calls.emplace_back("modal");
        modal_window = window;
        modal = is_modal;
    }
    bool IsVisible(std::uint32_t object) override {
        calls.emplace_back("is-visible");
        Check(object == 0x7000u, "visibility object");
        return visible;
    }
    void SetXPosition(std::uint32_t window,
                      std::uint32_t metrics_mode, float value) override {
        calls.emplace_back("set-x");
        Check(window == 0xa000u && metrics_mode == 1u,
              "X position target and metrics mode");
        x = value;
    }
    float GetXPosition(std::uint32_t window,
                       std::uint32_t metrics_mode) override {
        calls.emplace_back("get-x");
        Check(window == 0xa000u && metrics_mode == 1u,
              "X query target and metrics mode");
        return x;
    }
    std::uint32_t Close(std::uint32_t object) override {
        calls.emplace_back("close");
        Check(object == 0x7000u, "close object");
        ++closed;
        return 0x77u;
    }
};

void CheckLifecycle() {
    LifecycleRuntime runtime;
    Check(pal4::RepeatGameWindow::Destroy(0x7000u, runtime) == 0x55u &&
              runtime.words.at(0x7000u) ==
                  pal4::RepeatGameWindow::kVtable &&
              runtime.calls == (std::vector<std::string>{
                  "write-word", "destroy-base"}),
          "destructor restores vtable before base destructor");
    runtime.calls.clear();
    pal4::RepeatGameWindow::SetVisible(0x7000u, true, 0x9000u, runtime);
    Check(!runtime.hiding && runtime.parent_visible &&
              runtime.parent == 0x9000u && runtime.modal &&
              runtime.modal_window == 0x8000u && runtime.x == 14.0f &&
              runtime.floats.at(0x7034u) == 0.0f &&
              runtime.calls.front() == "manager" &&
              runtime.calls[1] == "process-events" &&
              runtime.calls[2] == "parent" &&
              runtime.calls.back() == "modal",
          "show resets animation before setting modal state");
    runtime.calls.clear();
    pal4::RepeatGameWindow::SetVisible(0x7000u, false, 0x9999u, runtime);
    Check(runtime.hiding && !runtime.parent_visible &&
              !runtime.modal && runtime.parent == 0x9999u &&
              runtime.calls == (std::vector<std::string>{
                  "manager", "process-events", "parent", "read-word",
                  "modal"}),
          "hide skips animation and clears modal state");
}

void CheckAnimationAndRange() {
    LifecycleRuntime runtime;
    Check(pal4::RepeatGameWindow::UpdatePosition(
              0x7000u, 0.5, runtime) == 0u &&
              runtime.calls == (std::vector<std::string>{"is-visible"}),
          "hidden frame skips animation");
    runtime.calls.clear();
    runtime.visible = true;
    Check(pal4::RepeatGameWindow::UpdatePosition(
              0x7000u, 0.75, runtime) == 0x100u &&
              runtime.floats.at(0x7034u) == 0.5f &&
              runtime.x == 141.5f && runtime.closed == 0u,
          "visible animation uses two-thirds elapsed scale");
    Check(pal4::RepeatGameWindow::CheckPositionRange(0x7000u, runtime),
          "current X is within closed interval");
    for (const auto [x, inside] :
         {std::pair{132.99f, false}, std::pair{133.0f, true},
          std::pair{153.0f, true}, std::pair{153.01f, false},
          std::pair{std::nanf(""), false}}) {
        runtime.x = x;
        Check(pal4::RepeatGameWindow::CheckPositionRange(
                  0x7000u, runtime) == inside,
              "position range boundaries");
    }
    Check(pal4::RepeatGameWindow::SetPosition(
              0x7000u, 1.0f, runtime) == 0x77u &&
              runtime.closed == 1u && runtime.x == 269.0f &&
              runtime.floats.at(0x7034u) == 1.0f,
          "completed animation clamps and invokes close slot");
    Check(pal4::RepeatGameWindow::SetPosition(
              0x7000u, std::nanf(""), runtime) == 0x100u &&
              runtime.closed == 1u,
          "unordered x87 comparison retains continuation path");
}

void CheckInput() {
    LifecycleRuntime runtime;
    Check(pal4::RepeatGameWindow::OnMouseClick(
              0x7000u, 0x9000u, runtime) == 1u &&
              runtime.closed == 1u,
          "mouse event word at +0x1c closes on zero");
    runtime.words[0x901cu] = 1u;
    Check(pal4::RepeatGameWindow::OnMouseClick(
              0x7000u, 0x9000u, runtime) == 1u &&
              runtime.closed == 1u,
          "nonzero mouse event skips close");
    Check(pal4::RepeatGameWindow::OnKeyDown(
              0x7000u, 0x9000u, runtime) == 1u &&
              runtime.closed == 2u,
          "enter key closes dialog");
    runtime.words[0x9010u] = 27u;
    Check(pal4::RepeatGameWindow::OnKeyDown(
              0x7000u, 0x9000u, runtime) == 1u &&
              runtime.closed == 2u,
          "other keys leave dialog open");
}

}  // namespace

int main() {
    try {
        CheckConstruct();
        CheckNullCallbacksAndFailure();
        CheckLifecycle();
        CheckAnimationAndRange();
        CheckInput();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
