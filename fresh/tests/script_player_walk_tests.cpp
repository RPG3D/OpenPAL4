#include "pal4/script_player_walk.h"

#include <bit>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::IScriptPlayerWalkRuntime {
public:
    pal4::PlayerWalkWaitState state{};
    std::vector<std::string> calls;
    std::string action_name = "idle";
    bool actor_exists = true;
    bool route_exists = true;
    bool movement_starts = true;
    bool allocation_fails = false;
    bool movement_active = true;
    bool running = false;
    bool use_actor_name = false;
    bool walking_back = false;
    bool project_target = false;
    bool use_npc_name = false;
    std::uint8_t expected_wait_mode = 7u;
    std::int32_t c02_result = 0;
    std::int32_t c01_result = 0;

    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager");
        return 0x4000u;
    }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::int32_t index) override {
        Check(manager == 0x4000u && index == 2, "walk actor lookup");
        calls.emplace_back("indexed");
        return actor_exists ? 0x5000u : 0u;
    }
    std::uint32_t CurrentPlayer(std::uint32_t manager) override {
        Check(manager == 0x4000u, "current player manager");
        calls.emplace_back("current");
        return actor_exists ? 0x5000u : 0u;
    }
    std::string_view ActorName(std::uint32_t actor) override {
        Check(actor == 0x5000u, "current actor name receiver");
        calls.emplace_back("actor name");
        return "Hero";
    }
    std::uint32_t NpcManager() override {
        calls.emplace_back("npc manager");
        return 0x8000u;
    }
    std::uint32_t NpcByName(std::uint32_t manager,
                            std::string_view name) override {
        Check(manager == 0x8000u &&
                  (name == "NPC01" || name.empty()),
              "NPC manager name lookup");
        calls.emplace_back("npc lookup");
        return actor_exists ? 0x5000u : 0u;
    }
    pal4::PlayerWalkPoint ReadPosition(std::uint32_t actor) override {
        Check(actor == 0x5000u, "walk starting position actor");
        calls.emplace_back("read position");
        return {1.0f, 2.0f, 3.0f};
    }
    pal4::PlayerWalkPoint ConvertPoint(
        const pal4::PlayerWalkPoint& point) override {
        Check((point.x == 1.0f && point.y == 2.0f && point.z == 3.0f) ||
                  (point.x == 4.0f && point.y == 5.0f && point.z == 6.0f),
              "walk point conversion input");
        calls.emplace_back(point.x == 1.0f ? "convert start" :
                                              "convert target");
        return {point.x + 10.0f, point.y + 10.0f, point.z + 10.0f};
    }
    std::uint32_t ArenaManager() override {
        calls.emplace_back("arena");
        return 0x4000u;
    }
    std::uint32_t PlanRoute(std::uint32_t source,
                            pal4::PlayerWalkPoint& target) override {
        Check(source == 0x4194u && target.x == 14.0f &&
                  target.y == 15.0f && target.z == 16.0f,
              "arena route source and converted target");
        calls.emplace_back("plan");
        if (project_target) target.y = 12.5f;
        return route_exists ? 0x9000u : 0u;
    }
    bool StartMovement(std::uint32_t movement,
                       const pal4::PlayerWalkPoint& from,
                       const pal4::PlayerWalkPoint& to,
                       std::uint32_t route,
                       std::uint32_t mode) override {
        Check(movement == 0x5564u && from.x == 11.0f &&
                  from.y == 12.0f && from.z == 13.0f &&
                  to.x == 14.0f &&
                  to.y == (project_target ? 12.5f : 15.0f) &&
                  to.z == 16.0f &&
                  route == 0x9000u && mode == 0u,
              "walk vtable +24 operands");
        calls.emplace_back("start movement");
        return movement_starts;
    }
    void SetMovementSpeed(std::uint32_t movement, float speed) override {
        Check(movement == 0x5564u &&
                  // Independent original vtable+36 immediates, not production constants.
                  std::bit_cast<std::uint32_t>(speed) == (running ? 0x430c0000u : 0x42340000u),
              "walk vtable +36 speed");
        calls.emplace_back("speed");
    }
    void SetMovementStatus(std::uint32_t actor,
                           std::uint32_t value) override {
        Check(actor == 0x5000u && value == (walking_back ? 1u : 0u),
              "actor +1540 movement status");
        calls.emplace_back("reset status");
    }
    std::string_view ActionName(std::uint32_t actor) override {
        Check(actor == 0x5000u, "walk action name receiver");
        calls.emplace_back("action name");
        return action_name;
    }
    std::int32_t SetAction(std::uint32_t actor,
                           std::string_view action) override {
        Check(actor == 0x5000u, "walk action receiver");
        calls.emplace_back("action:" + std::string(action));
        if (action == "C02" || action == "C03") return c02_result;
        if (action == "C01") return c01_result;
        return 1;
    }
    pal4::PlayerWalkWaitState* Allocate(
        std::uint32_t size) override {
        Check(size == 0x24u, "walk wait allocation size");
        calls.emplace_back("allocate");
        return allocation_fails ? nullptr : &state;
    }
    std::uint32_t ScriptExecutor() override {
        calls.emplace_back("executor");
        return 0x6000u;
    }
    void Construct(pal4::PlayerWalkWaitState& candidate,
                   std::uint8_t mode,
                   std::uint32_t queue) override {
        Check(&candidate == &state && mode == expected_wait_mode &&
                  queue == 0x6054u,
              "walk wait constructor");
        calls.emplace_back("construct");
    }
    void AssignName(pal4::PlayerWalkWaitState& candidate,
                    std::string_view name) override {
        Check(&candidate == &state &&
                  name == (use_npc_name ? "NPC01-Player-Movment" :
                           use_actor_name ? "Hero-Player-Movment" :
                                            "2-Player-Movment") &&
                  state.actor == 0x5000u &&
                  state.vtable_address == 0x84957cu && state.phase == 1u,
              "walk wait actor, vtable, phase and name");
        calls.emplace_back("name");
    }
    void CleanupWeaponEffects(std::uint32_t actor) override {
        Check(actor == 0x5000u, "walk effects actor");
        calls.emplace_back("effects");
    }
    void Flush(pal4::PlayerWalkWaitState& candidate) override {
        Check(&candidate == &state, "walk flush receiver");
        calls.emplace_back("flush");
    }
    void AddHandler(std::uint32_t queue,
                    pal4::PlayerWalkWaitState* candidate) override {
        Check(queue == 0x6054u &&
                  candidate == (allocation_fails ? nullptr : &state),
              "walk queue result");
        calls.emplace_back("add");
    }
    std::uint32_t CleanupName() override {
        calls.emplace_back("cleanup");
        return 0x1234u;
    }
    std::uint32_t ShowPathError() override {
        calls.emplace_back("message box");
        return 0x99u;
    }
    bool MovementActive(std::uint32_t movement) override {
        Check(movement == 0x5564u, "walk active receiver");
        calls.emplace_back("active");
        return movement_active;
    }
    void StopMovement(std::uint32_t movement) override {
        Check(movement == 0x5564u, "walk stop vtable +28 receiver");
        calls.emplace_back("stop movement");
    }
    void Close(pal4::PlayerWalkWaitState& candidate) override {
        Check(&candidate == &state, "walk close receiver");
        calls.emplace_back("close");
    }
};

