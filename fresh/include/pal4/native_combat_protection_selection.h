#pragma once
#include <cstdint>
#include <memory>
#include <span>
#include "pal4/check_protect_task.h"
namespace pal4 {
class NativeCombatPlayerManager;
class NativeCombatMonsterManager;
class NativeCombatActorSkillRuntime;
class NativeCombatFormulaCatalog;
class INativeSkillSystemServices;
//563A90/538FC0. Borrows live rosters/skills/formulas and the game's RNG.
// Select returns a real protector or zero; it creates no attack tasks.
class NativeCombatProtectionSelection final : public ICombatProtectionSelectionRuntime {
public:
    NativeCombatProtectionSelection(NativeCombatPlayerManager&,NativeCombatMonsterManager&,
        std::span<NativeCombatActorSkillRuntime* const>,const NativeCombatFormulaCatalog&,
        INativeSkillSystemServices&);
    ~NativeCombatProtectionSelection();
    NativeCombatProtectionSelection(const NativeCombatProtectionSelection&)=delete;
    NativeCombatProtectionSelection& operator=(const NativeCombatProtectionSelection&)=delete;
    std::uint32_t Select(std::uint32_t source,std::uint32_t target) override;
    bool CanProtect(std::uint32_t candidate,std::uint32_t target);
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
