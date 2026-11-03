#include "pal4/script_poetry_wait.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::IScriptPoetryWaitRuntime {
public:
    pal4::TutorialWaitState state{};
    std::vector<std::string> calls;
    bool allocation_fails = false;
    std::uint32_t frame = 0x7000u;

    std::uint32_t UiFrameManager() override {
        calls.emplace_back("ui manager");
        return 0x4000u;
    }
    void CreateAndHidePoetryWindow(std::uint32_t manager,
                                   std::uint32_t poetry_id) override {
        Check(manager == 0x4000u && poetry_id == 9u,
              "poetry window operands");
        calls.emplace_back("create/hide");
    }
    std::uint32_t PoetryWindow(std::uint32_t manager) override {
        Check(manager == 0x4000u, "poetry window manager");
        calls.emplace_back("poetry window");
        return frame;
    }
    pal4::TutorialWaitState* AllocateWait(
        std::uint32_t bytes) override {
        Check(bytes == 0x24u, "poetry wait size");
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
        Check(&handler == &state && mode == 2u &&
                  queue == 0x6054u, "poetry wait constructor");
        calls.emplace_back("construct");
    }
    void FlushWait(pal4::TutorialWaitState& handler) override {
        Check(&handler == &state &&
                  handler.vtable_address == 0x84966cu &&
                  handler.frame == frame,
              "poetry wait state before flush");
        calls.emplace_back("flush");
    }
    std::uint32_t AddHandler(
        std::uint32_t queue, pal4::TutorialWaitState* handler) override {
        Check(queue == 0x6054u &&
                  handler == (allocation_fails ? nullptr : &state),
              "poetry wait queue");
        calls.emplace_back("add handler");
        return 0x1234u;
    }
};

void Cases() {
    Runtime no_wait;
    pal4::ScriptPoetryWait direct(no_wait);
    Check(direct.Show(9u, 0u) == 0x7000u &&
              no_wait.calls == std::vector<std::string>{
                  "ui manager", "create/hide", "ui manager",
                  "poetry window"},
          "no-wait poetry returns frame without queue");

    Runtime low_byte_zero;
    pal4::ScriptPoetryWait low_byte_direct(low_byte_zero);
    Check(low_byte_direct.Show(9u, 0x100u) == 0x7000u &&
              low_byte_zero.calls == no_wait.calls,
          "wait gate uses only low byte");

    Runtime wait;
    pal4::ScriptPoetryWait queued(wait);
    Check(queued.Show(9u, 2u) == 0x1234u &&
              wait.calls == std::vector<std::string>{
                  "ui manager", "create/hide", "ui manager",
                  "poetry window", "allocate", "executor",
                  "construct", "flush", "executor", "add handler"},
          "poetry wait queues frame with supplied mode");

    Runtime failed;
    failed.allocation_fails = true;
    pal4::ScriptPoetryWait failed_wait(failed);
    Check(failed_wait.Show(9u, 2u) == 0x1234u &&
              failed.calls == std::vector<std::string>{
                  "ui manager", "create/hide", "ui manager",
                  "poetry window", "allocate", "executor",
                  "add handler"},
          "failed wait allocation still queues null");
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
