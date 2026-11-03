#include "pal4/monster_combat_parameter_callback.h"

namespace pal4 {
namespace {

MonsterCombatVector3 ReadVector(
    const std::uint32_t address,
    IMonsterCombatParameterProcessRuntime& runtime) {
    return {runtime.ReadFloat(address), runtime.ReadFloat(address + 4u),
            runtime.ReadFloat(address + 8u)};
}

float Dot(const MonsterCombatVector3& first,
          const MonsterCombatVector3& second) {
    return first.x * second.x + first.y * second.y + first.z * second.z;
}

struct DocumentScope {
    IMonsterCombatParameterProcessRuntime& runtime;
    std::uint32_t document;
    ~DocumentScope() { runtime.DestroyCombatDocument(document); }
};

}  // namespace

std::uint32_t MonsterCombatParameterCallback::Construct(
    const std::uint32_t object,
    IMonsterCombatParameterCallbackRuntime& runtime) {
    runtime.WriteWord(object + 4u, 0u);
    runtime.WriteWord(object, 0x849384u);
    runtime.WriteWord(object + 8u, 0u);
    return object;
}

std::uint32_t MonsterCombatParameterCallback::Process(
    const std::uint32_t callback, const std::uint32_t argument,
    IMonsterCombatParameterProcessRuntime& runtime) {
    if (argument != 0u) return 0u;
    const auto game = runtime.GameInstance();
    const auto state = runtime.ReadWord(game + 396u);
    if (state == 0u || runtime.InvokeGameStateSlot20(state) != 1u)
        return 0u;
    const auto monster = runtime.ReadWord(callback + 8u);
    if (monster == 0u) return 0u;
    const auto actor_control = runtime.ActorControl();
    if (runtime.ReadWord(actor_control + 732u) != 0u ||
        runtime.ReadWord(actor_control + 736u) != 0u)
        return 0u;

    const auto movement = runtime.MovementAdapter();
    runtime.WriteWord(runtime.ReadWord(movement) + 88u, 1u);
    runtime.ClearGameState(game);
    const auto document = runtime.CreateCombatDocument("CombatParam");
    DocumentScope document_scope{runtime, document};
    const auto arena = runtime.ArenaManager();
    const auto base = runtime.ReadCString(runtime.ReadWord(arena + 8u));
    auto world_path = runtime.FloorSoundPath(base);
    if (world_path.empty()) world_path = "x";

    const auto hero = runtime.ReadWord(actor_control + 4u);
    const auto monster_clump = runtime.ReadWord(monster + 20u);
    const auto hero_clump = runtime.ReadWord(hero + 20u);
    const auto monster_forward = ReadVector(
        runtime.ObjectTransform(runtime.ReadWord(monster_clump + 4u)) + 32u,
        runtime);
    const auto hero_forward = ReadVector(
        runtime.ObjectTransform(runtime.ReadWord(hero_clump + 4u)) + 32u,
        runtime);
    const MonsterCombatVector3 delta{
        runtime.ReadFloat(monster + 172u) - runtime.ReadFloat(hero + 172u),
        runtime.ReadFloat(monster + 176u) - runtime.ReadFloat(hero + 176u),
        runtime.ReadFloat(monster + 180u) - runtime.ReadFloat(hero + 180u)};
    const auto direction = runtime.NormalizeDirection(delta);
    const auto facing = Dot(hero_forward, direction);
    auto angle = runtime.AngleFromDot(Dot(monster_forward, hero_forward));
    if (hero_forward.x < monster_forward.x)
        angle = 6.2831855 - angle;
    const auto degrees = angle * 57.29578;

    std::uint32_t world_mode = 0u;
    std::uint32_t combat_mode = 0u;
    if (degrees < 45.0 || degrees > 315.0) {
        if (facing > 0.0f) {
            world_mode = 1u;
            combat_mode = 2u;
        } else {
            world_mode = 2u;
            combat_mode = 1u;
        }
    }
    runtime.SetCombatWorld(document, "combatWorld", world_path,
                           world_mode, 1u);

    const auto behavior = runtime.ReadWord(monster + 1620u);
    // IDA5A9853 jumps over collection and Combat entry when +654 is null.
    if (behavior == 0u) return 0u;
    if (behavior != 0u) {
        const auto script_name = runtime.ReadBehaviorScriptName(
            runtime.ReadWord(behavior + 76u),
            "AAB_MST_ESSBEH_script_func");
        if (!script_name.empty()) {
            runtime.InitializeScript();
            const auto script_path = runtime.ReadCString(
                runtime.ReadWord(arena + 200u));
            const auto cache_key = script_path + "[" + script_name + "]";
            if (!runtime.ScriptCacheContains(cache_key)) {
                const auto interpreter = runtime.CreateScriptInterpreter(
                    runtime.IsCsbMode(), script_path, cache_key,
                    script_name, runtime.ReadWord(arena + 212u));
                if (interpreter != 0u &&
                    runtime.LoadScriptModule(interpreter)) {
                    const auto function =
                        runtime.ScriptFunctionName(interpreter);
                    const auto module_path =
                        runtime.ScriptModulePath(interpreter);
                    runtime.PushScriptModule(module_path, function);
                    runtime.ReleaseInterpreter(interpreter);
                } else {
                    runtime.ShowMissingScriptMessage();
                }
            }
            runtime.CleanupAllScripts();
            if (static_cast<std::int32_t>(
                    runtime.ReadWord(behavior + 64u)) <= 0) {
                runtime.AddActiveCharacter(arena, monster, 0x40000000u);
                runtime.SetClumpRenderCallback(
                    runtime.ReadWord(movement), monster_clump);
                return 0u;
            }
        }
    }
    runtime.DispatchCombatPositions(monster, 1008981770u);
    runtime.InitializeCombatSystem(monster, combat_mode);
    return 0u;
}

}  // namespace pal4
