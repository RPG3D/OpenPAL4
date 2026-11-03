#include "pal4/script_player_action.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::IScriptPlayerActionRuntime {
public:
    pal4::PlayerActionWaitState action_state{};
    pal4::PlayerEndActionWaitState end_state{};
    std::vector<std::string> calls;
    bool actor_exists = true;
    bool npc_exists = true;
    bool npc_action = false;
    bool allocation_fails = false;
    bool animation_flag = false;
    std::uint8_t expected_action_mode = 7u;
    std::int32_t c01_result = 1;
    std::string animation_name = "run";

    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager");
        return 0x4000u;
    }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::int32_t index) override {
        Check(manager == 0x4000u && index == 2, "indexed player lookup");
        calls.emplace_back("indexed");
        return actor_exists ? 0x5000u : 0u;
    }
    std::uint32_t CurrentPlayer(std::uint32_t manager) override {
        Check(manager == 0x4000u, "current player manager");
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
              "NPC action name lookup");
        calls.emplace_back("npc lookup");
        return npc_exists ? 0x5000u : 0u;
    }
    std::int32_t SetAction(std::uint32_t actor,
                           std::string_view action) override {
        Check(actor == 0x5000u, "action actor");
        calls.emplace_back("action:" + std::string(action));
        return action == "C01" ? c01_result : 1;
    }
    void SetFlag(std::uint32_t actor, std::uint32_t flag) override {
        Check(actor == 0x5000u && flag <= 1u, "set animation flag");
        calls.emplace_back("set:" + std::to_string(flag));
    }
    void ClearFlag(std::uint32_t actor, std::uint32_t flag) override {
        Check(actor == 0x5000u && flag == 0u,
              "clear animation flag");
        calls.emplace_back("clear");
    }
    bool GetFlag(std::uint32_t actor) override {
        Check(actor == 0x5000u, "get animation flag");
        calls.emplace_back("get flag");
        return animation_flag;
    }
    std::string_view ActionName(std::uint32_t actor) override {
        Check(actor == 0x5000u, "animation name actor");
        calls.emplace_back("get name");
        return animation_name;
    }
    void ApplyEquipmentVisuals(std::uint32_t actor) override {
        Check(actor == 0x5000u, "equipment actor");
        calls.emplace_back("equipment");
    }
    void ClearWeaponEffects(std::uint32_t actor) override {
        Check(actor == 0x5000u, "clear effects actor");
        calls.emplace_back("clear effects");
    }
    void UpdateEffectList(std::uint32_t actor) override {
        Check(actor == 0x5000u, "repeat effect list actor");
        calls.emplace_back("effects 1");
    }
    void UpdateEffectList2(std::uint32_t actor) override {
        Check(actor == 0x5000u, "repeat end effect list actor");
        calls.emplace_back("effects 2");
    }
    void UpdateEffectList3(std::uint32_t actor) override {
        Check(actor == 0x5000u, "effect list 3 actor");
        calls.emplace_back("effects 3");
    }
    void UpdateEffectList4(std::uint32_t actor) override {
        Check(actor == 0x5000u, "effect list 4 actor");
        calls.emplace_back("effects 4");
    }
    void CleanupWeaponEffects(std::uint32_t actor) override {
        Check(actor == 0x5000u, "cleanup effects actor");
        calls.emplace_back("cleanup effects");
    }
    std::string_view FormatActorName(std::uint32_t actor) override {
        Check(actor == 0x5000u, "formatted actor name");
        calls.emplace_back("format name");
        return "hero";
    }
    pal4::PlayerActionWaitState* AllocateAction(
        std::uint32_t size) override {
        Check(size == 0x2cu, "action wait allocation size");
        calls.emplace_back("allocate action");
        return allocation_fails ? nullptr : &action_state;
    }
    pal4::PlayerEndActionWaitState* AllocateEnd(
        std::uint32_t size) override {
        Check(size == 0x24u, "end wait allocation size");
        calls.emplace_back("allocate end");
        return allocation_fails ? nullptr : &end_state;
    }
    std::uint32_t ScriptExecutor() override {
        calls.emplace_back("executor");
        return 0x6000u;
    }
    void ConstructAction(pal4::PlayerActionWaitState& state,
                         std::uint8_t mode, std::uint32_t queue) override {
        Check(&state == &action_state && mode == expected_action_mode &&
                  queue == 0x6054u,
              "action wait constructor");
        calls.emplace_back("construct action");
    }
    void ConstructEnd(pal4::PlayerEndActionWaitState& state,
                      std::uint32_t mode, std::uint32_t queue) override {
        Check(&state == &end_state && mode == 1u && queue == 0x6054u,
              "end wait constructor");
        calls.emplace_back("construct end");
    }
    void AssignName(pal4::PlayerActionWaitState& state,
                    std::string_view name) override {
        Check(&state == &action_state &&
                  name == (npc_action ? "NPC01-Player-Movment" : "hero") &&
                  state.vtable_address == 0x84952cu && state.phase == 3u &&
                  state.actor == 0x5000u,
              "action wait state before name assignment");
        calls.emplace_back("assign name");
    }
    void FlushAction(pal4::PlayerActionWaitState& state) override {
        Check(&state == &action_state, "action flush receiver");
        calls.emplace_back("flush action");
    }
    void FlushEnd(pal4::PlayerEndActionWaitState& state) override {
        Check(&state == &end_state && state.actor == 0x5000u &&
                  state.vtable_address == 0x849538u,
              "end wait state before flush");
        calls.emplace_back("flush end");
    }
    void AddAction(std::uint32_t queue,
                   pal4::PlayerActionWaitState* state) override {
        Check(queue == 0x6054u &&
                  state == (allocation_fails ? nullptr : &action_state),
              "action handler queue");
        calls.emplace_back("add action");
    }
    std::uint32_t AddEnd(std::uint32_t queue,
                         pal4::PlayerEndActionWaitState* state) override {
        Check(queue == 0x6054u &&
                  state == (allocation_fails ? nullptr : &end_state),
              "end handler queue");
        calls.emplace_back("add end");
        return 0x1234u;
    }
    std::uint32_t CleanupName() override {
        calls.emplace_back("cleanup name");
        return 0x5678u;
    }
    void CloseAction(pal4::PlayerActionWaitState& state) override {
        Check(&state == &action_state, "close action receiver");
        calls.emplace_back("close action");
    }
    void CloseEnd(pal4::PlayerEndActionWaitState& state) override {
        Check(&state == &end_state, "close end receiver");
        calls.emplace_back("close end");
    }
};

