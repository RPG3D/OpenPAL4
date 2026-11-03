#include "pal4/game_data_effect_table_binding.h"

namespace pal4 {

std::uint32_t GameDataEffectTableBinding::Refresh(
    std::uint32_t table, GameDataEffectTableKind kind,
    IGameDataEffectTableBindingRuntime& runtime) {
    const auto bound_begin = runtime.ReadWord(table + 128);
    const auto bound_count = bound_begin
                                 ? (runtime.ReadWord(table + 132) -
                                    bound_begin) / 4
                                 : 0;
    const auto field_begin = runtime.ReadWord(table + 24);
    const auto field_count = field_begin
                                 ? (runtime.ReadWord(table + 28) -
                                    field_begin) / 60
                                 : 0;
    if (bound_count == field_count)
        return field_begin ? 0 : table + 20;

    static_cast<void>(runtime.LogBind("beginBindField, table:", table + 52));
    runtime.AssignEffects(table + 108, table + 20);
    runtime.InitializeStructures(kind, table + 108);
    runtime.BindEffects(table + 108);
    return runtime.LogBind("endBindField, table:", table + 52);
}

std::uint32_t GameDataEffectTableBinding::RefreshRole(
    std::uint32_t table, IGameDataEffectTableBindingRuntime& runtime) {
    return Refresh(table, GameDataEffectTableKind::role, runtime);
}

std::uint32_t GameDataEffectTableBinding::RefreshUpgrade(
    std::uint32_t table, IGameDataEffectTableBindingRuntime& runtime) {
    return Refresh(table, GameDataEffectTableKind::upgrade, runtime);
}

std::uint32_t GameDataEffectTableBinding::RefreshCombatPrize(
    std::uint32_t table, IGameDataEffectTableBindingRuntime& runtime) {
    return Refresh(table, GameDataEffectTableKind::combat_prize, runtime);
}

}  // namespace pal4
