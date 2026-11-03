#pragma once

#include "pal4/native_script_player_position_runtime.h"
#include "pal4/pal_movement_adapter.h"
#include "pal4/collision_segment_geometry.h"
#include "pal4/config_composite_collision.h"

struct RpAtomic;
struct RpClump;

namespace pal4 {

class NativeSceneMovementAsset;
struct NativeClumpCollisionHit;
class INativeCollisionEventCallbacks;

// Binds the original movement adapter's primary child to the scene floor.
class NativeScenePlayerMovement final : public INativeScriptPlayerMovement,
                                        private PalMovementAdapterRuntimeStub {
public:
    explicit NativeScenePlayerMovement(NativeSceneMovementAsset& scene);
    void BindEventCallbacks(INativeCollisionEventCallbacks* events) noexcept {
        events_ = events;
    }

    // Drop borrowed world hits/callbacks before their creating scene is released.
    void DetachScene() noexcept;
    [[nodiscard]] std::uint32_t Address() const noexcept override;
    void SelectAndUpdate(std::uint32_t movement, float above, float below,
                         std::uint32_t mode) override;
    [[nodiscard]] bool CheckActive(std::uint32_t movement, float above,
                                   float below, std::uint32_t mode) override;
    void ResolveCollision(ConfigCollisionPosition& lower,
                          ConfigCollisionPosition& upper,
                          CollisionPoint3& displacement,
                          std::uint32_t actor_object);
    void ResolveCollisionModes(ConfigCollisionPosition& lower,
        ConfigCollisionPosition& upper,CollisionPoint3& displacement,
        std::uint32_t actor_object,std::uint32_t first,std::uint32_t collision,
        std::uint32_t third);
    bool ProbeGround(const CollisionPoint3&,float above,float below,
        CollisionPoint3& output,bool active_only);
    [[nodiscard]] RpClump* ActiveCollisionClump();
    [[nodiscard]] RpAtomic* FloorAtomic() const noexcept {
        return floor_atomic_;
    }
    [[nodiscard]] std::int32_t FloorTriangle() const noexcept {
        return floor_triangle_;
    }

private:
    std::uint32_t DispatchPrimary(
        std::uint32_t child,
        const std::array<std::uint32_t, 7>& arguments) override;
    std::uint32_t ProbeChild(std::uint32_t child, std::uint32_t fixed_first,
                             std::uint32_t fixed_second, std::uint32_t movement,
                             std::uint32_t above, std::uint32_t below,
                             PalMovementProbeScratch& scratch) override;
    std::uint32_t CanSwitch(std::uint32_t child, std::uint32_t movement,
                            std::uint32_t above, std::uint32_t below,
                            std::uint32_t mode) override;
    std::uint32_t UpdateChild(std::uint32_t child, std::uint32_t movement,
                              std::uint32_t above, std::uint32_t below) override;
    std::uint32_t ProbeActive(std::uint32_t child, std::uint32_t fixed_first,
                              std::uint32_t fixed_second,
                              const std::array<std::uint32_t, 4>& args) override;
    void DeactivateChild(std::uint32_t child) override;
    bool FindSecondary(const CollisionPoint3&, float above, float below,
        NativeClumpCollisionHit& hit, bool store_fields = true);

    [[nodiscard]] static CollisionPoint3 Position(std::uint32_t address);

    NativeSceneMovementAsset& scene_;
    INativeCollisionEventCallbacks* events_ = nullptr;
    PalMovementAdapterState state_{};
    std::uint32_t primary_child_ = 0u;
    std::uint32_t secondary_child_ = 0u;
    RpAtomic* floor_atomic_ = nullptr;
    std::int32_t floor_triangle_ = -1;
    bool secondary_platform_{};
};

}  // namespace pal4
