#include "pal4/script_player_turn.h"

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

bool Near(float actual, float expected) {
    return std::fabs(actual - expected) < 0.0001f;
}

class Runtime final : public pal4::IScriptPlayerTurnRuntime {
public:
    pal4::PlayerTurnWaitState state{};
    std::vector<std::string> calls;
    float yaw = 20.0f;
    float expected_rotation = 0.0f;
    bool actor_exists = true;
    bool target_exists = true;
    bool npc_source_exists = true;
    bool npc_info_exists = true;
    bool current_exists = true;
    bool object_exists = true;
    bool allocation_fails = false;
    bool text_allocation_fails = false;
    std::int32_t action_result = 1;
    std::int32_t timed_result = 1;
    std::string expected_name = "2-Player-Movment";
    std::uint32_t current_actor = 0x7000u;

    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager");
        return 0x4000u;
    }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::int32_t index) override {
        Check(manager == 0x4000u && (index == 2 || index == 3),
              "turn actor lookup");
        calls.emplace_back(index == 2 ? "indexed" : "target");
        return index == 2 ? (actor_exists ? 0x5000u : 0u) :
                            (target_exists ? 0x7000u : 0u);
    }
    std::uint32_t CurrentPlayer(std::uint32_t manager) override {
        Check(manager == 0x4000u, "current player manager");
        calls.emplace_back("current player");
        return current_exists ? current_actor : 0u;
    }
    std::int32_t CurrentPlayerIndex(std::uint32_t manager) override {
        Check(manager == 0x4000u, "current player index manager");
        calls.emplace_back("current index");
        return 2;
    }
    std::uint32_t ObjectManager() override {
        calls.emplace_back("object manager");
        return 0xa000u;
    }
    std::uint32_t ObjectByName(std::uint32_t manager,
                               std::string_view name) override {
        Check(manager == 0xa000u && name == "OBJ01",
              "turn object lookup");
        calls.emplace_back("object lookup");
        return object_exists ? 0x7000u : 0u;
    }
    std::uint32_t NpcManager() override {
        calls.emplace_back("npc manager");
        return 0x8000u;
    }
    std::uint32_t NpcByName(std::uint32_t manager,
                            std::string_view name) override {
        Check(manager == 0x8000u &&
                  (name == "NPC01" || name == "SOURCE" || name.empty()),
              "NPC name lookup");
        calls.emplace_back(name == "SOURCE" ? "npc source" : "npc lookup");
        if (name == "SOURCE") return npc_source_exists ? 0x5000u : 0u;
        return target_exists ? 0x7000u : 0u;
    }
    std::uint32_t NpcInfoByName(std::uint32_t manager,
                                std::string_view name) override {
        Check(manager == 0x8000u && name == "SOURCE",
              "NPC baseline info lookup");
        calls.emplace_back("npc info");
        return npc_info_exists ? 0x9000u : 0u;
    }
    float BaselineYaw(std::uint32_t info) override {
        Check(info == 0x9000u, "NPC baseline record");
        calls.emplace_back("baseline yaw");
        return 41.0f;
    }
    pal4::PlayerTurnVector ReadPosition(std::uint32_t actor) override {
        Check(actor == 0x5000u || actor == 0x7000u,
              "facing actor position");
        calls.emplace_back(actor == 0x5000u ? "source position" :
                                               "target position");
        return actor == 0x5000u ? pal4::PlayerTurnVector{1.0f, 2.0f, 3.0f} :
                                  pal4::PlayerTurnVector{4.0f, 6.0f, 8.0f};
    }
    void ClampFacingVector(pal4::PlayerTurnVector& vector) override {
        Check(vector.x == 3.0f && vector.y == 4.0f && vector.z == 5.0f,
              "target-minus-source facing vector");
        calls.emplace_back("clamp vector");
    }
    float FacingAngleDegrees(
        const pal4::PlayerTurnVector& vector) override {
        Check(vector.x == 3.0f && vector.y == 4.0f && vector.z == 5.0f,
              "facing angle input");
        calls.emplace_back("angle");
        return 41.0f;
    }
    pal4::PlayerTurnWaitState* Allocate(std::uint32_t size) override {
        Check(size == 0x38u, "turn wait allocation size");
        calls.emplace_back("allocate");
        return allocation_fails ? nullptr : &state;
    }
    std::uint32_t ScriptExecutor() override {
        calls.emplace_back("executor");
        return 0x6000u;
    }
    void Construct(pal4::PlayerTurnWaitState& candidate,
                   std::uint8_t mode, std::uint32_t queue) override {
        Check(&candidate == &state && (mode == 7u || mode == 1u) &&
                  queue == 0x6054u,
              "turn wait mode and queue");
        calls.emplace_back("construct");
    }
    void Flush(pal4::PlayerTurnWaitState& candidate) override {
        Check(&candidate == &state && state.actor == 0x5000u &&
                  state.vtable_address == 0x849570u,
              "turn actor and vtable set before flush");
        calls.emplace_back("flush");
    }
    bool AllocateText(pal4::PlayerTurnWaitState& candidate,
                      std::uint32_t size) override {
        Check(&candidate == &state && state.phase == 2u &&
                  size == expected_name.size(),
              "turn name allocation");
        calls.emplace_back("allocate text");
        return !text_allocation_fails;
    }
    void AssignName(pal4::PlayerTurnWaitState& candidate,
                    std::string_view name) override {
        Check(&candidate == &state && name == expected_name,
              "turn formatted name");
        calls.emplace_back("name");
    }
    float ReadYaw(std::uint32_t actor) override {
        Check(actor == 0x5000u, "read yaw actor");
        calls.emplace_back("read yaw");
        return yaw;
    }
    float ReadX(std::uint32_t actor) override {
        Check(actor == 0x5000u, "read rotation X actor");
        calls.emplace_back("read x");
        return 12.0f;
    }
    float ReadZ(std::uint32_t actor) override {
        Check(actor == 0x5000u, "read rotation Z actor");
        calls.emplace_back("read z");
        return 34.0f;
    }
    void SetRotation(std::uint32_t actor, float x,
                     float turn_yaw, float z,
                     std::uint32_t mode) override {
        Check(actor == 0x5000u && x == 12.0f && z == 34.0f &&
                  Near(turn_yaw, expected_rotation) && mode == 0u,
              "turn interpolation vector");
        calls.emplace_back("rotation");
    }
    std::int32_t SetAction(std::uint32_t actor,
                           std::string_view action) override {
        Check(actor == 0x5000u &&
                  (action == "C02" || action == "Z02"),
              "turn action state");
        calls.emplace_back("action:" + std::string(action));
        return action == "C02" ? action_result : 1;
    }
    std::int32_t SetTimedAction(std::uint32_t actor,
                                std::string_view action,
                                float blend) override {
        Check(actor == 0x5000u && Near(blend, 0.2f) &&
                  (action == "C01" || action == "Z01"),
              "idle timed action and blend");
        calls.emplace_back("timed:" + std::string(action));
        return action == "C01" ? timed_result : 1;
    }
    void CleanupWeaponEffects(std::uint32_t actor) override {
        Check(actor == 0x5000u, "turn effects actor");
        calls.emplace_back("effects");
    }
    void Close(pal4::PlayerTurnWaitState& candidate) override {
        Check(&candidate == &state, "turn close receiver");
        calls.emplace_back("close");
    }
    void AddHandler(std::uint32_t queue,
                    pal4::PlayerTurnWaitState* candidate) override {
        Check(queue == 0x6054u &&
                  candidate == (allocation_fails ? nullptr : &state),
              "turn queue result");
        calls.emplace_back("add");
    }
    std::uint32_t CleanupName() override {
        calls.emplace_back("cleanup");
        return 0x1234u;
    }
};

