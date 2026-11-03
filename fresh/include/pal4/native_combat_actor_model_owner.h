#pragma once

#include "pal4/native_combat_actor_model_services.h"
#include <cstdint>
#include <memory>
#include <string_view>
#include <array>

namespace pal4 {
struct ResourceVec3;
class NativeActorInstanceOwner;
class NativeCombatActorOwner;
class NativeActorInstancePool;
class NativeMonsterRecordCatalog;
class IActorAnimationControlRuntime;
class NativeScriptEventRouter;
class IEffectHandleReleaseRuntime;

// Composes the existing combat actor and model owners. The pool/SDK outlive it.
// This owns actor808 linkage; it does not register the monster or run its AI.
class NativeCombatActorModelOwner final : public INativeCombatActorModelServices {
public:
    explicit NativeCombatActorModelOwner(NativeActorInstancePool& pool);
    ~NativeCombatActorModelOwner() override;
    NativeCombatActorModelOwner(const NativeCombatActorModelOwner&)=delete;
    NativeCombatActorModelOwner& operator=(const NativeCombatActorModelOwner&)=delete;
    std::uint32_t Construct(const NativeMonsterRecordCatalog&,std::int32_t id);
    [[nodiscard]] std::uint32_t Attach(std::string_view model);
    //531690: attaches the copied record's model and applies the current slot
    // descriptor through the actual SDK transform. Does not dispatch join events.
    [[nodiscard]] std::uint32_t RefreshResource() override;
    void BeginMotion(NativeCombatActorHealthRuntime&,INativeSkillSystemServices&,INativeCombatMonsterRoster*) override;
    void UpdateMotion(std::uint32_t flag,NativeCombatActorHealthRuntime&,
                      INativeSkillSystemServices&,INativeCombatMonsterRoster* = nullptr) override;
    void PlayAction(std::string_view,std::uint8_t,INativeSkillSystemServices&);
    // ActorData child slot52/36: null child returns without drawing.
    void RenderSlot(std::uint32_t camera,std::uint32_t slot);
    // Scene-join model activation; a genuinely absent child is skipped.
    void Activate(std::uint32_t world,std::uint32_t queue,std::uint32_t effect_context=0u);
    void PositionSceneModel(const ResourceVec3&);
    void RotateSceneModel(const ResourceVec3&,bool relative=false);
    //57B520's actor444 -> child4 position step, using existing descriptor data.
    void RestoreFormationPosition();
    std::array<float,3> FormationPosition();
    void RestoreRenderCallbacks();
    void Update(float elapsed,NativeScriptEventRouter&);
    // Borrow only while the current matching child remains attached.
    [[nodiscard]] IActorAnimationControlRuntime& AnimationRuntime();
    [[nodiscard]] std::uint32_t Slot() const;
    void SetSlot(std::uint32_t slot);
    void Detach(std::uint32_t effect_context=0u);
    void Destroy();
    void DestroyAfterEffectShutdown(std::uint32_t group,IEffectHandleReleaseRuntime&);
    [[nodiscard]] std::uint32_t Address() const noexcept override;
    [[nodiscard]] const NativeActorInstanceOwner* Model() const noexcept;
    // Checked mutable borrow for composed native model/animation services.
    [[nodiscard]] NativeActorInstanceOwner& CurrentModel();
    // Borrowed constructor owner for native health/skill adapters. Such adapters
    // must Close before this owner is destroyed; they never own this actor.
    [[nodiscard]] NativeCombatActorOwner& Actor() noexcept;
private:
    void RunMotion(std::uint32_t,NativeCombatActorHealthRuntime&,INativeSkillSystemServices&,INativeCombatMonsterRoster*,bool);
    class Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace pal4
