#pragma once
#include "pal4/native_combat_message_children.h"
#include "pal4/actor_render_state.h"
#include "pal4/native_combat_skill_kind.h"
#include <functional>
#include <memory>
namespace pal4 {
class NativeCombatSkillActorRegistry;class NativeMagicWindowRecords;
class NativeCombatFormulaCatalog;class NativeCombatDamageCalculator;
class NativeCombatRosterTargets;class NativeEffectSceneSession;class NativeScriptEventRouter;
class ICombatActionTaskRuntime;
// Borrows actual actor/model, effect and skill owners. Message children must
// retire before this service and every borrowed dependency.
class NativeCombatSkillActionRuntime final:public INativeCombatMessageChildServices {
public:
    using EffectFactory=std::function<ActorEffectHandle(std::uint32_t,std::string_view,std::string_view)>;
    using Motion=std::function<void(std::uint32_t,std::uint32_t)>;
    using Wounded=std::function<std::uint32_t(std::uint32_t,std::uint32_t)>;
    using ResetModel=std::function<void(std::uint32_t)>;
    NativeCombatSkillActionRuntime(std::uint32_t,NativeCombatSkillActorRegistry&,const NativeMagicWindowRecords&,
        const NativeCombatFormulaCatalog&,NativeCombatDamageCalculator&,NativeCombatRosterTargets&,
        NativeEffectSceneSession&,NativeScriptEventRouter&,NativeCombatMessageChildren&,
        ICombatActionTaskRuntime&,EffectFactory,Motion,Wounded,
        NativeCombatSkillKind kind=NativeCombatSkillKind::magic,ResetModel reset_model={});
    ~NativeCombatSkillActionRuntime();
    void Begin(std::uint32_t)override;void Stop(std::uint32_t)override;
    void Tick(std::uint32_t)override;bool Complete(std::uint32_t)override;
    void Command(std::uint32_t,std::string_view)override;
    void Effect(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)override;
    std::size_t Subscriptions()const noexcept;
    std::size_t Casts()const noexcept;
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
