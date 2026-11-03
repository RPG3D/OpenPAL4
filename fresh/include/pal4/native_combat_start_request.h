#pragma once
#include <cstdint>
namespace pal4 {
struct NativeCombatSystemImage;
class NativeCombatTaskQueue;
class INativeSkillSystemServices;
// Original5756D0 prefix: acquire SkillSystem, reset/clean tasks, initialize
// per-request fields. Tail construction must precede this; cold init follows.
// Does not perform XML validation, world/UI loading or ready-state commit.
void InitializeNativeCombatStartRequest(NativeCombatSystemImage&,
    NativeCombatTaskQueue&,INativeSkillSystemServices&,std::uint32_t mode);
}