void StartCases() {
    Runtime runtime;
    runtime.action_result = 0;
    pal4::ScriptPlayerTurn turn(runtime);
    Check(turn.Start(2, 41.0f, 7u) == 0x1234u &&
              Near(runtime.state.delta, 21.0f) &&
              Near(runtime.state.start_yaw, 20.0f) &&
              Near(runtime.state.speed, 21.0f) &&
              Near(runtime.state.duration, 1.0f) &&
              Near(runtime.state.elapsed, 0.0f) &&
              runtime.calls == std::vector<std::string>{
                  "manager", "indexed", "allocate", "executor",
                  "construct", "flush", "allocate text", "name",
                  "read yaw", "action:C02", "action:Z02", "effects",
                  "executor", "add", "cleanup"},
          "turn start layout, action fallback and queue order");

    Runtime near;
    pal4::ScriptPlayerTurn near_turn(near);
    Check(near_turn.Start(2, 20.004f, 7u) == 0x1234u &&
              near.state.duration == 0.0f &&
              near.calls[9] == "close" && near.calls[10] == "executor",
          "near-equal target closes without starting turn animation");

    struct Case { float current; float target; float delta; };
    for (const auto& test : {
             Case{350.0f, 10.0f, 20.0f},
             Case{200.0f, 10.0f, 170.0f},
             Case{190.0f, 20.0f, -170.0f},
             Case{10.0f, 350.0f, -20.0f},
             Case{132.0f, 324.0f, -168.0f},
             Case{10.0f, 190.0f, 180.0f},
             Case{10.0f, 180.0f, 170.0f},
             Case{180.0f, 10.0f, -170.0f},
             Case{180.0f, -10.0f, -190.0f},
             Case{30.0f, 20.0f, -10.0f}}) {
        Runtime sample;
        sample.yaw = test.current;
        pal4::ScriptPlayerTurn sample_turn(sample);
        Check(sample_turn.Start(2, test.target, 7u) == 0x1234u &&
                  Near(sample.state.delta, test.delta) &&
                  Near(sample.state.duration,
                       std::fabs(test.delta) / 21.0f),
              "IDA turn branch delta and duration");
    }

    Runtime no_text;
    no_text.text_allocation_fails = true;
    pal4::ScriptPlayerTurn no_text_turn(no_text);
    Check(no_text_turn.Start(2, 41.0f, 7u) == 0x1234u &&
              no_text.calls[7] == "read yaw",
          "failed text allocation still prepares turn");

    Runtime no_handler;
    no_handler.allocation_fails = true;
    pal4::ScriptPlayerTurn no_handler_turn(no_handler);
    Check(no_handler_turn.Start(2, 41.0f, 7u) == 0x1234u &&
              no_handler.calls == std::vector<std::string>{
                  "manager", "indexed", "allocate", "executor", "add",
                  "cleanup"},
          "failed wait allocation still queues null");

    Runtime missing;
    missing.actor_exists = false;
    pal4::ScriptPlayerTurn missing_turn(missing);
    Check(missing_turn.Start(2, 41.0f, 7u) == 0u &&
              missing.calls == std::vector<std::string>{
                  "manager", "indexed"},
          "missing actor skips turn");
}

