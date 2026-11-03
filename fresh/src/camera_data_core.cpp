#include "pal4/camera_data.h"

#include <initializer_list>

namespace pal4 {

CameraDataState& CameraData::Construct(CameraDataState& state) {
    state.At(0x0) = 0u;
    runtime_.InitializeNested(state, 0x0cu);
    state.At(0x34) = 0u;
    runtime_.InitializeNested(state, 0x230u);

    state.At(0x48) = 0x42c80000u;
    state.At(0x4c) = 0x42c80000u;
    state.At(0x38) = 1u;
    state.At(0x1a8) = 1u;
    state.At(0x3c) = 0u;
    state.At(0x40) = 0u;
    state.At(0x44) = 0u;
    state.At(0x1ac) = 0u;
    state.At(0x1b0) = 0u;
    state.At(0x8c) = 0u;
    state.At(0x90) = 0u;
    state.At(0x98) = 0x41a00000u;
    state.At(0x9c) = 0x41a00000u;
    state.At(0xa0) = 0u;
    state.At(0x2bc) = 0u;
    state.At(0x1b4) = 0x42d5cccdu;
    state.At(0x1b8) = 0x41980000u;
    state.At(0x1bc) = 0x416f5c29u;
    state.At(0x2b8) = runtime_.NullFunctionSentinel();
    state.At(0x94) = 0x3f800000u;
    return state;
}

void CameraData::Destroy(CameraDataState& state) {
    runtime_.DestroyCamera(state);
    runtime_.DestroyNested(state, 0x0cu);
}

bool CameraDevice::Initialize(CameraDataState& state,
                              std::uint32_t width,
                              std::uint32_t height,
                              bool depth_raster,
                              float near_clip,
                              float far_clip) {
    if (state.At(0) != 0u)
        static_cast<void>(Destroy(state));
    const auto camera = runtime_.CreateCamera();
    state.At(0) = camera;
    if (camera == 0u) {
        static_cast<void>(Destroy(state));
        return false;
    }

    runtime_.AttachFrame(camera, runtime_.CreateFrame());
    runtime_.StoreRaster(camera, 0x60u,
                         runtime_.CreateRaster(width, height, 0u, 2u));
    if (depth_raster)
        runtime_.StoreRaster(camera, 0x64u,
                             runtime_.CreateRaster(width, height, 0u, 1u));
    runtime_.SetNearClip(camera, near_clip);
    runtime_.SetFarClip(camera, far_clip);

    const auto frame = runtime_.CameraFrame(camera);
    if (frame == 0u ||
        runtime_.CameraRaster(camera, 0x60u) == 0u ||
        (depth_raster && runtime_.CameraRaster(camera, 0x64u) == 0u)) {
        static_cast<void>(Destroy(state));
        return false;
    }
    runtime_.TranslateFrame(frame, {0.0f, 0.0f, 0.0f}, 0u);
    return true;
}

std::uint32_t CameraDevice::Destroy(CameraDataState& state) {
    std::uint32_t result = 0u;
    const auto camera = state.At(0);
    if (camera != 0u) {
        const auto world = runtime_.CameraWorld(camera);
        if (world != 0u)
            runtime_.RemoveCameraFromWorld(world, camera);
        const auto frame = runtime_.CameraFrame(camera);
        if (frame != 0u) {
            runtime_.AttachFrame(camera, 0u);
            runtime_.DestroyFrame(frame);
        }
        for (const auto offset : {0x60u, 0x64u}) {
            const auto raster = runtime_.CameraRaster(camera, offset);
            if (raster != 0u) {
                runtime_.StoreRaster(camera, offset, 0u);
                runtime_.DestroyRaster(raster);
            }
        }
        result = runtime_.DestroyCamera(camera);
        state.At(0) = 0u;
    }
    state.At(0x34) = 0u;
    return result;
}

}  // namespace pal4
