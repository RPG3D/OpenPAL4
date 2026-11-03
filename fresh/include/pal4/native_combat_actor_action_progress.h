#pragma once
#include <cstdint>
#include <memory>

namespace pal4 {
class NativeCombatActorOwner;
class NativeCombatPlayerActorOwner;
class NativeCombatActorHealthRuntime;
class NativeCombatFormulaCatalog;
class NativeScriptEventRouter;
class INativeSkillSystemServices;
// Borrows a real Player or Monster owner, health/property adapter, formulas, router and
// initialized SkillSystem. Action selection/execution are separate services.
class NativeCombatActorActionProgress final {
public:
    NativeCombatActorActionProgress(NativeCombatActorOwner&,NativeCombatActorHealthRuntime&,
        const NativeCombatFormulaCatalog&,NativeScriptEventRouter&,INativeSkillSystemServices&);
    NativeCombatActorActionProgress(NativeCombatPlayerActorOwner&,NativeCombatActorHealthRuntime&,
        const NativeCombatFormulaCatalog&,NativeScriptEventRouter&,INativeSkillSystemServices&);
    ~NativeCombatActorActionProgress();
    [[nodiscard]] std::uint32_t Facet() const;
    [[nodiscard]] float Rate();
    [[nodiscard]] float Progress();
    [[nodiscard]] bool Ready();
    void Prepare();
    void SetProgress(float value);
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
