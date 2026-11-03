#pragma once
#include "pal4/native_scene_sky_cleanup.h"
#include "pal4/native_arena_scene_resource_stage.h"
#include <rwcore.h>
#include <rpworld.h>
#include <rpmatfx.h>
#include <array>
#include <cstring>
#include <stdexcept>

namespace pal4::test {
inline void VerifyNativeSceneSkyCleanup(NativeArenaSceneResourceStage& stage,
                                       std::uint32_t arena) {
    const auto check = [](bool ok) {
        if (!ok) throw std::runtime_error("native dual sky cleanup probe");
    };
    const auto fill = [&](SceneSkyDualState& state) {
        std::array<RpMaterial*,2> observers{};
        unsigned index = 0;
        for (auto* layer : {&state.first,&state.second}) {
            auto* clump = RpClumpCreate();
            auto* frame = RwFrameCreate();
            auto* atomic = RpAtomicCreate();
            auto* geometry = RpGeometryCreate(3,1,rpGEOMETRYPOSITIONS);
            auto* material = RpMaterialCreate();
            auto* raster = RwRasterCreate(4,4,32,rwRASTERTYPETEXTURE|rwRASTERFORMAT8888);
            auto* texture = raster ? RwTextureCreate(raster) : nullptr;
            check(clump && frame && atomic && geometry && material && texture);
            check(RpMatFXMaterialSetEffects(material,rpMATFXEFFECTENVMAP) &&
                  RpMatFXMaterialSetupEnvMap(material,texture,nullptr,TRUE,1.0f));
            check(RwTextureDestroy(texture)); // Material retains the environment texture.
            check(RpGeometryTriangleSetVertexIndices(geometry,&geometry->triangles[0],0,1,2) &&
                  RpGeometryTriangleSetMaterial(geometry,&geometry->triangles[0],material));
            check(RpAtomicSetGeometry(atomic,geometry,0));
            check(RpGeometryDestroy(geometry));
            RpClumpSetFrame(clump,frame);
            RpAtomicSetFrame(atomic,frame);
            check(RpClumpAddAtomic(clump,atomic));
            RpAtomicSetPipeline(atomic,RpAtomicGetDefaultPipeline());
            layer->clump = reinterpret_cast<std::uint32_t>(clump);
            layer->first_matrix = reinterpret_cast<std::uint32_t>(RwMatrixCreate());
            layer->second_matrix = reinterpret_cast<std::uint32_t>(RwMatrixCreate());
            check(layer->first_matrix && layer->second_matrix);
            layer->active = 1;
            layer->first_translation = {1.0f,2.0f,3.0f};
            layer->second_translation = {4.0f,5.0f,6.0f};
            observers[index++] = material; // Initial reference survives geometry destruction.
        }
        return observers;
    };
    auto* image = reinterpret_cast<void*>(arena+320u);
    SceneSkyDualState state;
    std::memcpy(&state,image,sizeof(state));
    check(!state.first.clump && !state.first.first_matrix && !state.first.second_matrix &&
          !state.second.clump && !state.second.first_matrix && !state.second.second_matrix);
    const auto before = *reinterpret_cast<std::uint32_t*>(arena+316u);
    const auto after = *reinterpret_cast<std::uint32_t*>(arena+404u);
    auto observers = fill(state);
    auto expected = state;
    for (auto* layer : {&expected.first,&expected.second}) {
        layer->clump=0; layer->first_matrix=0; layer->second_matrix=0; layer->active=0;
    }
    std::memcpy(image,&state,sizeof(state));
    stage.ReleaseSkyClumps();
    check(std::memcmp(image,&expected,sizeof(expected))==0 &&
          *reinterpret_cast<std::uint32_t*>(arena+316u)==before &&
          *reinterpret_cast<std::uint32_t*>(arena+404u)==after);
    for (auto* material : observers) {
        check(material->refCount==1 && RpMatFXMaterialGetEffects(material)==rpMATFXEFFECTNULL);
        check(RpMaterialDestroy(material));
    }
    stage.ReleaseSkyClumps();
    check(std::memcmp(image,&expected,sizeof(expected))==0);
    // Leave populated owners for normal stage Release and its exception path.
    state=expected;
    observers=fill(state);
    for (auto* material : observers) check(RpMaterialDestroy(material));
    std::memcpy(image,&state,sizeof(state));
}
inline bool NativeSceneSkyResourcesEmpty(std::uint32_t arena) {
    SceneSkyDualState state;
    std::memcpy(&state,reinterpret_cast<const void*>(arena+320u),sizeof(state));
    return !state.first.clump && !state.first.first_matrix && !state.first.second_matrix &&
           !state.first.active && !state.second.clump && !state.second.first_matrix &&
           !state.second.second_matrix && !state.second.active;
}
}  // namespace pal4::test
