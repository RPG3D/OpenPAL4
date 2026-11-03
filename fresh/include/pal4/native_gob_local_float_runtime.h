#pragma once

#include "pal4/native_gob_object_image.h"
#include "pal4/native_gob_local_float_memory.h"
#include "pal4/native_movement_secondary_helpers.h"

#include <functional>

namespace pal4 {
class NativeGobMachineRuntime;
class NativeSceneMovementAsset;

struct NativeGobLocalFloatActorServices {
    IGameResourceManagerRuntime* transforms{};
    std::function<std::uint32_t()> actor_control;
    std::function<void(std::uint32_t, const std::array<float, 3>&)> set_position;
    std::function<void(std::uint32_t, std::uint32_t)> update_grid;
    std::function<std::uint32_t()> update_floor;
};

// Borrows the inherited Machine runner; owns its helper's native dispatch only.
class NativeGobLocalFloatRuntime final : public INativeGobPlatformService,
    public INativeMovementSecondaryHelper, private NativeGobLocalFloatMemory,
    private IGobLocalFloatFrameRuntime, private IGobLocalFloatTouchRuntime,
    private IGobLocalFloatMovementRuntime, private IGobLocalFloatTriggerRuntime {
public:
    NativeGobLocalFloatRuntime(NativeGobObjectImage&, NativeGobMachineRuntime&,
        NativeBaseObjectMemory&, NativeGobLocalFloatActorServices);
    ~NativeGobLocalFloatRuntime() override;
    std::uint32_t Touch(std::uint32_t argument) override;
    void AttachMovement(NativeSceneMovementAsset&) override;
    void DetachMovement(NativeSceneMovementAsset&) override;
    void Advance(float elapsed);
    RpClump* Clump() const override;
    double SwitchThreshold() const override;
    void Enter() override;
    void Leave() override;

private:
    void Require(std::uint32_t object) const;
    std::uint32_t TryTriggerMachineTask(std::uint32_t, std::uint32_t) override;
    void UpdateMachineTasks(std::uint32_t, float) override;
    void StorePlatformDelta(const Vector3&, float) override;
    std::uint32_t ActorControlAccess() override;
    void ApplyActorTranslation(std::uint32_t, const Vector3&, std::uint32_t) override;
    void MatrixTranslate(std::uint32_t, const Vector3&, std::uint32_t) override;
    void MatrixRotateY(std::uint32_t, float, std::uint32_t) override;
    void FrameUpdateObjects(std::uint32_t) override;
    void MatrixQueryRotation(std::uint32_t, float&, float&) override;
    void SetActorPosition(std::uint32_t, const Vector3&) override;
    void SetActorTransform(std::uint32_t, const Vector3&, std::uint32_t) override;
    void UpdateMinimapGrid(std::uint32_t, std::uint32_t) override;
    std::uint32_t UpdateFloorAttributes() override;
    std::uint32_t InvokeMachineTrigger(std::uint32_t, const Vector3&) override;
    std::uint32_t MovementAdapterRoot() override;
    std::uint32_t MovementProperty(std::uint32_t) override;
    void SetClumpProperties(std::uint32_t, std::uint32_t, std::uint32_t,
        std::uint32_t, std::uint32_t, std::uint32_t) override;
    void SetClumpRenderCallback(std::uint32_t, std::uint32_t) override;
    std::uint32_t AttachMovementHelper(std::uint32_t, std::uint32_t) override;
    std::uint32_t DetachMovementHelper(std::uint32_t, std::uint32_t) override;

    NativeGobObjectImage& object_;
    NativeGobMachineRuntime& machine_;
    NativeGobLocalFloatActorServices actors_;
    NativeSceneMovementAsset* movement_{};
    bool clump_registered_{}, helper_registered_{};
    std::array<std::uint32_t, 2> movement_root_{};
    Vector3 translation_{};
    float rotation_{};
};
}  // namespace pal4
