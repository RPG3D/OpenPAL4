#pragma once
#include "pal4/combat_status_tasks.h"
namespace pal4 {
class NativeCombatSkillActorRegistry;
// Borrowed actual routing domain for FendOff's source508(target) callback.
// Read/actor-call only; does not invent defend/flee/death state services.
class NativeCombatActorReactionServices final : public ICombatStatusActorRuntime {
public:
    explicit NativeCombatActorReactionServices(NativeCombatSkillActorRegistry&);
    std::uint32_t ReadWord(std::uint32_t) override;
    void CallActor(std::uint32_t actor,std::uint32_t slot,std::uint32_t target) override;
private:
    NativeCombatSkillActorRegistry& registry_;
};
}
