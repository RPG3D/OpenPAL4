#pragma once
#include <cstdint>
#include <memory>
#include <span>
namespace pal4 {
class NativeCombatPlayerManager;
class NativeCombatMonsterManager;
class NativeCombatActorSkillRuntime;
class NativeCombatFormulaCatalog;
class INativeSkillSystemServices;
class NativeScriptEventRouter;
//558290/5582A0 over actual actor skills and the Player stage roster. Borrows
// all dependencies; Process borrows the real68-byte EndAttack child image.
class NativeCombatJointAttack final {
public:
    NativeCombatJointAttack(NativeCombatPlayerManager&,NativeCombatMonsterManager&,
        std::span<NativeCombatActorSkillRuntime* const>,const NativeCombatFormulaCatalog&,
        INativeSkillSystemServices&,NativeScriptEventRouter&);
    ~NativeCombatJointAttack();
    NativeCombatJointAttack(const NativeCombatJointAttack&)=delete;
    NativeCombatJointAttack& operator=(const NativeCombatJointAttack&)=delete;
    void Process(std::uint32_t child);
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
