#pragma once

#include "pal4/base_object_activity.h"
#include "pal4/native_player_actor_assets.h"

namespace pal4 {

// Wraps the game object's reset calls while the SDK actor owner holds its clumps.
class NativePlayerActorResetRuntime : public IBaseObjectActivityRuntime {
public:
    NativePlayerActorResetRuntime(NativePlayerActorAssets& actors,
                                  IBaseObjectActivityRuntime& host)
        : actors_(actors), host_(host) {}

    void ApplyMaterialTexture(BaseObjectState& state) override;
    void CleanupObject(BaseObjectState& state) override;
    [[nodiscard]] bool DialogNeedsUpdate(BaseObjectState& state,
                                          std::uint32_t offset) override;
    void UpdateDialog(BaseObjectState& state, std::uint32_t offset) override;
    void CleanupAnimationData(BaseObjectState& state,
                              std::uint32_t offset) override;
    [[nodiscard]] std::uint32_t GameInstance() override;
    void CleanupClump(BaseObjectState& state,
                      std::uint32_t texture_context) override;
    [[nodiscard]] std::uint32_t GetProperty(BaseObjectState& state) override;

private:
    NativePlayerActorAssets& actors_;
    IBaseObjectActivityRuntime& host_;
};

}  // namespace pal4
