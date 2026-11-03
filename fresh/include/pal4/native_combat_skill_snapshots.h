#pragma once
#include <cstdint>
namespace pal4 {
struct ActorSkillActionSnapshot;
class NativeSkillRecordCatalog;
class INativeSkillSystemServices;
//52CA4B evaluates independent base8430E8 copies, never live Combat actors.
void ProcessNativeCombatSkillSnapshots(std::uint32_t skill,ActorSkillActionSnapshot& target,
    ActorSkillActionSnapshot& actor,NativeSkillRecordCatalog&,INativeSkillSystemServices&);
}
