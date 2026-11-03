#pragma once

#include "pal4/native_player_initial_reset_activity.h"
#include "pal4/base_object_named_weapon.h"
#include "pal4/base_object_material_texture.h"

namespace pal4 {

// Resource services are borrowed and must outlive this adapter. Pair with
// NativePlayerActorResetRuntime/ReloadActivity, which unloads the actor owner
// before CleanupClump validates the remaining resource image.
class NativePlayerResetActivity final : public IBaseObjectActivityRuntime {
public:
    NativePlayerResetActivity(IGameObjectEmbeddedAnimationRuntime& animation,
        std::uint32_t game, IBaseObjectNamedWeaponRuntime& weapons,
        IBaseObjectMaterialRuntime& materials)
        : tail_(animation, game), weapons_(weapons), materials_(materials) {}

    void ApplyMaterialTexture(BaseObjectState&) override;
    void CleanupObject(BaseObjectState&) override;
    bool DialogNeedsUpdate(BaseObjectState&, std::uint32_t offset) override;
    void UpdateDialog(BaseObjectState&, std::uint32_t offset) override;
    void CleanupAnimationData(BaseObjectState&, std::uint32_t offset) override;
    std::uint32_t GameInstance() override;
    void CleanupClump(BaseObjectState&, std::uint32_t context) override;
    std::uint32_t GetProperty(BaseObjectState&) override;

private:
    NativePlayerInitialResetActivity tail_;
    IBaseObjectNamedWeaponRuntime& weapons_;
    IBaseObjectMaterialRuntime& materials_;
};

}  // namespace pal4
