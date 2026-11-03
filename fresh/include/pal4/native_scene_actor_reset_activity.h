#pragma once
#include "pal4/native_player_reset_effect_runtime.h"
#include <functional>

namespace pal4 {
class NativeAnimatedActorAsset;
class IBaseObjectMaterialRuntime;
class IBaseObjectAttachedEffectRuntime;
// Borrow actor resources from their creating owners during synchronous reset.
class NativeSceneActorResetActivity final : public IBaseObjectActivityRuntime {
public:
    NativeSceneActorResetActivity(NativeAnimatedActorAsset& actor,
        IGameObjectEmbeddedAnimationRuntime& animation, std::uint32_t game,
        IBaseObjectMaterialRuntime& materials, std::function<void()> release_animation,
        std::function<void()> unbind_actor, IBaseObjectAttachedEffectRuntime* attached = nullptr);
    void ApplyMaterialTexture(BaseObjectState&) override;
    void CleanupObject(BaseObjectState&) override;
    bool DialogNeedsUpdate(BaseObjectState&,std::uint32_t) override;
    void UpdateDialog(BaseObjectState&,std::uint32_t) override;
    void CleanupAnimationData(BaseObjectState&,std::uint32_t) override;
    std::uint32_t GameInstance() override;
    void CleanupClump(BaseObjectState&,std::uint32_t) override;
    std::uint32_t GetProperty(BaseObjectState&) override;
private:
    NativeAnimatedActorAsset& actor_;
    NativePlayerInitialResetActivity tail_;
    NativePlayerResetEffectRuntime effects_;
    IBaseObjectMaterialRuntime& materials_;
    std::function<void()> release_animation_, unbind_actor_;
    IBaseObjectAttachedEffectRuntime* attached_{};
};
}