void WalkCases() {
    constexpr pal4::PlayerWalkPoint target{4.0f, 5.0f, 6.0f};
    Runtime runtime;
    pal4::ScriptPlayerWalk walk(runtime);
    Check(walk.WalkTo(2, target, 7u) == 0x1234u &&
              runtime.calls == std::vector<std::string>{
                  "manager", "indexed", "read position", "convert start",
                  "convert target", "arena", "plan", "start movement",
                  "speed", "reset status", "action name", "action name",
                  "action:C02", "action:Z02", "allocate", "executor",
                  "construct", "name", "effects", "flush", "executor",
                  "add", "cleanup"},
          "walk route, action and wait-handler call order");

    Runtime already_walking;
    already_walking.action_name = "c02";
    pal4::ScriptPlayerWalk same_action(already_walking);
    Check(same_action.WalkTo(2, target, 7u) == 0x1234u &&
              already_walking.calls[10] == "action name" &&
              already_walking.calls[11] == "allocate",
          "case-insensitive C02 guard avoids action reset");

    Runtime no_actor;
    no_actor.actor_exists = false;
    pal4::ScriptPlayerWalk missing(no_actor);
    Check(missing.WalkTo(2, target, 7u) == 0u &&
              no_actor.calls == std::vector<std::string>{
                  "manager", "indexed"},
          "missing actor returns before route planning");

    Runtime no_route;
    no_route.route_exists = false;
    pal4::ScriptPlayerWalk route_failure(no_route);
    Check(route_failure.WalkTo(2, target, 7u) == 0u &&
              no_route.calls.back() == "plan",
          "missing route returns without message or handler");

    Runtime no_movement;
    no_movement.movement_starts = false;
    pal4::ScriptPlayerWalk movement_failure(no_movement);
    Check(movement_failure.WalkTo(2, target, 7u) == 0x99u &&
              no_movement.calls.back() == "message box",
          "failed movement returns message-box result");

    Runtime no_handler;
    no_handler.allocation_fails = true;
    pal4::ScriptPlayerWalk allocation_failure(no_handler);
    Check(allocation_failure.WalkTo(2, target, 7u) == 0x1234u &&
              no_handler.calls[14] == "allocate" &&
              no_handler.calls[15] == "executor" &&
              no_handler.calls[16] == "add" &&
              no_handler.calls[17] == "cleanup",
          "failed handler allocation still queues null");
}