void UpdateCases() {
    Runtime runtime;
    pal4::ScriptPlayerTurn turn(runtime);
    auto& state = runtime.state;
    state.actor = 0x5000u;
    state.speed = 21.0f;
    state.delta = 21.0f;
    state.start_yaw = 20.0f;
    state.duration = 1.0f;
    runtime.expected_rotation = 30.5f;
    turn.Update(state, 0.5 / 21.0);
    Check(Near(state.elapsed, 0.5f) &&
              runtime.calls == std::vector<std::string>{
                  "read x", "read z", "rotation"},
          "partial frame interpolates without closing");

    runtime.calls.clear();
    runtime.expected_rotation = 41.0f;
    runtime.timed_result = 0;
    turn.Update(state, 1.0);
    Check(Near(state.elapsed, 1.0f) &&
              runtime.calls == std::vector<std::string>{
                  "timed:C01", "timed:Z01", "close", "read x",
                  "read z", "rotation"},
          "completion clamps elapsed, restores idle, closes then rotates");

    runtime.calls.clear();
    state.duration = 0.0f;
    turn.Update(state, 0.1);
    Check(runtime.calls == std::vector<std::string>{"close"},
          "zero duration only closes");

    runtime.calls.clear();
    runtime.timed_result = 1;
    turn.Finish(state);
    Check(runtime.calls == std::vector<std::string>{
              "timed:C01", "close"},
          "explicit finish skips fallback after successful C01");
}

