#pragma once

#include "pal4/camera_task_object.h"

namespace pal4 {
// Public Windows legacy CRT boundary used by the original camera's PC24 path.
// Export addresses and their DLL reference remain valid for the process lifetime.
class NativeCameraTaskOrientationMath final : public ICameraTaskOrientationMath {
public:
    NativeCameraTaskOrientationMath();
    [[nodiscard]] float Arcsine(float value) override;
    [[nodiscard]] float Arctangent2(float y,float x) override;
};
}
