#pragma once
#include "pal4/native_combat_sequence.h"
#include <span>
namespace pal4 {
class NativeCombatPlayerManager;
class NativeCombatMonsterManager;
class NativeCombatActorSkillRuntime;
// Uses actual stage lists for icons and first Monster list for the count.
NativeCombatSequence CollectNativeCombatSequence(NativeCombatPlayerManager&,
    NativeCombatMonsterManager&,std::span<NativeCombatActorSkillRuntime* const> player_skills);
}