void FacePlayerCases() {
    Runtime runtime;
    pal4::ScriptPlayerTurn turn(runtime);
    Check(turn.FacePlayer(2, 3, 7u) == 0x1234u &&
              Near(runtime.state.delta, 21.0f) &&
              runtime.calls == std::vector<std::string>{
                  "manager", "indexed", "manager", "target",
                  "source position", "target position", "clamp vector",
                  "angle", "allocate", "executor", "construct", "flush",
                  "allocate text", "name", "read yaw", "action:C02",
                  "effects", "executor", "add", "cleanup"},
          "face-player vector and common turn handler order");

    Runtime missing_source;
    missing_source.actor_exists = false;
    pal4::ScriptPlayerTurn missing_source_turn(missing_source);
    Check(missing_source_turn.FacePlayer(2, 3, 7u) == 0u &&
              missing_source.calls == std::vector<std::string>{
                  "manager", "indexed", "manager", "target"},
          "target lookup still occurs when source is missing");

    Runtime missing_target;
    missing_target.target_exists = false;
    pal4::ScriptPlayerTurn missing_target_turn(missing_target);
    Check(missing_target_turn.FacePlayer(2, 3, 7u) == 0u &&
              missing_target.calls == std::vector<std::string>{
                  "manager", "indexed", "manager", "target"},
          "missing target skips vector and turn handler");
}

void FaceNpcCases() {
    Runtime runtime;
    pal4::ScriptPlayerTurn turn(runtime);
    Check(turn.FaceNpc(2, "NPC01", 7u) == 0x1234u &&
              Near(runtime.state.delta, 21.0f) &&
              runtime.calls == std::vector<std::string>{
                  "manager", "indexed", "npc manager", "npc lookup",
                  "source position", "target position", "clamp vector",
                  "angle", "allocate", "executor", "construct", "flush",
                  "allocate text", "name", "read yaw", "action:C02",
                  "effects", "executor", "add", "cleanup"},
          "face-NPC name lookup and shared turn setup");

    Runtime missing_source;
    missing_source.actor_exists = false;
    pal4::ScriptPlayerTurn missing_source_turn(missing_source);
    Check(missing_source_turn.FaceNpc(2, "", 7u) == 0u &&
              missing_source.calls == std::vector<std::string>{
                  "manager", "indexed", "npc manager", "npc lookup"},
          "NPC lookup still occurs for missing source and empty name");

    Runtime missing_npc;
    missing_npc.target_exists = false;
    pal4::ScriptPlayerTurn missing_npc_turn(missing_npc);
    Check(missing_npc_turn.FaceNpc(2, "NPC01", 7u) == 0u &&
              missing_npc.calls == std::vector<std::string>{
                  "manager", "indexed", "npc manager", "npc lookup"},
          "missing NPC skips vector and turn handler");
}

