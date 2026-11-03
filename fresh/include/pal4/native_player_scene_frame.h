#pragma once

#include "pal4/base_object_activity.h"
#include "pal4/collision_actor.h"
#include "pal4/native_game_object_animation_lifecycle.h"
#include "pal4/player_manager_operations.h"

#include <cstddef>
#include <cstdint>

namespace pal4 {

class NativeActorControlSession;
class NativePlayerActorAssets;
class NativePlayerManagerSession;
class NativePlayerTransformRuntime;
class IAnimationEventQueueActivityRuntime;

struct NativePlayerSceneFrameResult {
    std::uint32_t active_players{};
    std::uint32_t visible_players{};
    std::uint32_t animation_calls{};
    std::uint32_t character_updates{};
};

// Binds the original player-manager render slot to the owned Role and
// CollisionActor objects in a live Scene frame.
class NativePlayerSceneFrame final : private PlayerManagerOperationsRuntimeStub,
                                     private IBaseObjectActivityRuntime,
                                     private NativeGameObjectAnimationLifecycle,
                                     private ICollisionActorCallbackRuntime {
public:
    NativePlayerSceneFrame(NativePlayerManagerSession& players,
                           NativePlayerActorAssets& assets,
                           NativePlayerTransformRuntime& transforms,
                           NativeActorControlSession& control,
                           NativeBaseObjectMemory& memory);

    [[nodiscard]] NativePlayerSceneFrameResult Render(std::uint32_t camera);
    [[nodiscard]] std::uint32_t ProcessCameraPass(std::uint32_t camera);
    //4414C0: actual active slots atAA8, unchanged camera context.
    [[nodiscard]] std::uint32_t DispatchRelatedAnimations(std::uint32_t camera,IAnimationEventQueueActivityRuntime&);

private:
    [[nodiscard]] std::uint32_t ActorControl() override;
    void CallSlotRender(std::uint32_t slot, std::uint32_t virtual_slot,
                        std::uint32_t argument) override;
    [[nodiscard]] std::uint32_t CallSlotUpdate(
        std::uint32_t slot, std::uint32_t virtual_slot,
        std::uint32_t argument) override;
    void RenderDebug(std::uint32_t actor_control) override;

    [[nodiscard]] bool RenderClump(BaseObjectState& player,
                                    std::uint32_t camera) override;
    void ProcessAnimation(BaseObjectState& player, std::uint32_t offset,
                          std::uint32_t argument) override;
    [[nodiscard]] std::uint32_t UpdateCharacter(
        BaseObjectState& player, std::uint32_t offset,
        std::uint32_t parent) override;

    [[nodiscard]] std::uint32_t FindHierarchy(std::uint32_t clump) override;
    void ProcessAnimationData(GameObjectEmbeddedAnimationState& state,
                              std::uint32_t hierarchy,
                              std::int32_t index) override;

    void Output(std::uint32_t callback, std::uint32_t destination,
                std::uint32_t source, std::uint32_t parent) override;
    void Path(std::uint32_t callback, std::uint32_t path,
              std::uint32_t parent) override;
    [[nodiscard]] std::uint32_t Move(
        std::uint32_t callback, std::uint32_t position,
        std::uint32_t point, std::uint32_t parent) override;

    void WriteSlotWord(std::uint32_t offset, std::uint32_t value);

    NativeBaseObjectMemory& memory_;
    NativePlayerManagerSession& players_;
    NativePlayerActorAssets& assets_;
    NativePlayerTransformRuntime& transforms_;
    NativeActorControlSession& control_;
    PlayerManagerSlotState* current_slot_{};
    std::size_t current_index_{};
    std::uint32_t camera_{};
    std::uint32_t camera_pass_calls_{};
    NativePlayerSceneFrameResult result_{};
};

}  // namespace pal4
