#pragma once
#include "pal4/native_gob_object_image.h"
#include "pal4/native_gob_machine_runtime.h"
#include <array>
#include <functional>
#include <memory>
#include <string>

namespace pal4 {
class NativeSceneGobAssets;
class NativeCollisionEventRouter;

struct NativeGobObstacleActorServices {
    std::function<std::uint32_t()> actor_control;
    std::function<void(const std::array<float,3>&)> world_force;
    std::function<void(float)> floor_sound;
    std::function<void(float)> apply_status;
    std::function<void(std::uint32_t,std::uint32_t,float)> start_status;
    std::function<void(std::uint32_t)> movement_status;
    std::function<std::string()> script_path;
};

// Owns only native dispatch. Image, inherited Machine, primary collision
// clump, actor services and effect group retain their original creating owners.
class NativeGobObstacleRuntime final : public INativeGobObstacleService {
public:
    NativeGobObstacleRuntime(NativeGobObjectImage&,NativeGobMachineRuntime&,
        NativeBaseObjectMemory&,NativeSceneGobAssets&,NativeCollisionEventRouter&,
        NativeGobObstacleActorServices,NativeGobMachineServices,INativeGobEffectService&);
    ~NativeGobObstacleRuntime() override;
    std::uint32_t Touch(std::uint32_t event) override;
    void AttachMovement(NativeSceneMovementAsset&) override;
    void DetachMovement(NativeSceneMovementAsset&) override;
    void Advance(float elapsed);
    std::uint32_t HelperIdentity() const noexcept;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
