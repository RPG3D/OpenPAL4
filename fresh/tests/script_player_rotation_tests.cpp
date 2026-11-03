#include "pal4/script_player_rotation.h"

#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::IScriptPlayerRotationRuntime {
public:
    pal4::PlayerRotationWaitState state{};
    std::vector<std::string> calls;
    float expected_angle = 0.0f;
    bool actor_exists = true;
    bool allocation_fails = false;
    bool text_allocation_fails = false;
    bool use_actor_name = false;
    bool use_npc_name = false;

    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager");
        return 0x4000u;
    }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::int32_t index) override {
        Check(manager == 0x4000u && index == 2, "indexed lookup");
        calls.emplace_back("indexed");
        return actor_exists ? 0x5000u : 0u;
    }
    std::uint32_t CurrentPlayer(std::uint32_t manager) override {
        Check(manager == 0x4000u, "current lookup manager");
        calls.emplace_back("current");
        return actor_exists ? 0x5000u : 0u;
    }
    std::uint32_t NpcManager() override {
        calls.emplace_back("npc manager");
        return 0x7000u;
    }
    std::uint32_t NpcByName(std::uint32_t manager,
                            std::string_view name) override {
        Check(manager == 0x7000u &&
                  (name == "NPC01" || name.empty()),
              "NPC rotation name lookup");
        calls.emplace_back("npc lookup");
        return actor_exists ? 0x5000u : 0u;
    }
    std::string_view ActorName(std::uint32_t actor) override {
        Check(actor == 0x5000u, "actor name receiver");
        calls.emplace_back("actor name");
        return "Hero";
    }
    float ReadX(std::uint32_t actor) override {
        Check(actor == 0x5000u, "rotation X receiver");
        calls.emplace_back("read x");
        return 12.0f;
    }
    float ReadZ(std::uint32_t actor) override {
        Check(actor == 0x5000u, "rotation Z receiver");
        calls.emplace_back("read z");
        return 34.0f;
    }
    void SetRotation(std::uint32_t actor,
                     const pal4::PlayerRotation& rotation,
                     std::uint32_t mode) override {
        Check(actor == 0x5000u && rotation.x == 12.0f &&
                  rotation.yaw == expected_angle && rotation.z == 34.0f &&
                  mode == 0u,
              "vtable +8 rotation vector preserves X/Z");
        calls.emplace_back("rotation");
    }
    pal4::PlayerRotationWaitState* Allocate(
        std::uint32_t size) override {
        Check(size == 0x20u, "rotation wait allocation size");
        calls.emplace_back("allocate");
        return allocation_fails ? nullptr : &state;
    }
    std::uint32_t ScriptExecutor() override {
        calls.emplace_back("executor");
        return 0x6000u;
    }
    void Construct(pal4::PlayerRotationWaitState& candidate,
                   std::uint32_t mode,
                   std::uint32_t queue) override {
        Check(&candidate == &state && mode == 0u && queue == 0x6054u,
              "rotation wait constructor");
        calls.emplace_back("construct");
    }
    bool AllocateText(pal4::PlayerRotationWaitState& candidate,
                      std::uint32_t size) override {
        Check(&candidate == &state &&
                  size == (use_npc_name ? 20u :
                           use_actor_name ? 19u : 16u) &&
                  state.phase == 2u && state.vtable_address == 0x849550u &&
                  state.enabled == 1u,
              "rotation handler state and text length");
        calls.emplace_back("allocate text");
        return !text_allocation_fails;
    }
    void AssignName(pal4::PlayerRotationWaitState& candidate,
                    std::string_view name) override {
        Check(&candidate == &state &&
                  name == (use_npc_name ? "NPC01-Player-Movment" :
                           use_actor_name ? "Hero-Player-Movment" :
                                            "2-Player-Movment"),
              "rotation wait formatted name");
        calls.emplace_back("assign name");
    }
    void Close(pal4::PlayerRotationWaitState& candidate) override {
        Check(&candidate == &state, "close rotation wait receiver");
        calls.emplace_back("close");
    }
    void AddHandler(std::uint32_t queue,
                    pal4::PlayerRotationWaitState* candidate) override {
        Check(queue == 0x6054u &&
                  candidate == (allocation_fails ? nullptr : &state),
              "rotation handler queue");
        calls.emplace_back("add");
    }
    std::uint32_t CleanupName() override {
        calls.emplace_back("cleanup");
        return 0x1234u;
    }
};

void DirectionCases() {
    constexpr std::array<std::pair<std::string_view, float>, 10> cases{{
        {"N", 0.0f}, {"S", 180.0f}, {"W", 90.0f},
        {"E", 270.0f}, {"NW", 45.0f}, {"NE", 315.0f},
        {"SE", 225.0f}, {"SW", 135.0f},
        {"ne", 0.0f}, {"unknown", 0.0f}}};
    for (const auto& [direction, angle] : cases) {
        Runtime runtime;
        runtime.expected_angle = angle;
        pal4::ScriptPlayerRotation player(runtime);
        Check(player.SetDirection(2, direction) == 0x1234u &&
                  runtime.calls == std::vector<std::string>{
                      "manager", "indexed", "read x", "read z",
                      "rotation", "allocate", "executor", "construct",
                      "allocate text", "assign name", "close", "executor",
                      "add", "cleanup"},
              "direction mapping and handler order");
    }
}

