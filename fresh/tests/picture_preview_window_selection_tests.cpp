#include "pal4/picture_preview_window_selection.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::PicturePreviewWindowSelectionRuntimeStub {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    bool has_task{true};
    std::uint8_t dispatched{7u};

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void UpdateStaticInfo(pal4::PicturePreviewWindowState&,
                          std::uint32_t selected) override {
        Check(selected == 0x2100u, "move event forwards image");
        calls.emplace_back("static-info");
    }
    void HandleItemClick(pal4::PicturePreviewWindowState&,
                         std::uint32_t selected) override {
        Check(selected == 0x2100u, "click event forwards image");
        calls.emplace_back("item-click");
    }
    bool HasTriggerTask(std::uint32_t index) override {
        Check(index == 3u, "task lookup uses image +0x118");
        calls.emplace_back("has-task");
        return has_task;
    }
    std::uint32_t TriggerTask(std::uint32_t index) override {
        Check(index == 3u, "task lookup repeated after predicate");
        calls.emplace_back("task");
        return 0x4300u;
    }
    void Hide(pal4::PicturePreviewWindowState&,
              std::uint32_t first, std::uint32_t second) override {
        Check(first == 0u && second == 0u, "preview hidden before dispatch");
        calls.emplace_back("hide");
    }
    std::uint8_t DispatchPictureView(std::uint32_t first,
                                     std::uint32_t task) override {
        Check(first == 1u && task == 0x4300u,
              "picture view receives task and event word one");
        calls.emplace_back("dispatch");
        return dispatched;
    }
};

void CheckMouseEvents() {
    pal4::PicturePreviewWindowState state{};
    Runtime runtime;
    runtime.words[0x5008u] = 0x2100u;
    runtime.words[0x501cu] = 0u;
    Check(pal4::PicturePreviewWindowSelection::OnMouseMove(
              state, 0x5000u, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{"read", "static-info"},
          "move updates static info and consumes event");
    runtime.calls.clear();
    Check(pal4::PicturePreviewWindowSelection::OnMouseClick(
              state, 0x5000u, runtime) == 1u &&
              runtime.calls ==
                  std::vector<std::string>{"read", "read", "item-click"},
          "primary click invokes item handler");
    runtime.words[0x501cu] = 1u;
    runtime.calls.clear();
    Check(pal4::PicturePreviewWindowSelection::OnMouseClick(
              state, 0x5000u, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{"read"},
          "other mouse buttons are consumed without selection");
}

void CheckItemClick() {
    pal4::PicturePreviewWindowState state{};
    Runtime runtime;
    runtime.words[0x2218u] = 3u;
    Check(pal4::PicturePreviewWindowSelection::HandleItemClick(
              state, 0x2100u, runtime) == 7u &&
              runtime.calls == std::vector<std::string>{
                  "read", "has-task", "task", "hide", "dispatch"},
          "resolved picture dispatches after hiding preview");
    runtime.calls.clear();
    runtime.has_task = false;
    Check(pal4::PicturePreviewWindowSelection::HandleItemClick(
              state, 0x2100u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"read", "has-task"},
          "missing task leaves preview visible");
    runtime.calls.clear();
    runtime.has_task = true;
    runtime.dispatched = 0u;
    Check(pal4::PicturePreviewWindowSelection::HandleItemClick(
              state, 0x2100u, runtime) == 0u &&
              runtime.calls.back() == "dispatch",
          "dispatch return propagates");
    runtime.calls.clear();
    runtime.words.erase(0x2218u);
    try {
        (void)pal4::PicturePreviewWindowSelection::HandleItemClick(
            state, 0x2100u, runtime);
        Check(false, "failed field read must propagate");
    } catch (const std::out_of_range&) {
        Check(runtime.calls == std::vector<std::string>{"read"},
              "field failure has no side effects");
    }
}

}  // namespace

int main() {
    CheckMouseEvents();
    CheckItemClick();
    std::cout << "Picture preview window selection tests passed\n";
}
