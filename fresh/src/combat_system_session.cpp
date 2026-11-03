#include "pal4/combat_system_session.h"

namespace pal4 {

std::uint32_t CombatSystemSession::Initialize(
    const std::uint32_t object, ICombatSystemColdRuntime& runtime) {
    runtime.LogInitialization(true);
    runtime.GetSkillSystem();
    runtime.LoadTranslator(object + 380u,
                           "gamedata\\database\\combatDb.db");
    const auto console = runtime.GetConsole();
    runtime.SubscribeConsoleEvents(console);
    runtime.InitializeMatFx(object);
    runtime.GetSkillSystem();
    const auto result = runtime.LogInitialized();
    runtime.WriteByte(object + 517u, 1u);
    return result;
}

void CombatSystemSession::End(const std::uint32_t object,
                              ICombatSystemSessionRuntime& runtime) {
    runtime.GetSkillSystem();
    runtime.EndCombat(object);
    runtime.EndCombatSequence(object, 0u);
    runtime.SetRenderObject(runtime.ReadWord(object + 428u));
    runtime.ResetTaskFlags(runtime.GetActorManager() + 96u);
    runtime.CleanupTaskQueue(runtime.GetActorManager() + 96u);
    runtime.DeinitializeGameObject(runtime.ReadWord(object + 424u));
    runtime.DestructActorManager(runtime.GetActorManager());

    const auto manager = runtime.GetActorManager();
    const auto first_begin = runtime.ReadWord(manager + 68u);
    runtime.DestroyCombatDataRange(first_begin,
                                   runtime.ReadWord(manager + 72u));
    runtime.WriteWord(manager + 72u, first_begin);
    const auto second_begin = runtime.ReadWord(manager + 84u);
    runtime.DestroyCombatDataRange(second_begin,
                                   runtime.ReadWord(manager + 88u));
    runtime.WriteWord(manager + 88u, second_begin);

    runtime.DestroyConsole(runtime.GetConsole());
    runtime.CleanupWorld(object);
    runtime.DestructTranslator(object + 380u);
    runtime.InitializeGameStats(runtime.ReadWord(object + 416u));
    runtime.GetSkillSystem();
}

std::uint32_t CombatSystemSession::SetEndState(
    const std::uint32_t object, const std::uint32_t end_value,
    ICombatSystemSessionRuntime& runtime) {
    runtime.WriteWord(object + 508u, 7u);
    runtime.WriteWord(object + 504u, end_value);
    return end_value;
}

}  // namespace pal4
