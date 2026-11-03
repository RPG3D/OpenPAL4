#pragma once

#include "pal4/base_object_activity.h"
#include "pal4/game_object_embedded_animation.h"

#include <cstdint>

namespace pal4 {

// Handles the first NewGame reset of constructed player slots before any
// actor, weapon, material or effect resource has been loaded. Slot owners keep
// this stable reference; bind the actual loaded lifecycle when it is available.
class NativePlayerInitialResetActivity final :
    public IBaseObjectActivityRuntime {
public:
    NativePlayerInitialResetActivity(
        IGameObjectEmbeddedAnimationRuntime& animation,
        std::uint32_t game_instance)
        : animation_(animation), game_instance_(game_instance) {}
    void BindLoadedActivity(IBaseObjectActivityRuntime* loaded);

    void ApplyMaterialTexture(BaseObjectState& state) override;
    void CleanupObject(BaseObjectState& state) override;
    [[nodiscard]] bool DialogNeedsUpdate(BaseObjectState& state,
                                          std::uint32_t offset) override;
    void UpdateDialog(BaseObjectState& state,std::uint32_t offset) override;
    void CleanupAnimationData(BaseObjectState& state,
                              std::uint32_t offset) override;
    [[nodiscard]] std::uint32_t GameInstance() override;
    void CleanupClump(BaseObjectState& state,
                      std::uint32_t texture_context) override;
    [[nodiscard]] std::uint32_t GetProperty(BaseObjectState& state) override;

private:
    IGameObjectEmbeddedAnimationRuntime& animation_;
    std::uint32_t game_instance_;
    IBaseObjectActivityRuntime* loaded_{};
};

}  // namespace pal4
