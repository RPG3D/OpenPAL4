#pragma once
#include <cstdint>
#include <cstddef>
#include <memory>
#include <span>
namespace pal4 {
class NativeCombatPlayerManager;
class NativeCombatMonsterManager;
class NativeCombatActorSkillRuntime;
class NativeCombatEventHandlers;
class NativeScriptEventRouter;
struct NativeCombatSystemImage;
//57B5E0/57B9B0. Borrows actual rosters, skills, handler flag and state/router.
// Routes remain owned by NativeCombatEventHandlers. Does not own battle outcome.
class NativeCombatActionEnd final {
public:
    NativeCombatActionEnd(NativeCombatPlayerManager&,NativeCombatMonsterManager&,
        std::span<NativeCombatActorSkillRuntime* const>,NativeCombatEventHandlers&,
        NativeCombatSystemImage&,NativeScriptEventRouter&);
    ~NativeCombatActionEnd();
    NativeCombatActionEnd(const NativeCombatActionEnd&)=delete;
    NativeCombatActionEnd& operator=(const NativeCombatActionEnd&)=delete;
    std::uint32_t Handle(std::uint32_t owner,std::uint32_t payload);
    std::uint32_t CheckOutcome();
    void Close();
    std::size_t ActiveIterators() const noexcept;
private:
    class Impl;
    std::shared_ptr<Impl> impl_;
};
}
