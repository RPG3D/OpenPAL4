#pragma once
#include "pal4/dead_effect_task.h"
#include "pal4/fade_off_task.h"
namespace pal4 {
class NativeCombatActorModelOwner;
class NativeCombatActorSkillRuntime;
class NativeEffectSceneSession;
// Borrow the actual actor/model, lifecycle and Combat effect group. Retire
// tasks before these dependencies; group playback owns the temporary EFF6.
class NativeCombatFatalVisualRuntime final : public IDeadEffectTaskRuntime,public IFadeOffTaskRuntime {
public:
    NativeCombatFatalVisualRuntime(NativeCombatActorModelOwner&,NativeCombatActorSkillRuntime&,
                                  NativeEffectSceneSession&,std::uint32_t combat);
    std::uint32_t ReadWord(std::uint32_t) override;
    void WriteWord(std::uint32_t,std::uint32_t) override;
    float ReadFloat(std::uint32_t) override;
    void WriteFloat(std::uint32_t,float) override;
    void SetActorState(std::uint32_t,std::uint32_t,std::uint32_t) override;
    std::int32_t FloatToLong(float) override;
    std::uint32_t ActorFacet(std::uint32_t) override;
    std::uint32_t SetFacetColor(std::uint32_t,std::uint32_t) override;
    std::uint32_t CombatSystem() override;
    std::uint32_t CombatCamera(std::uint32_t) override;
    DeadEffectHandle CreateEffect(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) override;
    std::uint32_t ChildPosition(std::uint32_t) override;
    void UpdatePosition(const DeadEffectHandle&,std::uint32_t,std::uint8_t) override;
    void UpdateScale(const DeadEffectHandle&,const std::array<float,3>&,std::uint8_t) override;
    void FinalizeEffect(const DeadEffectHandle&) override;
    void ResetTemporaryEffect(DeadEffectHandle&) override;
private:
    void CheckActor(std::uint32_t) const;
    NativeCombatActorModelOwner& model_;
    NativeCombatActorSkillRuntime& skills_;
    NativeEffectSceneSession& effects_;
    std::uint32_t combat_;
};
}