void CallbackCases() {
    Runtime runtime;
    runtime.state.actor = 0x5000u;
    pal4::ScriptPlayerWalk walk(runtime);
    walk.Update(runtime.state);
    Check(runtime.calls == std::vector<std::string>{"active"},
          "active movement keeps wait handler open");
    runtime.calls.clear();
    runtime.movement_active = false;
    walk.Update(runtime.state);
    Check(runtime.calls == std::vector<std::string>{
              "active", "action:C01", "action:Z01", "close"},
          "completed movement restores idle and closes");
    runtime.calls.clear();
    walk.Stop(runtime.state);
    Check(runtime.calls == std::vector<std::string>{
              "stop movement", "close"},
          "explicit stop calls movement vtable before close");
}

void RunCases() {
    Runtime runtime;
    runtime.running = true;
    runtime.project_target = true;
    pal4::ScriptPlayerWalk run(runtime);
    const pal4::PlayerWalkPoint target{4.0f, 5.0f, 6.0f};
    Check(run.RunTo(2, target, 7u) == 0x1234u &&
              runtime.calls[8] == "speed" &&
              runtime.calls[12] == "action:C03" &&
              runtime.calls[13] == "action:Z02" &&
              runtime.calls.back() == "cleanup",
          "run forwards the snapped target with speed 140 and C03 fallback");

    Runtime already_running;
    already_running.running = true;
    already_running.action_name = "c03";
    pal4::ScriptPlayerWalk same_action(already_running);
    Check(same_action.RunTo(2, target, 7u) == 0x1234u &&
              already_running.calls[10] == "action name" &&
              already_running.calls[11] == "allocate",
          "case-insensitive C03 guard avoids action reset");
}

void CurrentCases() {
    constexpr pal4::PlayerWalkPoint target{4.0f, 5.0f, 6.0f};
    Runtime runtime;
    runtime.use_actor_name = true;
    pal4::ScriptPlayerWalk walk(runtime);
    Check(walk.WalkCurrent(target, 7u) == 0x1234u &&
              runtime.calls == std::vector<std::string>{
                  "manager", "current", "read position", "convert start",
                  "convert target", "arena", "plan", "start movement",
                  "speed", "reset status", "action name", "action name",
                  "action:C02", "action:Z02", "actor name", "allocate",
                  "executor", "construct", "name", "effects", "flush",
                  "executor", "add", "cleanup"},
          "current walking uses actor lookup and name after action");

    Runtime missing;
    missing.actor_exists = false;
    pal4::ScriptPlayerWalk missing_walk(missing);
    Check(missing_walk.WalkCurrent(target, 7u) == 0u &&
              missing.calls == std::vector<std::string>{
                  "manager", "current"},
          "missing current actor skips route planning");
}

void BackCases() {
    constexpr pal4::PlayerWalkPoint target{4.0f, 5.0f, 6.0f};
    Runtime runtime;
    runtime.walking_back = true;
    pal4::ScriptPlayerWalk walk(runtime);
    Check(walk.WalkBack(2, target, 7u) == 0x1234u &&
              runtime.calls[8] == "speed" &&
              runtime.calls[9] == "reset status" &&
              runtime.calls[12] == "action:C02" &&
              runtime.calls.back() == "cleanup",
          "backward walk sets status one and follows walk handler path");
}