void StartNpcCases() {
    Runtime runtime;
    runtime.expected_name = "SOURCE-Player-Movment";
    runtime.action_result = 0;
    pal4::ScriptPlayerTurn turn(runtime);
    Check(turn.StartNpc("SOURCE", 41.0f, 7u) == 0x1234u &&
              Near(runtime.state.delta, 21.0f) &&
              Near(runtime.state.duration, 1.0f) &&
              runtime.calls == std::vector<std::string>{
                  "npc manager", "npc source", "allocate", "executor",
                  "construct", "flush", "allocate text", "name",
                  "read yaw", "action:C02", "action:Z02", "effects",
                  "executor", "add", "cleanup"},
          "named NPC turn uses shared handler, name and action fallback");

    Runtime near;
    near.expected_name = "SOURCE-Player-Movment";
    pal4::ScriptPlayerTurn near_turn(near);
    Check(near_turn.StartNpc("SOURCE", 20.004f, 7u) == 0x1234u &&
              near.calls[9] == "close" && near.state.duration == 0.0f,
          "near NPC yaw closes handler without action change");

    Runtime no_handler;
    no_handler.allocation_fails = true;
    pal4::ScriptPlayerTurn no_handler_turn(no_handler);
    Check(no_handler_turn.StartNpc("SOURCE", 41.0f, 7u) == 0x1234u &&
              no_handler.calls == std::vector<std::string>{
                  "npc manager", "npc source", "allocate", "executor",
                  "add", "cleanup"},
          "NPC turn queues null after allocation failure");

    Runtime missing;
    missing.npc_source_exists = false;
    pal4::ScriptPlayerTurn missing_turn(missing);
    Check(missing_turn.StartNpc("SOURCE", 41.0f, 7u) == 0u &&
              missing.calls == std::vector<std::string>{
                  "npc manager", "npc source"},
          "missing NPC skips turn setup");
}

void FaceNpcToNpcCases() {
    Runtime runtime;
    runtime.expected_name = "SOURCE-Player-Movment";
    pal4::ScriptPlayerTurn turn(runtime);
    Check(turn.FaceNpcToNpc("SOURCE", "NPC01", 7u) == 0x1234u &&
              Near(runtime.state.delta, 21.0f) &&
              runtime.calls == std::vector<std::string>{
                  "npc manager", "npc source", "npc manager", "npc lookup",
                  "source position", "target position", "clamp vector",
                  "angle", "allocate", "executor", "construct", "flush",
                  "allocate text", "name", "read yaw", "action:C02",
                  "effects", "executor", "add", "cleanup"},
          "NPC-to-NPC facing uses target-minus-source and source label");

    Runtime missing_source;
    missing_source.npc_source_exists = false;
    pal4::ScriptPlayerTurn missing_source_turn(missing_source);
    Check(missing_source_turn.FaceNpcToNpc("SOURCE", "NPC01", 7u) ==
              0x7000u &&
              missing_source.calls == std::vector<std::string>{
                  "npc manager", "npc source", "npc manager", "npc lookup"},
          "missing source returns the already-found target pointer");

    Runtime missing_target;
    missing_target.target_exists = false;
    pal4::ScriptPlayerTurn missing_target_turn(missing_target);
    Check(missing_target_turn.FaceNpcToNpc("SOURCE", "NPC01", 7u) == 0u &&
              missing_target.calls == std::vector<std::string>{
                  "npc manager", "npc source", "npc manager", "npc lookup"},
          "missing target skips NPC facing");
}

void FaceNpcToPlayerCases() {
    Runtime runtime;
    runtime.expected_name = "SOURCE-Player-Movment";
    pal4::ScriptPlayerTurn turn(runtime);
    Check(turn.FaceNpcToPlayer("SOURCE", 3, 7u) == 0x1234u &&
              Near(runtime.state.delta, 21.0f) &&
              runtime.calls == std::vector<std::string>{
                  "npc manager", "npc source", "manager", "target",
                  "source position", "target position", "clamp vector",
                  "angle", "allocate", "executor", "construct", "flush",
                  "allocate text", "name", "read yaw", "action:C02",
                  "effects", "executor", "add", "cleanup"},
          "NPC-to-player facing resolves player second and labels NPC");

    Runtime missing_source;
    missing_source.npc_source_exists = false;
    pal4::ScriptPlayerTurn missing_source_turn(missing_source);
    Check(missing_source_turn.FaceNpcToPlayer("SOURCE", 3, 7u) ==
              0x7000u &&
              missing_source.calls == std::vector<std::string>{
                  "npc manager", "npc source", "manager", "target"},
          "missing NPC returns found player pointer");

    Runtime missing_target;
    missing_target.target_exists = false;
    pal4::ScriptPlayerTurn missing_target_turn(missing_target);
    Check(missing_target_turn.FaceNpcToPlayer("SOURCE", 3, 7u) == 0u &&
              missing_target.calls == std::vector<std::string>{
                  "npc manager", "npc source", "manager", "target"},
          "missing player skips NPC facing");
}

