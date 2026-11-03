#pragma once

#include "pal4/camera_task_object.h"
#include "pal4/game_math_tables.h"
#include "pal4/native_camera_task_orientation_math.h"

namespace pal4 {

class NativeRenderWareCameraTaskGeometry final : public ICameraTaskGeometry {
public:
    NativeRenderWareCameraTaskGeometry();

    void ClampDirection(CameraTaskVector& direction) override;
    void Normalize(CameraTaskVector& vector) override;
    [[nodiscard]] CameraTaskQuaternion ToQuaternion(
        const CameraTaskBasis& basis) override;
    [[nodiscard]] CameraTaskVector ToEuler(
        const CameraTaskQuaternion& quaternion) override;

private:
    GameMathTables math_;
    NativeCameraTaskOrientationMath orientation_math_;
};

}  // namespace pal4
