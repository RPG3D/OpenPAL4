#include "pal4/script_world_map_wait.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::IScriptWorldMapWaitRuntime {
public:
    pal4::TutorialWaitState state{};
    std::vector<std::string> calls;
    bool allocation_fails = false;
    bool state_throws = false;
    std::uint32_t frame = 0x7000u;

    std::uint32_t UiFrameManager() override {
        calls.emplace_back("ui manager");
        return 0x4000u;
    }
    void ProcessWorldMapEvent(std::uint32_t manager,
                              std::uint32_t first,
                              std::uint32_t second) override {
        Check(manager == 0x4000u && first == 1u && second == 0u,
              "world map event payload");
        calls.emplace_back("event");
    }
    std::uint32_t FindFrame(std::uint32_t manager,
                            std::string_view name) override {
        Check(manager == 0x4000u && name == "WorldMap",
              "world map frame lookup");
        calls.emplace_back("find frame");
        return frame;
    }
    pal4::TutorialWaitState* AllocateWait(
        std::uint32_t bytes) override {
        Check(bytes == 0x24u, "world map wait size");
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
        Check(&handler == &state && mode == 1u && queue == 0x6054u,
              "world map wait constructor");
        calls.emplace_back("construct");
    }
    void FlushWait(pal4::TutorialWaitState& handler) override {
        Check(&handler == &state &&
                  handler.vtable_address == 0x84966cu &&
                  handler.frame == frame,
              "world map wait state before flush");
        calls.emplace_back("flush");
    }
    void AddHandler(std::uint32_t queue,
                    pal4::TutorialWaitState* handler) override {
        Check(queue == 0x6054u &&
                  handler == (allocation_fails ? nullptr : &state),
              "world map wait queue");
        calls.emplace_back("add handler");
    }
    std::uint32_t CreateDocument() override {
        calls.emplace_back("create document");
        return 0x9000u;
    }
    void EnterState(std::uint32_t state_id,
                    std::uint32_t document) override {
        Check(state_id == 11u && document == 0x9000u,
              "world map state transition");
        calls.emplace_back("enter state");
        if (state_throws) throw std::runtime_error("state failure");
    }
    std::uint32_t DestroyDocument(std::uint32_t document) override {
        Check(document == 0x9000u, "world map document cleanup");
        calls.emplace_back("destroy document");
        return 0x1234u;
    }
};

void Cases() {
    Runtime runtime;
    pal4::ScriptWorldMapWait world_map(runtime);
    Check(world_map.Show() == 0x1234u &&
              runtime.calls == std::vector<std::string>{
                  "ui manager", "event", "ui manager", "find frame",
                  "allocate", "executor", "construct", "flush",
                  "executor", "add handler", "create document",
                  "enter state", "destroy document"},
          "world map event and wait precede state eleven");

    Runtime failed;
    failed.allocation_fails = true;
    pal4::ScriptWorldMapWait failed_map(failed);
    Check(failed_map.Show() == 0x1234u &&
              failed.calls == std::vector<std::string>{
                  "ui manager", "event", "ui manager", "find frame",
                  "allocate", "executor", "add handler",
                  "create document", "enter state", "destroy document"},
          "allocation failure queues null before state transition");

    Runtime missing;
    missing.frame = 0u;
    pal4::ScriptWorldMapWait missing_map(missing);
    Check(missing_map.Show() == 0x1234u && missing.state.frame == 0u,
          "missing world map frame is retained in wait handler");

    Runtime throws;
    throws.state_throws = true;
    pal4::ScriptWorldMapWait throws_map(throws);
    bool caught = false;
    try {
        static_cast<void>(throws_map.Show());
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throws.calls.back() == "destroy document",
          "state transition failure destroys temporary XML document");
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
