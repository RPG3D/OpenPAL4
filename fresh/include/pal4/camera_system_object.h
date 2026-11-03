#pragma once

#include "pal4/string_buffer.h"
#include "pal4/camera_data.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace pal4 {

class CameraAllocation;
class CameraParameterSetters;

struct CameraSystemObjectState {
    std::uint32_t camera{};
    StringBufferState name{};
    std::array<std::uint32_t, 4> tasks{};
};
static_assert(offsetof(CameraSystemObjectState, name) == 4u);
static_assert(offsetof(CameraSystemObjectState, tasks) == 16u);
static_assert(sizeof(CameraSystemObjectState) == 32u);

class ICameraSystemObjectRuntime {
public:
    virtual ~ICameraSystemObjectRuntime() = default;
    virtual void InitializeName(StringBufferState& name) = 0;
    virtual void InitializeTasks(std::array<std::uint32_t, 4>& tasks) = 0;
    virtual void DestroyTasks(std::array<std::uint32_t, 4>& tasks) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupName(
        StringBufferState& name) = 0;
    [[nodiscard]] virtual CameraDataState& CameraData(
        std::uint32_t camera_data) = 0;
    [[nodiscard]] virtual std::array<float, 3> SceneLookAt(
        std::uint32_t scene, std::uint32_t selector) = 0;
    virtual void UpdateTasks(std::array<std::uint32_t, 4>& tasks,
                             float delta) = 0;
};

class CameraSystemObject final {
public:
    CameraSystemObject(ICameraSystemObjectRuntime& runtime,
                       CameraAllocation& allocation,
                       CameraParameterSetters& parameters)
        : runtime_(runtime), allocation_(allocation),
          parameters_(parameters) {}

    // 0x5eaa70: initialize camera slot, name and task container.
    CameraSystemObjectState& Construct(CameraSystemObjectState& state);
    // 0x5eaac0: release camera, tasks and name in that order.
    [[nodiscard]] std::uint32_t Destroy(CameraSystemObjectState& state);
    // 0x5eac40: forward scene parameter to camera data at +0x34.
    [[nodiscard]] std::uint32_t SetSceneParameter(
        CameraSystemObjectState& state, std::uint32_t parameter);
    // 0x5eac50: set camera target from the configured scene object.
    [[nodiscard]] bool SetLookAtFromScene(
        CameraSystemObjectState& state, std::uint32_t selector);
    // 0x5eac60: return the owner's first word.
    [[nodiscard]] std::uint32_t InternalId(
        const CameraSystemObjectState& state) const;
    // 0x5eac80: update tasks before advancing the owned camera.
    [[nodiscard]] std::optional<std::uint32_t> Update(
        CameraSystemObjectState& state, CameraAnimation& animation,
        float delta);
    // 0x5eace0: add three camera angles and update the matrix.
    [[nodiscard]] std::uint32_t AdjustAngles(
        CameraSystemObjectState& state,
        const std::array<float, 3>& delta,
        std::uint32_t mode);
    // 0x5ead20: add to current camera distance and update the matrix.
    [[nodiscard]] std::optional<std::uint32_t> AdjustDistance(
        CameraSystemObjectState& state, float delta,
        std::uint32_t mode);
    // 0x5ead40: add rotation and distance deltas in one matrix update.
    [[nodiscard]] std::uint32_t AdjustAnglesAndDistance(
        CameraSystemObjectState& state,
        const std::array<float, 3>& angle_delta,
        float distance_delta, std::uint32_t mode);

private:
    ICameraSystemObjectRuntime& runtime_;
    CameraAllocation& allocation_;
    CameraParameterSetters& parameters_;
};

}  // namespace pal4
