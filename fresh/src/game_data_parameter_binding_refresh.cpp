#include "pal4/game_data_parameter_binding_refresh.h"

namespace pal4 {
namespace {

std::uint32_t Refresh(std::uint32_t table, bool drop,
                      IGameDataParameterBindingRefreshRuntime& runtime) {
    const auto bound_begin = runtime.ReadWord(table + 128);
    const auto bound_count = bound_begin
                                 ? (runtime.ReadWord(table + 132) - bound_begin) /
                                       4
                                 : 0;
    const auto field_begin = runtime.ReadWord(table + 24);
    const auto field_count = field_begin
                                 ? (runtime.ReadWord(table + 28) - field_begin) /
                                       60
                                 : 0;
    if (bound_count == field_count)
        return field_begin ? 0 : table + 20;

    const auto log = runtime.BeginLog("beginBindField, table:", table + 52);
    runtime.AssignEffects(table + 108, table + 20);
    if (drop)
        runtime.InitializeDropStructures(table + 108);
    else
        runtime.InitializeUpgradeStructures(table + 108);
    runtime.BindEffects(table + 108);
    runtime.EndLog(log, "endBindField, table:", table + 52);
    return log ? runtime.ReleaseLog(log) : 0;
}

}  // namespace

std::uint32_t GameDataParameterBindingRefresh::RefreshUpgrade(
    std::uint32_t table, IGameDataParameterBindingRefreshRuntime& runtime) {
    return Refresh(table, false, runtime);
}

std::uint32_t GameDataParameterBindingRefresh::RefreshDrop(
    std::uint32_t table, IGameDataParameterBindingRefreshRuntime& runtime) {
    return Refresh(table, true, runtime);
}

}  // namespace pal4