void StartCases() {
    Runtime runtime;
    pal4::ScriptPlayerAction actions(runtime);
    Check(actions.StartIndexed(2, "z03", -1, 7u) == 0x5678u &&
              runtime.action_state.execute_flag == -1 &&
              runtime.action_state.completed_loops == 0u &&
              runtime.calls == std::vector<std::string>{
                  "manager", "indexed", "action:z03", "set:1",
                  "equipment", "format name", "allocate action",
                  "executor", "construct action", "assign name",
                  "effects 3", "flush action", "executor", "add action",
                  "cleanup name"},
          "indexed special action, wait state and effect order");

    Runtime current;
    pal4::ScriptPlayerAction current_actions(current);
    Check(current_actions.StartCurrent("walk", 1, 7u) == 0x5678u &&
              current.calls == std::vector<std::string>{
                  "manager", "current", "action:walk", "set:1",
                  "clear effects", "format name", "allocate action",
                  "executor", "construct action", "assign name",
                  "set:0", "flush action", "executor", "add action",
                  "cleanup name"},
          "current ordinary action and one-cycle flag reset");

    Runtime missing;
    missing.actor_exists = false;
    pal4::ScriptPlayerAction missing_actions(missing);
    Check(missing_actions.StartIndexed(2, "walk", 1, 7u) == 0u &&
              missing.calls == std::vector<std::string>{"manager", "indexed"},
          "missing indexed player returns null lookup");

    Runtime failed;
    failed.allocation_fails = true;
    pal4::ScriptPlayerAction failed_actions(failed);
    Check(failed_actions.StartCurrent("walk", 1, 7u) == 0x5678u &&
              failed.calls == std::vector<std::string>{
                  "manager", "current", "action:walk", "set:1",
                  "clear effects", "format name", "allocate action",
                  "executor", "add action", "cleanup name"},
          "failed action allocation still queues null and cleans name");
}

