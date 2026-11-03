#include "pal4/script_player_position.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::IScriptPlayerPositionRuntime {
public:
    pal4::PlayerPositionWaitState state{};
    std::vector<std::string> calls;
    bool actor_exists = true;
    bool current = true;
    bool active = true;
    bool allocation_fails = false;
    bool use_actor_name = false;
    bool use_npc_name = false;
    int npc_action_result = 0;
    int property_calls = 0;

    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager");
        return 0x4000u;
    }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::int32_t index) override {
        Check(manager == 0x4000u && index == 2, "player lookup");
        calls.emplace_back("indexed");
        return actor_exists ? 0x5000u : 0u;
    }
    void SetPosition(std::uint32_t actor,
                     const pal4::PlayerPosition& position) override {
        Check(actor == 0x5000u && position.x == 1.0f &&
                  position.y == 2.0f && position.z == 3.0f,
              "player vtable +60 position write");
        calls.emplace_back("position");
    }
    std::uint32_t CurrentPlayer(std::uint32_t manager) override {
        Check(manager == 0x4000u, "current player manager");
        calls.emplace_back("current");
        return current ? 0x5000u : 0x6000u;
    }
    std::string_view ActorName(std::uint32_t actor) override {
        Check(actor == 0x5000u, "current actor name receiver");
        calls.emplace_back("actor name");
        return "Hero";
    }
    std::uint32_t NpcManager() override {
        calls.emplace_back("npc manager");
        return 0x9000u;
    }
    std::uint32_t NpcByName(std::uint32_t manager,
                            std::string_view name) override {
        Check(manager == 0x9000u &&
                  (name == "NPC01" || name.empty()),
              "position NPC lookup");
        calls.emplace_back("npc lookup");
        return actor_exists ? 0x5000u : 0u;
    }
    void SetNpcPosition(std::uint32_t actor,
                        const pal4::PlayerPosition& position,
                        std::uint32_t mode) override {
        Check(actor == 0x5000u && position.x == 1.0f &&
                  position.y == 2.0f && position.z == 3.0f && mode == 0u,
              "NPC vtable +4 position and mode");
        calls.emplace_back("npc position");
    }
    std::int32_t SetNpcAction(std::uint32_t actor,
                              std::string_view action) override {
        Check(actor == 0x5000u &&
                  (action == "C01" || action == "Z01"),
              "NPC action receiver");
        calls.emplace_back("npc action:" + std::string(action));
        return action == "C01" ? npc_action_result : 1;
    }
    float MovementProperty(std::uint32_t actor) override {
        Check(actor == 0x5000u, "movement property actor");
        calls.emplace_back("property");
        ++property_calls;
        return 10.0f;
    }
    std::uint32_t MovementAdapter() override {
        calls.emplace_back("adapter");
        return 0x7000u;
    }
    void SelectAndUpdate(std::uint32_t adapter,
                         std::uint32_t movement,
                         float first, float second,
                         std::uint32_t mode) override {
        Check(adapter == 0x7000u && movement == 0x50acu &&
                  std::fabs(first - 3.5f) < 0.0001f &&
                  std::fabs(second - 25.3f) < 0.0001f && mode == 1u,
              "movement adapter update parameters");
        calls.emplace_back("select");
    }
    bool CheckActive(std::uint32_t adapter,
                     std::uint32_t movement,
                     float first, float second,
                     std::uint32_t mode) override {
        Check(adapter == 0x7000u && movement == 0x50acu &&
                  std::fabs(first - 3.5f) < 0.0001f &&
                  std::fabs(second - 25.3f) < 0.0001f && mode == 0u,
              "movement adapter active check parameters");
        calls.emplace_back("active");
        return active;
    }
    void UpdateFloorColor() override {
        calls.emplace_back("floor");
    }
    void UpdateGrid(std::uint32_t position,
                    std::uint32_t context) override {
        Check(position == 0x50acu && context == 0x50a0u,
              "grid position and context offsets");
        calls.emplace_back("grid");
    }
    void SetAction(std::uint32_t actor,
                   std::string_view action) override {
        Check(actor == 0x5000u && action == "C01", "idle action");
        calls.emplace_back("action");
    }
    pal4::PlayerPositionWaitState* Allocate(
        std::uint32_t size) override {
        Check(size == 0x20u, "position wait allocation size");
        calls.emplace_back("allocate");
        return allocation_fails ? nullptr : &state;
    }
    std::uint32_t ScriptExecutor() override {
        calls.emplace_back("executor");
        return 0x8000u;
    }
    void Construct(pal4::PlayerPositionWaitState& candidate,
                   std::uint32_t mode,
                   std::uint32_t queue) override {
        Check(&candidate == &state && mode == 0u && queue == 0x8054u,
              "nonblocking position wait constructor");
        calls.emplace_back("construct");
    }
    void AssignName(pal4::PlayerPositionWaitState& candidate,
                    std::string_view name) override {
        Check(&candidate == &state &&
                  name == (use_npc_name ? "NPC01-Player-Movment" :
                           use_actor_name ? "Hero-Player-Movment" :
                                            "2-Player-Movment") &&
                  state.vtable_address == 0x849544u &&
                  state.enabled == 1u && state.phase == 1u,
              "position wait vtable, state and formatted name");
        calls.emplace_back("name");
    }
    void AddHandler(std::uint32_t queue,
                    pal4::PlayerPositionWaitState* candidate) override {
        Check(queue == 0x8054u &&
                  candidate == (allocation_fails ? nullptr : &state),
              "position wait queue receives allocation result");
        calls.emplace_back("add");
    }
    std::uint32_t CleanupName() override {
        calls.emplace_back("cleanup");
        return 0x1234u;
    }
};

