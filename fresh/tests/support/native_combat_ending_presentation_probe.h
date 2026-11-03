#pragma once
#include "pal4/native_combat_ending_presentation.h"
namespace pal4::test {
void VerifyNativeCombatEndingPresentation(NativeCombatEndingWindow&,const NativeCombatReport&,
    NativeCombatPlayerManager&,NativeInventorySession&,const NativeGameDatabase&,NativeCombatEndingPresentation::ActorSkills);
}
