#pragma once
#include "pal4/native_player_initial_reset_activity.h"

namespace pal4 {
class NativePlayerEquipmentSession;

// Effects/memory facet borrowed by NativePlayerActorReloadActivity. The outer
// lifecycle owns material restoration, animation binding release and clump
// destruction; this facet supplies real effect retirement and shared cleanup.
class NativePlayerResetEffectRuntime final : public IBaseObjectActivityRuntime {
public:
    NativePlayerResetEffectRuntime(IGameObjectEmbeddedAnimationRuntime& animation,std::uint32_t game)
        : empty_(animation,game) {}
    void BindEquipment(NativePlayerEquipmentSession* equipment) noexcept { equipment_=equipment; }
    void ApplyMaterialTexture(BaseObjectState& state) override;
    void CleanupObject(BaseObjectState& state) override;
    bool DialogNeedsUpdate(BaseObjectState& state,std::uint32_t offset) override;
    void UpdateDialog(BaseObjectState& state,std::uint32_t offset) override;
    void CleanupAnimationData(BaseObjectState& state,std::uint32_t offset) override;
    std::uint32_t GameInstance() override;
    void CleanupClump(BaseObjectState& state,std::uint32_t context) override;
    std::uint32_t GetProperty(BaseObjectState& state) override;
private:
    NativePlayerInitialResetActivity empty_;
    NativePlayerEquipmentSession* equipment_{};
};
}