void NpcCases() {
    constexpr pal4::PlayerWalkPoint target{4.0f, 5.0f, 6.0f};
    Runtime runtime;
    runtime.use_npc_name = true;
    pal4::ScriptPlayerWalk walk(runtime);
    Check(walk.WalkNpc("NPC01", target, 7u) == 0x1234u &&
              runtime.calls[0] == "npc manager" &&
              runtime.calls[1] == "npc lookup" &&
              runtime.calls[2] == "read position" &&
              runtime.calls[14] == "allocate" &&
              runtime.calls.back() == "cleanup",
          "NPC walk uses name lookup and shared movement handler");

    Runtime missing;
    missing.actor_exists = false;
    pal4::ScriptPlayerWalk missing_walk(missing);
    Check(missing_walk.WalkNpc("", target, 7u) == 0u &&
              missing.calls == std::vector<std::string>{
                  "npc manager", "npc lookup"},
          "empty NPC name is looked up and missing actor skips route");

    Runtime running;
    running.running = true;
    running.use_npc_name = true;
    pal4::ScriptPlayerWalk npc_run(running);
    Check(npc_run.RunNpc("NPC01", target, 7u) == 0x1234u &&
              running.calls[0] == "npc manager" &&
              running.calls[1] == "npc lookup" &&
              running.calls[8] == "speed" &&
              running.calls[12] == "action:C03" &&
              running.calls[13] == "action:Z02" &&
              running.calls.back() == "cleanup",
          "NPC run uses speed 140 and C03/Z02 on shared route");

    Runtime backward;
    backward.walking_back = true;
    backward.use_npc_name = true;
    pal4::ScriptPlayerWalk npc_back(backward);
    Check(npc_back.BackNpc("NPC01", target, 7u) == 0x1234u &&
              backward.calls[0] == "npc manager" &&
              backward.calls[8] == "speed" &&
              backward.calls[9] == "reset status" &&
              backward.calls[12] == "action:C02" &&
              backward.calls.back() == "cleanup",
          "NPC back walk sets movement status one");
}

void EndMoveCases() {
    Runtime indexed;
    indexed.expected_wait_mode = 1u;
    pal4::ScriptPlayerWalk indexed_walk(indexed);
    Check(indexed_walk.EndMoveIndexed(2) == 0x1234u &&
              indexed.calls == std::vector<std::string>{
                  "manager", "indexed", "allocate", "executor",
                  "construct", "name", "effects", "flush", "executor",
                  "add", "cleanup"},
          "indexed end-move queues mode-one handler without route planning");

    Runtime current;
    current.use_actor_name = true;
    current.expected_wait_mode = 1u;
    pal4::ScriptPlayerWalk current_walk(current);
    Check(current_walk.EndMoveCurrent() == 0x1234u &&
              current.calls == std::vector<std::string>{
                  "manager", "current", "actor name", "allocate",
                  "executor", "construct", "name", "effects", "flush",
                  "executor", "add", "cleanup"},
          "current end-move names handler from actor before allocation");

    Runtime npc;
    npc.use_npc_name = true;
    npc.expected_wait_mode = 1u;
    pal4::ScriptPlayerWalk npc_walk(npc);
    Check(npc_walk.EndMoveNpc("NPC01") == 0x1234u &&
              npc.calls == std::vector<std::string>{
                  "npc manager", "npc lookup", "allocate", "executor",
                  "construct", "name", "effects", "flush", "executor",
                  "add", "cleanup"},
          "NPC end-move uses script name and shared handler");

    Runtime missing;
    missing.actor_exists = false;
    pal4::ScriptPlayerWalk missing_walk(missing);
    Check(missing_walk.EndMoveNpc("") == 0u &&
              missing.calls == std::vector<std::string>{
                  "npc manager", "npc lookup"},
          "missing NPC skips end-move handler");

    Runtime failed;
    failed.allocation_fails = true;
    pal4::ScriptPlayerWalk failed_walk(failed);
    Check(failed_walk.EndMoveIndexed(2) == 0x1234u &&
              failed.calls == std::vector<std::string>{
                  "manager", "indexed", "allocate", "executor", "add",
                  "cleanup"},
          "failed end-move allocation still queues null and cleans name");
}

}  // namespace

int main() {
    try {
        WalkCases();
        CallbackCases();
        RunCases();
        CurrentCases();
        BackCases();
        NpcCases();
        EndMoveCases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
