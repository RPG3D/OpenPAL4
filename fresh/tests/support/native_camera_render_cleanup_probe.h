#pragma once

#include "pal4/base_object_lifecycle.h"
#include "pal4/camera_resource_native.h"
#include "pal4/native_camera_render_cleanup.h"
#include <rwcore.h>
#include <rpworld.h>
#include <array>
#include <cstring>
#include <stdexcept>

namespace pal4::test {
inline void VerifyNativeCameraRenderCleanup() {
    const auto check = [](bool ok) {
        if (!ok) throw std::runtime_error("native resource camera cleanup probe");
    };
    RwBBox bounds{{100.0f,100.0f,100.0f},{-100.0f,-100.0f,-100.0f}};
    auto* world = RpWorldCreate(&bounds);
    auto* borrowed_frame = RwFrameCreate();
    check(world && borrowed_frame);
    NativeCameraResourceRuntime cameras;
    CameraRenderState state;
    state.camera = CameraResourceHelpers::Create(16u, cameras);
    state.auxiliary_camera = CameraResourceHelpers::Create(16u, cameras);
    check(state.camera && state.auxiliary_camera);
    auto* camera = reinterpret_cast<RwCamera*>(state.camera);
    check(RpWorldAddCamera(world, camera) == world);
    const auto raster = [&] {
        auto* result = RwRasterCreate(16,16,32,
            rwRASTERTYPECAMERATEXTURE|rwRASTERFORMAT8888);
        check(result != nullptr);
        return reinterpret_cast<std::uint32_t>(result);
    };
    state.primary_raster = raster();
    state.stereo_raster = raster();
    state.effects_raster = raster();
    state.current_raster = state.effects_raster;
    RwCameraSetRaster(camera, reinterpret_cast<RwRaster*>(state.primary_raster));
    RwCameraSetRaster(reinterpret_cast<RwCamera*>(state.auxiliary_camera),
                       reinterpret_cast<RwRaster*>(state.stereo_raster));
    auto* texture = RwTextureCreate(reinterpret_cast<RwRaster*>(state.primary_raster));
    check(texture != nullptr);
    RwTextureAddRef(texture); // Keep an observer reference across game cleanup.
    state.texture = reinterpret_cast<std::uint32_t>(texture);
    std::array<unsigned,2> order{};
    unsigned count{};
    struct Probe {
        unsigned tag;
        unsigned* count;
        std::array<unsigned,2>* order;
        RwTexture* texture;
        RwCamera* camera;
        RpWorld* world;
        bool correct{};
    } probes[2]{{1u,&count,&order,texture,camera,world},
                {2u,&count,&order,texture,camera,world}};
    RxNodeDefinition definition{};
    definition.name = const_cast<char*>("resource-camera-cleanup-probe");
    definition.editable = rxNODEDEFCONST;
    definition.pipelineNodePrivateDataSize = sizeof(Probe*);
    definition.nodeMethods.nodeBody =
        [](RxPipelineNode*,const RxPipelineNodeParam*) -> RwBool { return TRUE; };
    definition.nodeMethods.pipelineNodeTerm = [](RxPipelineNode* node) {
        auto& p = **static_cast<Probe**>(node->privateData);
        p.correct = p.texture->refCount == 1 && !RwTextureGetRaster(p.texture) &&
                    RwCameraGetWorld(p.camera) == p.world;
        if (*p.count < p.order->size()) (*p.order)[(*p.count)++] = p.tag;
    };
    const auto pipeline = [&](Probe& probe) {
        auto* result = RxPipelineCreate();
        check(result != nullptr);
        auto* locked = RxPipelineLock(result);
        check(locked && RxLockedPipeAddFragment(locked,nullptr,&definition,
                  static_cast<RxNodeDefinition*>(nullptr)) &&
              RxLockedPipeUnlock(locked) == result);
        *static_cast<Probe**>(result->nodes[0].privateData) = &probe;
        return reinterpret_cast<std::uint32_t>(result);
    };
    state.first_pipeline = pipeline(probes[0]);
    state.second_pipeline = pipeline(probes[1]);
    state.world = reinterpret_cast<std::uint32_t>(world);
    state.source_frame = reinterpret_cast<std::uint32_t>(borrowed_frame);
    state.clump = 0x1234u;
    state.dimensions = 0x5678u;
    state.target = 0x9012u;
    state.active = 1u;
    state.maximum[0] = 12.0f;
    state.minimum[1] = -8.0f;
    state.frustum_type = 3u;
    BaseObjectState object;
    object.Write(0x34u, 0xabcdefu);
    object.Write(0x90u, 0x123456u);
    auto* image = object.words.data() + 0x38u/4u;
    std::memcpy(image,&state,sizeof(state));
    NativeCameraRenderCleanup::CleanupResource(object);
    CameraRenderState expected;
    expected.maximum[0] = 12.0f;
    expected.minimum[1] = -8.0f;
    expected.frustum_type = 3u;
    check(std::memcmp(image,&expected,sizeof(expected)) == 0 &&
          object.Read(0x34u) == 0xabcdefu && object.Read(0x90u) == 0x123456u &&
          count == 2u && order == std::array<unsigned,2>{2u,1u} &&
          probes[0].correct && probes[1].correct);
    NativeCameraRenderCleanup::CleanupResource(object);
    check(count == 2u && texture->refCount == 1 && !RwTextureGetRaster(texture));
    check(RwTextureDestroy(texture) && RwFrameDestroy(borrowed_frame) &&
          RpWorldDestroy(world));
}
}  // namespace pal4::test
