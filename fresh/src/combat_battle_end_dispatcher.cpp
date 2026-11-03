#include "pal4/combat_battle_end_dispatcher.h"

namespace pal4 {

std::uint32_t CombatBattleEndDispatcher::CreateEndTask(
    const std::uint32_t system, const std::uint32_t result,
    ICombatBattleEndRuntime& runtime) {
    const auto object = runtime.Allocate(24u);
    if (object == 0u) return 0u;
    runtime.WriteWord(object, kCallbackVtable);
    runtime.WriteWord(object + 4u, system);
    runtime.WriteWord(object + 8u, kEndCallback);
    runtime.WriteWord(object + 12u, 0u);
    runtime.WriteWord(object + 16u, result);
    return object;
}

std::uint32_t CombatBattleEndDispatcher::ShowRewards(
    const std::uint32_t system, const std::uint32_t result,
    ICombatBattleEndRuntime& runtime) {
    const auto report = runtime.PrepareReport();
    try {
        const auto copy = runtime.CopyReport(report);
        try {
            const auto prize = runtime.ProcessRewards(copy);
            try {
                runtime.WriteWord(prize + 16u, result);
                runtime.ShowEnding(system, prize);
            } catch (...) {
                runtime.DestroyPrize(prize);
                throw;
            }
            runtime.DestroyPrize(prize);
        } catch (...) {
            runtime.DestroyCopiedReport(copy);
            throw;
        }
        runtime.DestroyCopiedReport(copy);
    } catch (...) {
        static_cast<void>(runtime.DestroyReport(report));
        throw;
    }
    return runtime.DestroyReport(report);
}

std::uint32_t CombatBattleEndDispatcher::Handle(
    const std::uint32_t event, ICombatBattleEndRuntime& runtime) {
    const auto initial_system = runtime.CombatSystemInstance();
    const auto state = runtime.ReadWord(initial_system + 508u);
    if (state == 6u) return state;
    runtime.WriteWord(runtime.CombatSystemInstance() + 508u, 6u);

    const auto result = runtime.ReadWord(event);
    if (result == 1u || result == 2u) {
        const auto index = result == 1u ? 0u : 1u;
        const auto game_for_write = runtime.GameDataInstance();
        const auto game_for_read = runtime.GameDataInstance();
        runtime.SetGameEntry(game_for_write, index,
                             runtime.GameEntry(game_for_read, index) + 1u);
    }
    runtime.UnsubscribeConsole(runtime.CombatConsole());

    const auto system = runtime.CombatSystemInstance();
    const auto inactive = runtime.IsCombatInactive(system);
    if (result == 1u) return ShowRewards(system, result, runtime);

    if (!inactive && result == 0u) {
        // 0x57adfa pushes 1.f for 0x4c2570's image scale. The separate
        // delayed animation field +68 and fade end are both 2.f.
        runtime.ShowDefeatBanner("zi0", "zhangdoushibai", 1.0f,
                                 400.0f, 280.0f);
        const auto active_system = runtime.CombatSystemInstance();
        const auto task = CreateEndTask(active_system, result, runtime);
        return runtime.ScheduleClose(active_system, kCloseEventKey,
                                     4.0f, task);
    }

    const auto queue_system = runtime.CombatSystemInstance();
    const auto event_owner = runtime.ReadWord(queue_system + 412u);
    const auto task = CreateEndTask(queue_system, result, runtime);
    if (inactive)
        return runtime.ScheduleClose(event_owner, kCloseEventKey,
                                     0.0f, task);
    runtime.EnqueueClose(event_owner + 12u, kCloseEventKey, task);
    return runtime.CleanupQueuedEvent();
}

}  // namespace pal4
