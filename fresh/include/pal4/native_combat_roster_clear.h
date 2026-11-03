#pragma once
#include <cstdint>
#include <functional>
namespace pal4 {
class NativeCombatPlayerManager;
class NativeCombatMonsterManager;
class NativeCombatTaskQueue;
class NativeEffectSceneSession;
// Native529BD0 composition for Start and EndCombat. Drain tasks, detach domains
// and close actor borrowers first. EndCombat accepts the actual unregistered
// effect group through shared handle release. Collection/session owners stay live
// until any executing EndSequence/timer callback unwinds.
// Call again after callback failure to continue each collection's clear phases.
void ClearNativeCombatRosters(NativeCombatPlayerManager&,NativeCombatMonsterManager&,
    NativeCombatTaskQueue&,NativeEffectSceneSession&,
    const std::function<void(std::uint32_t)>& retire_player,
    const std::function<void(std::uint32_t)>& detach_player);
}
