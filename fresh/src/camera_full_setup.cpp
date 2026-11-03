#include "pal4/camera_full_setup.h"
#include "pal4/camera_allocation.h"

#include <cstring>

namespace pal4 {

bool CameraFullSetup::Initialize(
    CameraSystemObjectState& owner,
    const CameraSetupParameters& parameters) {
    static_cast<void>(allocation_.Release(owner.camera));
    const auto camera_data = runtime_.AllocateData(0x2ecu);
    if (camera_data == 0u) return false;
    runtime_.ConstructData(camera_data);
    owner.camera = camera_data;

    const auto far_clip = runtime_.FarClip();
    const auto near_clip = runtime_.NearClip();
    const auto initialized = runtime_.InitializeDevice(
        camera_data, parameters.viewport[2], parameters.viewport[3],
        parameters.depth_raster, near_clip, far_clip);

    auto viewport = parameters.viewport;
    const auto mode = runtime_.ViewMode();
    const auto scale = runtime_.ViewScale();
    runtime_.AdjustViewWindow(camera_data, viewport, scale, mode);
    runtime_.SetParameters(camera_data, parameters.target,
                           parameters.angle_y, parameters.angle_x,
                           parameters.angle_z, parameters.distance);
    runtime_.InitializeName(owner.name, parameters.name,
                            std::strlen(parameters.name));
    return initialized;
}

}  // namespace pal4