void CurrentPlayerFacingCases() {
    Runtime player_to_current;
    pal4::ScriptPlayerTurn first(player_to_current);
    Check(first.FacePlayerToCurrent(2, 7u) == 0x1234u &&
              player_to_current.calls == std::vector<std::string>{
                  "manager", "indexed", "manager", "current player",
                  "source position", "target position", "clamp vector",
                  "angle", "allocate", "executor", "construct", "flush",
                  "allocate text", "name", "read yaw", "action:C02",
                  "effects", "executor", "add", "cleanup"},
          "indexed player faces current player using source index label");

    Runtime no_source;
    no_source.actor_exists = false;
    pal4::ScriptPlayerTurn no_source_turn(no_source);
    Check(no_source_turn.FacePlayerToCurrent(2, 7u) == 0x7000u &&
              no_source.calls == std::vector<std::string>{
                  "manager", "indexed", "manager", "current player"},
          "missing indexed player returns current player pointer");

    Runtime current_to_npc;
    current_to_npc.current_actor = 0x5000u;
    current_to_npc.expected_name = "NPC01-Player-Movment";
    pal4::ScriptPlayerTurn second(current_to_npc);
    Check(second.FaceCurrentToNpc("NPC01", 7u) == 0x1234u &&
              current_to_npc.calls == std::vector<std::string>{
                  "manager", "current player", "npc manager",
                  "npc lookup", "source position", "target position",
                  "clamp vector", "angle", "allocate", "executor",
                  "construct", "flush", "allocate text", "name",
                  "read yaw", "action:C02", "effects", "executor",
                  "add", "cleanup"},
          "current player faces NPC with target NPC handler label");

    Runtime no_current;
    no_current.current_exists = false;
    pal4::ScriptPlayerTurn no_current_turn(no_current);
    Check(no_current_turn.FaceCurrentToNpc("NPC01", 7u) == 0x7000u &&
              no_current.calls == std::vector<std::string>{
                  "manager", "current player", "npc manager",
                  "npc lookup"},
          "missing current source returns found NPC pointer");

    Runtime npc_to_current;
    npc_to_current.expected_name = "SOURCE-Player-Movment";
    pal4::ScriptPlayerTurn third(npc_to_current);
    Check(third.FaceNpcToCurrent("SOURCE", 7u) == 0x1234u &&
              npc_to_current.calls == std::vector<std::string>{
                  "npc manager", "npc source", "manager",
                  "current player", "source position", "target position",
                  "clamp vector", "angle", "allocate", "executor",
                  "construct", "flush", "allocate text", "name",
                  "read yaw", "action:C02", "effects", "executor",
                  "add", "cleanup"},
          "NPC faces current player with NPC source label");

    Runtime missing_target;
    missing_target.current_exists = false;
    pal4::ScriptPlayerTurn missing_target_turn(missing_target);
    Check(missing_target_turn.FaceNpcToCurrent("SOURCE", 7u) == 0u &&
              missing_target.calls == std::vector<std::string>{
                  "npc manager", "npc source", "manager",
                  "current player"},
          "missing current target skips NPC turn");
}