void AngleCases() {
    Runtime angle;
    angle.expected_angle = 123.25f;
    pal4::ScriptPlayerRotation indexed(angle);
    Check(indexed.SetAngle(2, 123.25f) == 0x1234u &&
              angle.calls[4] == "rotation",
          "indexed numeric angle");

    Runtime current;
    current.expected_angle = 270.5f;
    current.use_actor_name = true;
    pal4::ScriptPlayerRotation current_player(current);
    Check(current_player.SetCurrentAngle(270.5f) == 0x1234u &&
              current.calls == std::vector<std::string>{
                  "manager", "current", "read x", "read z", "rotation",
                  "actor name", "allocate", "executor", "construct",
                  "allocate text", "assign name", "close", "executor",
                  "add", "cleanup"},
          "current numeric angle uses actor name after rotation");

    Runtime no_text;
    no_text.text_allocation_fails = true;
    pal4::ScriptPlayerRotation no_text_player(no_text);
    Check(no_text_player.SetAngle(2, 0.0f) == 0x1234u &&
              no_text.calls[9] == "close" &&
              no_text.calls[10] == "executor",
          "failed text allocation still closes and queues handler");

    Runtime no_handler;
    no_handler.allocation_fails = true;
    pal4::ScriptPlayerRotation no_handler_player(no_handler);
    Check(no_handler_player.SetAngle(2, 0.0f) == 0x1234u &&
              no_handler.calls == std::vector<std::string>{
                  "manager", "indexed", "read x", "read z", "rotation",
                  "allocate", "executor", "add", "cleanup"},
          "failed wait allocation still queues null");

    Runtime missing;
    missing.actor_exists = false;
    pal4::ScriptPlayerRotation missing_player(missing);
    Check(missing_player.SetCurrentAngle(1.0f) == 0u &&
              missing.calls == std::vector<std::string>{
                  "manager", "current"},
          "missing current actor skips rotation and queue");
}

void CompletedQueueCases() {
    class Queue final : public pal4::IScriptCompletedHandlerQueue {
    public:
        unsigned calls{};
        void AppendCompletedHandler(std::uint32_t vtable, std::int32_t marker,
                                     std::string_view name) override {
            Check(vtable == 0x849550u && marker == 2 && name == "2-Player-Movment",
                  "rotation forwards the original completed handler identity");
            ++calls;
        }
    } queue;
    for (unsigned mode = 0; mode != 3; ++mode) {
        Runtime runtime;
        runtime.actor_exists = mode != 1;
        runtime.allocation_fails = mode == 2;
        pal4::ScriptPlayerRotation player(runtime, &queue);
        (void)player.SetAngle(2, 0.0f);
        Check(queue.calls == 1, "missing actors and failed allocations cannot emit completed rotations");
    }
}

void NpcCases() {
    constexpr std::array<std::pair<std::string_view, float>, 10> cases{{
        {"N", 0.0f}, {"S", 180.0f}, {"W", 90.0f},
        {"E", 270.0f}, {"WN", 45.0f}, {"EN", 315.0f},
        {"ES", 225.0f}, {"WS", 135.0f},
        {"NW", 0.0f}, {"en", 0.0f}}};
    for (const auto& [direction, angle] : cases) {
        Runtime runtime;
        runtime.use_npc_name = true;
        runtime.expected_angle = angle;
        pal4::ScriptPlayerRotation npc(runtime);
        Check(npc.SetNpcDirection("NPC01", direction) == 0x1234u &&
                  runtime.calls == std::vector<std::string>{
                      "npc manager", "npc lookup", "read x", "read z",
                      "rotation", "allocate", "executor", "construct",
                      "allocate text", "assign name", "close", "executor",
                      "add", "cleanup"},
              "NPC direction mapping and shared rotation handler");
    }

    Runtime missing;
    missing.actor_exists = false;
    pal4::ScriptPlayerRotation npc(missing);
    Check(npc.SetNpcDirection("", "WN") == 0u &&
              missing.calls == std::vector<std::string>{
                  "npc manager", "npc lookup"},
          "missing NPC skips rotation and handler");

    Runtime numeric;
    numeric.use_npc_name = true;
    numeric.expected_angle = 123.25f;
    pal4::ScriptPlayerRotation numeric_npc(numeric);
    Check(numeric_npc.SetNpcAngle("NPC01", 123.25f) == 0x1234u &&
              numeric.calls == std::vector<std::string>{
                  "npc manager", "npc lookup", "read x", "read z",
                  "rotation", "allocate", "executor", "construct",
                  "allocate text", "assign name", "close", "executor",
                  "add", "cleanup"},
          "NPC numeric yaw preserves X/Z and closes named handler");

    Runtime failed;
    failed.allocation_fails = true;
    failed.expected_angle = 41.0f;
    pal4::ScriptPlayerRotation failed_npc(failed);
    Check(failed_npc.SetNpcAngle("NPC01", 41.0f) == 0x1234u &&
              failed.calls == std::vector<std::string>{
                  "npc manager", "npc lookup", "read x", "read z",
                  "rotation", "allocate", "executor", "add", "cleanup"},
          "NPC angle queues null handler after allocation failure");

    Runtime missing_numeric;
    missing_numeric.actor_exists = false;
    pal4::ScriptPlayerRotation missing_numeric_npc(missing_numeric);
    Check(missing_numeric_npc.SetNpcAngle("NPC01", 41.0f) == 0u &&
              missing_numeric.calls == std::vector<std::string>{
                  "npc manager", "npc lookup"},
          "missing NPC skips numeric yaw setup");
}

}  // namespace

int main() {
    try {
        DirectionCases();
        AngleCases();
        CompletedQueueCases();
        NpcCases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
