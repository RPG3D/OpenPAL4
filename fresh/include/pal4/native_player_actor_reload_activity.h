#pragma once

#include "pal4/native_player_actor_reset_runtime.h"

#include <functional>
#include <array>

namespace pal4 {
class NativeActorAnimationStateBinding;
class IBaseObjectMaterialRuntime;

// Resource and animation portion of BaseObjectActivity reset/initialize.
// Material restoration has its own required runtime; equipment/effect cleanup
// and game services remain explicit host effects.
class NativePlayerActorReloadActivity final : public NativePlayerActorResetRuntime {
public:
    using AnimationResolver = std::function<NativeActorAnimationStateBinding&(BaseObjectState&)>;
    NativePlayerActorReloadActivity(NativePlayerActorAssets& actors,
        IBaseObjectActivityRuntime& effects,
        AnimationResolver animation, IBaseObjectMaterialRuntime& materials);
    ~NativePlayerActorReloadActivity() override;
    void ApplyMaterialTexture(BaseObjectState& state) override;
    void CleanupAnimationData(BaseObjectState& state, std::uint32_t offset) override;
    void CleanupClump(BaseObjectState& state, std::uint32_t context) override;
    std::string CreateResourcePath() override;
    bool LoadClump(BaseObjectState& state, std::uint32_t name,
        std::string_view path, std::string_view model, std::uint32_t context,
        std::uint32_t option, std::uint32_t extra, std::uint32_t effects) override;
    void HideHookAtomics(std::uint32_t clump) override;
    void LoadAnimation(BaseObjectState& state, std::uint32_t clump,
        std::string_view path, std::string_view model) override;
    std::uint32_t HookCount(std::uint32_t clump, std::string_view name) override;
    bool LoadShadowClump(BaseObjectState& state, std::uint32_t context) override;
    void AttachAnimation(BaseObjectState& state, std::uint32_t shadow) override;
    void ProcessAtomics(BaseObjectState& state) override;
    void ProcessAtomics2(BaseObjectState& state) override;
private:
    struct Metadata {
        BaseObjectState* object{};
        std::string face, leg;
        // Borrowed metadata views must not discard the slot's constructed
        // StringBuffer allocations. Restore them before slot destruction.
        std::array<std::uint32_t, 3> saved_face{}, saved_leg{};
    };
    Metadata& MaterialMetadata(BaseObjectState& state);
    void ClearMetadata(BaseObjectState& state) noexcept;
    const NativeAnimatedActorAsset& Actor(BaseObjectState& state);
    NativePlayerActorAssets& actors_;
    AnimationResolver animation_;
    IBaseObjectMaterialRuntime& materials_;
    // Object storage must outlive this runtime, as with the actor asset owner.
    std::array<Metadata, 4> metadata_;
};
}  // namespace pal4