void ActionUpdateCases() {
    Runtime runtime;
    pal4::ScriptPlayerAction actions(runtime);
    auto& state = runtime.action_state;
    state.actor = 0x5000u;
    state.completed_loops = 9u;
    actions.ResetAction(state);
    Check(state.completed_loops == 0u &&
              runtime.calls == std::vector<std::string>{"close action"},
          "action cleanup clears the loop counter and closes without changing actor action");
    runtime.calls.clear();
    state.execute_flag = -1;
    actions.UpdateAction(state);
    Check(runtime.calls == std::vector<std::string>{"get name"},
          "hold wait remains active for another animation");
    runtime.calls.clear();
    runtime.animation_name = "HOLD";
    state.completed_loops = 9u;
    actions.UpdateAction(state);
    Check(state.completed_loops == 0u &&
              runtime.calls == std::vector<std::string>{
                  "get name", "effects 4", "close action"},
          "case-insensitive hold closes after effect update");

    runtime.calls.clear();
    state.execute_flag = 0;
    actions.UpdateAction(state);
    Check(runtime.calls.empty(), "zero execute flag has no update");

    state.execute_flag = 2;
    runtime.calls.clear();
    actions.UpdateAction(state);
    Check(state.completed_loops == 0u &&
              runtime.calls == std::vector<std::string>{"get flag"},
          "repeat counter waits for animation flag");
    runtime.animation_flag = true;
    actions.UpdateAction(state);
    actions.UpdateAction(state);
    Check(state.completed_loops == 2u, "repeat counter reaches target");
    runtime.calls.clear();
    runtime.c01_result = 0;
    actions.UpdateAction(state);
    Check(state.completed_loops == 0u &&
              runtime.calls == std::vector<std::string>{
                  "action:C01", "action:Z01", "close action"},
          "repeat completion falls back to Z01 and closes");

    runtime.calls.clear();
    state.execute_flag = 1;
    state.completed_loops = 1u;
    runtime.c01_result = 1;
    actions.UpdateAction(state);
    Check(runtime.calls == std::vector<std::string>{
              "action:C01", "set:1", "close action"},
          "one-cycle completion restores animation flag");
}

void EndCases() {
    Runtime runtime;
    pal4::ScriptPlayerAction actions(runtime);
    Check(actions.EndIndexed(2) == 0x1234u &&
              runtime.calls == std::vector<std::string>{
                  "manager", "indexed", "allocate end", "executor",
                  "construct end", "flush end", "clear", "executor",
                  "add end"},
          "indexed end handler setup and queue result");
    runtime.calls.clear();
    runtime.animation_flag = false;
    actions.UpdateEnd(runtime.end_state);
    Check(runtime.calls == std::vector<std::string>{"get flag"},
          "end handler waits for animation flag");
    runtime.calls.clear();
    runtime.animation_flag = true;
    runtime.c01_result = 0;
    actions.UpdateEnd(runtime.end_state);
    Check(runtime.calls == std::vector<std::string>{
              "get flag", "action:C01", "action:Z01", "set:1",
              "cleanup effects", "close end"},
          "end handler restores idle action and cleans weapon effects");

    Runtime failed;
    failed.allocation_fails = true;
    pal4::ScriptPlayerAction failed_actions(failed);
    Check(failed_actions.EndCurrent() == 0x1234u &&
              failed.calls == std::vector<std::string>{
                  "manager", "current", "allocate end", "executor",
                  "add end"},
          "failed current end allocation still queues null");
    failed.calls.clear();
    failed.actor_exists = false;
    Check(failed_actions.EndCurrent() == 0u &&
              failed.calls == std::vector<std::string>{"manager", "current"},
          "missing current actor skips end handler");
    pal4::ScriptPlayerAction::NoOp();
}

void NpcCases() {
    Runtime runtime;
    runtime.npc_action = true;
    pal4::ScriptPlayerAction action(runtime);
    Check(action.StartNpc("NPC01", "run", -1, 7u) == 0x5678u &&
              runtime.calls == std::vector<std::string>{
                  "npc manager", "npc lookup", "action:run", "set:1",
                  "allocate action", "executor", "construct action",
                  "assign name", "effects 3", "flush action", "executor",
                  "add action", "cleanup name"},
          "NPC start uses shared wait object without player visuals");

    Runtime missing;
    missing.npc_exists = false;
    pal4::ScriptPlayerAction missing_action(missing);
    Check(missing_action.StartNpc("", "run", 1, 7u) == 0u &&
              missing.calls == std::vector<std::string>{
                  "npc manager", "npc lookup"},
          "missing NPC skips action and wait handler");

    Runtime end;
    pal4::ScriptPlayerAction end_action(end);
    Check(end_action.EndNpc("NPC01") == 0x1234u &&
              end.calls == std::vector<std::string>{
                  "npc manager", "npc lookup", "allocate end", "executor",
                  "construct end", "flush end", "clear", "executor",
                  "add end"},
          "NPC end shares 36-byte end-action handler");

    Runtime failed;
    failed.allocation_fails = true;
    pal4::ScriptPlayerAction failed_action(failed);
    Check(failed_action.EndNpc("NPC01") == 0x1234u &&
              failed.calls == std::vector<std::string>{
                  "npc manager", "npc lookup", "allocate end",
                  "executor", "add end"},
          "failed NPC end allocation still queues null");
}

