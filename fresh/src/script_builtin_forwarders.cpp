#include "pal4/script_builtin_forwarders.h"

namespace pal4 {

std::int32_t ScriptPlayerLevelQuery::Get(
    std::int32_t index, IScriptPlayerLevelRuntime& runtime) {
    const auto player = runtime.PlayerByIndex(runtime.PlayerManager(), index);
    if (player == 0u) return -1;
    return runtime.ReadStat(player + 0x654u, 4u);
}

std::uint32_t ScriptLogoNavigation::Go(
    IScriptLogoNavigationRuntime& runtime) {
    return runtime.EnterState(runtime.GameInstance(), 0u, 0u);
}

std::uint32_t ScriptCameraDistanceOptimization::Set(
    std::uint32_t enabled, IScriptCameraDistanceRuntime& runtime) {
    return runtime.SetDistanceOptimization(enabled & 0xffu);
}

}  // namespace pal4
