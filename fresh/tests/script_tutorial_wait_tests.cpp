#include "pal4/script_tutorial_wait.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::IScriptTutorialWaitRuntime {
public:
    pal4::TutorialWaitState state{};
    std::vector<std::string> calls;
    std::uint32_t frame = 0x7000u;
    bool visible = true;
    bool allocation_fails = false;

    std::uint32_t UiFrameManager() override {
        calls.emplace_back("ui manager");
        return 0x4000u;
    }
    void ShowTutorial(std::uint32_t manager,
                      std::uint32_t key) override {
        Check(manager == 0x4000u && key == 1001u,
              "tutorial key forwarding");
        calls.emplace_back("show");
    }
    std::uint32_t FindFrame(std::uint32_t manager,
                            std::string_view name) override {
        Check(manager == 0x4000u && name == "TutorialWindow",
              "tutorial frame lookup");
        calls.emplace_back("find frame");
        return frame;
    }
    pal4::TutorialWaitState* AllocateWait(
        std::uint32_t bytes) override {
        Check(bytes == 0x24u, "tutorial wait allocation size");
        calls.emplace_back("allocate");
        return allocation_fails ? nullptr : &state;
    }
    std::uint32_t ScriptExecutor() override {
        calls.emplace_back("executor");
        return 0x6000u;
    }
    void ConstructWait(pal4::TutorialWaitState& handler,
                       std::uint8_t mode,
                       std::uint32_t queue) override {
        Check(&handler == &state && mode == 1u &&
                  queue == 0x6054u, "tutorial wait constructor");
        calls.emplace_back("construct");
    }
    void FlushWait(pal4::TutorialWaitState& handler) override {
        Check(&handler == &state &&
                  handler.vtable_address == 0x84966cu &&
                  handler.frame == frame,
              "tutorial wait state before flush");
        calls.emplace_back("flush");
    }
    std::uint32_t AddHandler(
        std::uint32_t queue, pal4::TutorialWaitState* handler) override {
        Check(queue == 0x6054u &&
                  handler == (allocation_fails ? nullptr : &state),
              "tutorial wait queue");
        calls.emplace_back("add handler");
        return 0x1234u;
    }
    bool IsVisible(std::uint32_t handle) override {
        Check(handle == frame && handle != 0u,
              "tutorial visibility receiver");
        calls.emplace_back("visible");
        return visible;
    }
    void CloseWait(pal4::TutorialWaitState& handler) override {
        Check(&handler == &state, "tutorial close receiver");
        calls.emplace_back("close");
    }
};

void Cases() {
    Runtime runtime;
    pal4::ScriptTutorialWait tutorial(runtime);
    Check(tutorial.Show(1001u) == 0x1234u &&
              runtime.calls == std::vector<std::string>{
                  "ui manager", "show", "ui manager", "find frame",
                  "allocate", "executor", "construct", "flush",
                  "executor", "add handler"},
          "tutorial shows before frame lookup and queues wait");

    runtime.calls.clear();
    tutorial.Update(runtime.state);
    Check(runtime.calls == std::vector<std::string>{"visible"},
          "visible tutorial remains pending");
    runtime.visible = false;
    runtime.calls.clear();
    tutorial.Update(runtime.state);
    Check(runtime.calls == std::vector<std::string>{
              "visible", "close"},
          "hidden tutorial closes wait");
    runtime.state.frame = 0u;
    runtime.calls.clear();
    tutorial.Update(runtime.state);
    Check(runtime.calls == std::vector<std::string>{"close"},
          "null frame closes without visibility query");

    Runtime missing;
    missing.frame = 0u;
    pal4::ScriptTutorialWait missing_tutorial(missing);
    Check(missing_tutorial.Show(1001u) == 0x1234u &&
              missing.state.frame == 0u,
          "null frame is stored in wait handler");

    Runtime failed;
    failed.allocation_fails = true;
    pal4::ScriptTutorialWait failed_tutorial(failed);
    Check(failed_tutorial.Show(1001u) == 0x1234u &&
              failed.calls == std::vector<std::string>{
                  "ui manager", "show", "ui manager", "find frame",
                  "allocate", "executor", "add handler"},
          "failed allocation still queues null");
}

}  // namespace

int main() {
    try {
        Cases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