void RepeatCases() {
    Runtime player;
    player.expected_action_mode = 0u;
    pal4::ScriptPlayerAction player_action(player);
    Check(player_action.StartRepeatIndexed(2, "Z03") == 0x5678u &&
              player.action_state.execute_flag == 1 &&
              player.action_state.completed_loops == 0u &&
              player.calls == std::vector<std::string>{
                  "manager", "indexed", "action:Z03", "effects 1",
                  "equipment", "format name", "allocate action",
                  "executor", "construct action", "assign name",
                  "set:0", "flush action", "executor", "add action",
                  "cleanup name"},
          "player repeat refreshes effects before visuals and waits in mode zero");

    Runtime ordinary;
    ordinary.expected_action_mode = 0u;
    pal4::ScriptPlayerAction ordinary_action(ordinary);
    Check(ordinary_action.StartRepeatIndexed(2, "walk") == 0x5678u &&
              ordinary.calls[3] == "effects 1" &&
              ordinary.calls[4] == "clear effects",
          "ordinary player repeat clears weapon effects");

    Runtime npc;
    npc.npc_action = true;
    npc.expected_action_mode = 0u;
    pal4::ScriptPlayerAction npc_action(npc);
    Check(npc_action.StartRepeatNpc("NPC01", "run") == 0x5678u &&
              npc.calls == std::vector<std::string>{
                  "npc manager", "npc lookup", "action:run", "effects 1",
                  "allocate action", "executor", "construct action",
                  "assign name", "set:0", "flush action", "executor",
                  "add action", "cleanup name"},
          "NPC repeat shares wait handler without player visuals");

    Runtime end_player;
    pal4::ScriptPlayerAction end_player_action(end_player);
    Check(end_player_action.EndRepeatIndexed(2) == 0x1234u &&
              end_player.calls == std::vector<std::string>{
                  "manager", "indexed", "effects 2", "allocate end",
                  "executor", "construct end", "flush end", "clear",
                  "executor", "add end"},
          "player repeat end refreshes effects before wait allocation");

    Runtime end_npc;
    pal4::ScriptPlayerAction end_npc_action(end_npc);
    Check(end_npc_action.EndRepeatNpc("NPC01") == 0x1234u &&
              end_npc.calls == std::vector<std::string>{
                  "npc manager", "npc lookup", "effects 2", "allocate end",
                  "executor", "construct end", "flush end", "clear",
                  "executor", "add end"},
          "NPC repeat end shares handler after effect refresh");

    Runtime missing;
    missing.npc_exists = false;
    pal4::ScriptPlayerAction missing_action(missing);
    Check(missing_action.StartRepeatNpc("NPC01", "run") == 0u &&
              missing.calls == std::vector<std::string>{
                  "npc manager", "npc lookup"},
          "missing NPC skips repeat action");
    missing.calls.clear();
    Check(missing_action.EndRepeatNpc("NPC01") == 0u &&
              missing.calls == std::vector<std::string>{
                  "npc manager", "npc lookup"},
          "missing NPC skips repeat end refresh");

    Runtime failed;
    failed.allocation_fails = true;
    failed.expected_action_mode = 0u;
    pal4::ScriptPlayerAction failed_action(failed);
    Check(failed_action.StartRepeatIndexed(2, "walk") == 0x5678u &&
              failed.calls == std::vector<std::string>{
                  "manager", "indexed", "action:walk", "effects 1",
                  "clear effects", "format name", "allocate action",
                  "executor", "add action", "cleanup name"},
          "repeat start queues null after allocation failure");
    failed.calls.clear();
    Check(failed_action.EndRepeatIndexed(2) == 0x1234u &&
              failed.calls == std::vector<std::string>{
                  "manager", "indexed", "effects 2", "allocate end",
                  "executor", "add end"},
          "repeat end queues null after allocation failure");
}

}  // namespace

int main() {
    try {
        StartCases();
        ActionUpdateCases();
        EndCases();
        NpcCases();
        RepeatCases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