void ResetNpcCases() {
    Runtime runtime;
    runtime.expected_name = "SOURCE-Player-Movment";
    pal4::ScriptPlayerTurn turn(runtime);
    Check(turn.ResetNpcDirection("SOURCE") == 0x1234u &&
              Near(runtime.state.delta, 21.0f) &&
              runtime.calls == std::vector<std::string>{
                  "npc manager", "npc source", "npc manager", "npc info",
                  "baseline yaw", "allocate", "executor", "construct",
                  "flush", "allocate text", "name", "read yaw",
                  "action:C02", "effects", "executor", "add", "cleanup"},
          "NPC reset turns to info baseline with fixed wait mode one");

    Runtime missing_npc;
    missing_npc.npc_source_exists = false;
    pal4::ScriptPlayerTurn missing_npc_turn(missing_npc);
    Check(missing_npc_turn.ResetNpcDirection("SOURCE") == 0u &&
              missing_npc.calls == std::vector<std::string>{
                  "npc manager", "npc source"},
          "missing NPC does not query baseline info");

    Runtime missing_info;
    missing_info.npc_info_exists = false;
    pal4::ScriptPlayerTurn missing_info_turn(missing_info);
    Check(missing_info_turn.ResetNpcDirection("SOURCE") == 0u &&
              missing_info.calls == std::vector<std::string>{
                  "npc manager", "npc source", "npc manager", "npc info"},
          "missing NPC info does not allocate turn handler");

    Runtime no_handler;
    no_handler.allocation_fails = true;
    pal4::ScriptPlayerTurn no_handler_turn(no_handler);
    Check(no_handler_turn.ResetNpcDirection("SOURCE") == 0x1234u &&
              no_handler.calls == std::vector<std::string>{
                  "npc manager", "npc source", "npc manager", "npc info",
                  "baseline yaw", "allocate", "executor", "add",
                  "cleanup"},
          "baseline reset still queues null after allocation failure");
}

void CurrentToObjectCases() {
    Runtime runtime;
    runtime.current_actor = 0x5000u;
    pal4::ScriptPlayerTurn turn(runtime);
    Check(turn.FaceCurrentToObject("OBJ01", 7u) == 0x1234u &&
              runtime.calls == std::vector<std::string>{
                  "manager", "current player", "object manager",
                  "object lookup", "source position", "target position",
                  "clamp vector", "angle", "manager", "current index",
                  "allocate", "executor", "construct", "flush",
                  "allocate text", "name", "read yaw", "action:C02",
                  "effects", "executor", "add", "cleanup"},
          "current player faces object using current index label");

    Runtime no_current;
    no_current.current_exists = false;
    pal4::ScriptPlayerTurn no_current_turn(no_current);
    Check(no_current_turn.FaceCurrentToObject("OBJ01", 7u) == 0x7000u &&
              no_current.calls == std::vector<std::string>{
                  "manager", "current player", "object manager",
                  "object lookup"},
          "missing current player returns found object pointer");

    Runtime no_object;
    no_object.current_actor = 0x5000u;
    no_object.object_exists = false;
    pal4::ScriptPlayerTurn no_object_turn(no_object);
    Check(no_object_turn.FaceCurrentToObject("OBJ01", 7u) == 0u &&
              no_object.calls == std::vector<std::string>{
                  "manager", "current player", "object manager",
                  "object lookup"},
          "missing object skips facing calculation");

    Runtime aligned;
    aligned.current_actor = 0x5000u;
    aligned.yaw = 41.0f;
    pal4::ScriptPlayerTurn aligned_turn(aligned);
    Check(aligned_turn.FaceCurrentToObject("OBJ01", 7u) == 0x1234u &&
              aligned.calls == std::vector<std::string>{
                  "manager", "current player", "object manager",
                  "object lookup", "source position", "target position",
                  "clamp vector", "angle", "manager", "current index",
                  "allocate", "executor", "construct", "flush",
                  "allocate text", "name", "read yaw", "close",
                  "executor", "add", "cleanup"},
          "already aligned current player closes turn without action");

    Runtime failed;
    failed.current_actor = 0x5000u;
    failed.allocation_fails = true;
    pal4::ScriptPlayerTurn failed_turn(failed);
    Check(failed_turn.FaceCurrentToObject("OBJ01", 7u) == 0x1234u &&
              failed.calls == std::vector<std::string>{
                  "manager", "current player", "object manager",
                  "object lookup", "source position", "target position",
                  "clamp vector", "angle", "manager", "current index",
                  "allocate", "executor", "add", "cleanup"},
          "failed allocation still queues null turn handler");
}

}  // namespace

int main() {
    try {
        StartCases();
        UpdateCases();
        FacePlayerCases();
        FaceNpcCases();
        StartNpcCases();
        FaceNpcToNpcCases();
        FaceNpcToPlayerCases();
        CurrentPlayerFacingCases();
        ResetNpcCases();
        CurrentToObjectCases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
