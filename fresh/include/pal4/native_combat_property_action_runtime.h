#pragma once
#include "pal4/native_combat_message_children.h"
#include <functional>
namespace pal4 {
class NativeCombatSkillActorRegistry;class NativeCombatRosterTargets;class NativeCombatPropertyRuntime;
// Borrowed per-source direct PropertyAction service. Its message children must
// retire before this service; all actor/effect/inventory owners outlive both.
class NativeCombatPropertyActionRuntime final:public INativeCombatMessageChildServices {
public:
    using Motion=std::function<void(std::uint32_t,std::uint32_t)>;
    NativeCombatPropertyActionRuntime(std::uint32_t,NativeCombatSkillActorRegistry&,NativeCombatRosterTargets&,
        NativeEffectSceneSession&,NativeScriptEventRouter&,NativeCombatMessageChildren&,NativeCombatPropertyRuntime&,Motion);
    ~NativeCombatPropertyActionRuntime();
    void Begin(std::uint32_t)override;void Stop(std::uint32_t)override;
    void Tick(std::uint32_t)override;bool Complete(std::uint32_t)override;
    void Command(std::uint32_t,std::string_view)override;
    void Effect(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)override;
    std::size_t Subscriptions()const noexcept;
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