void PositionCases() {
    const pal4::PlayerPosition position{1.0f, 2.0f, 3.0f};
    Runtime runtime;
    pal4::ScriptPlayerPosition player(runtime);
    Check(player.Set(2, position) == 0x1234u &&
              runtime.property_calls == 2 &&
              runtime.calls == std::vector<std::string>{
                  "manager", "indexed", "position", "manager", "current",
                  "property", "property", "adapter", "select", "adapter",
                  "active", "floor", "grid", "action", "allocate",
                  "executor", "construct", "name", "executor", "add",
                  "cleanup"},
          "current player position, movement and handler call order");

    Runtime inactive;
    inactive.active = false;
    pal4::ScriptPlayerPosition inactive_player(inactive);
    Check(inactive_player.Set(2, position) == 0x1234u &&
              inactive.calls[10] == "active" &&
              inactive.calls[11] == "action",
          "inactive adapter skips floor and grid updates");

    Runtime other;
    other.current = false;
    pal4::ScriptPlayerPosition other_player(other);
    Check(other_player.Set(2, position) == 0x1234u &&
              other.property_calls == 0 &&
              other.calls == std::vector<std::string>{
                  "manager", "indexed", "position", "manager", "current",
                  "action", "allocate", "executor", "construct", "name",
                  "executor", "add", "cleanup"},
          "noncurrent player skips movement refresh");

    Runtime missing;
    missing.actor_exists = false;
    pal4::ScriptPlayerPosition missing_player(missing);
    Check(missing_player.Set(2, position) == 0u &&
              missing.calls == std::vector<std::string>{
                  "manager", "indexed"},
          "missing player returns without position or queue effects");

    Runtime failed;
    failed.allocation_fails = true;
    pal4::ScriptPlayerPosition failed_player(failed);
    Check(failed_player.Set(2, position) == 0x1234u &&
              failed.calls[14] == "allocate" &&
              failed.calls[15] == "executor" &&
              failed.calls[16] == "add" &&
              failed.calls[17] == "cleanup",
          "failed position handler allocation still queues null");

    Runtime current;
    current.use_actor_name = true;
    pal4::ScriptPlayerPosition current_player(current);
    Check(current_player.SetCurrent(position) == 0x1234u &&
              current.calls == std::vector<std::string>{
                  "manager", "current", "position",
                  "property", "property", "adapter", "select", "adapter",
                  "active", "floor", "grid", "action", "actor name", "allocate",
                  "executor", "construct", "name", "executor", "add",
                  "cleanup"},
          "current-player script route uses actor name and movement update");
}

void CompletedQueueCases() {
    class Queue final : public pal4::IScriptCompletedHandlerQueue {
    public:
        unsigned calls{};
        void AppendCompletedHandler(std::uint32_t vtable, std::int32_t marker,
                                     std::string_view name) override {
            Check(vtable == 0x849544u && marker == 1 && name == "2-Player-Movment",
                  "position forwards the original completed handler identity");
            ++calls;
        }
    } queue;
    for (unsigned mode = 0; mode != 3; ++mode) {
        Runtime runtime;
        runtime.actor_exists = mode != 1;
        runtime.allocation_fails = mode == 2;
        pal4::ScriptPlayerPosition player(runtime, &queue);
        (void)player.Set(2, {1.0f,2.0f,3.0f});
        Check(queue.calls == 1, "missing actors and failed allocations cannot emit completed handlers");
    }
}

void NpcCases() {
    const pal4::PlayerPosition position{1.0f, 2.0f, 3.0f};
    Runtime runtime;
    runtime.use_npc_name = true;
    pal4::ScriptPlayerPosition npc(runtime);
    Check(npc.SetNpc("NPC01", position) == 0x1234u &&
              runtime.calls == std::vector<std::string>{
                  "npc manager", "npc lookup", "npc position",
                  "npc action:C01", "npc action:Z01", "allocate",
                  "executor", "construct", "name", "executor", "add",
                  "cleanup"},
          "NPC position uses vtable +4, idle fallback and shared handler");

    Runtime missing;
    missing.actor_exists = false;
    pal4::ScriptPlayerPosition missing_npc(missing);
    Check(missing_npc.SetNpc("", position) == 0u &&
              missing.calls == std::vector<std::string>{
                  "npc manager", "npc lookup"},
          "missing NPC skips position, action and handler");

    Runtime no_handler;
    no_handler.use_npc_name = true;
    no_handler.allocation_fails = true;
    no_handler.npc_action_result = 1;
    pal4::ScriptPlayerPosition failed_npc(no_handler);
    Check(failed_npc.SetNpc("NPC01", position) == 0x1234u &&
              no_handler.calls == std::vector<std::string>{
                  "npc manager", "npc lookup", "npc position",
                  "npc action:C01", "allocate", "executor", "add",
                  "cleanup"},
          "failed NPC handler allocation still queues null");
}

}  // namespace

int main() {
    try {
        PositionCases();
        CompletedQueueCases();
        NpcCases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
