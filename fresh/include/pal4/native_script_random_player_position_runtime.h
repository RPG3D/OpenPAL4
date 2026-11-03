#pragma once

#include "pal4/script_random_player_position.h"

namespace pal4 {
class NativeScriptPlayerPositionRuntime;
class NativePlayerTransformRuntime;
class NativeMinimapManagerOwner;
class NativeScenePlayerMovement;
class NativeSceneEvfAssets;

// Borrows existing player, scene collision and SDK transform owners. Scene
// bindings are replaced after load and detached before their owners retire.
class NativeScriptRandomPlayerPositionRuntime final
    : public IScriptRandomPlayerPositionRuntime {
public:
    NativeScriptRandomPlayerPositionRuntime(NativeScriptPlayerPositionRuntime&,
        NativePlayerTransformRuntime&, NativeMinimapManagerOwner&,
        NativeScenePlayerMovement&, NativeSceneEvfAssets&);
    std::uint32_t PlayerManager() override;
    std::uint32_t PlayerByIndex(std::uint32_t, std::int32_t) override;
    std::uint32_t ReadWord(std::uint32_t) override;
    float ReadFloat(std::uint32_t) override;
    double RandomComponent(float) override;
    double RandomRadius(float, float, float) override;
    std::uint32_t CollisionWorld() override;
    void ForEachCollisionObject(std::uint32_t, ScriptCollisionProbe&) override;
    std::uint32_t ProcessClumpCollision(const std::array<float,6>&,
        std::uint32_t, std::array<std::uint8_t,24>&) override;
    double ObjectProperty(std::uint32_t) override;
    std::uint32_t MovementAdapter() override;
    bool CheckMovement(std::uint32_t, std::array<float,3>&, float, float) override;
    void SetTransform(std::uint32_t, const std::array<float,3>&, std::uint32_t) override;
    std::uint32_t MinimapManager() override;
    std::uint32_t UpdateGrid(std::uint32_t, std::uint32_t, std::uint32_t) override;
private:
    NativeScriptPlayerPositionRuntime& players_;
    NativePlayerTransformRuntime& transforms_;
    NativeMinimapManagerOwner& minimap_;
    NativeScenePlayerMovement& movement_;
    NativeSceneEvfAssets& events_;
};
} // namespace pal4
