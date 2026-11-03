#pragma once
#include "pal4/native_particle_tank_runtime.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_particle_frame_runtime.h"
#include "pal4/effect_visual_object.h"
#include "native_particle_probe_camera.h"
#include <rwcore.h>
#include <rpworld.h>
#include <rpptank.h>
#include <bit>
#include <cmath>
#include <source_location>
#include <stdexcept>
#include <string>

namespace pal4::test {
inline void VerifyNativeParticleTank() {
    const auto check=[](bool ok,const std::source_location at=std::source_location::current()) {
        if(!ok) throw std::runtime_error("native particle tank line "+std::to_string(at.line()));
    };
    NativeBaseObjectMemory memory;
    NativeParticleTankRuntime runtime(memory);
    ParticleVisualState state;
    runtime.Construct(state);
    // Real SDK raster/texture ownership, including the material's retained ref.
    auto* raster=RwRasterCreate(2,2,32,rwRASTERTYPETEXTURE|rwRASTERFORMAT8888);
    check(raster!=nullptr);auto* texture=RwTextureCreate(raster);check(texture!=nullptr);
    state.WriteWord(200u,16u);
    (void)ParticleVisualObject::SetProperties(state,0u);
    check(runtime.Initialize(state,true,reinterpret_cast<std::uint32_t>(texture),2u,4.0f,5.0f,6.0f,true));
    auto* atomic=reinterpret_cast<RpAtomic*>(state.ReadWord(40u));
    auto* frame=reinterpret_cast<RwFrame*>(state.ReadWord(44u));
    check(atomic && frame && RpAtomicGetFrame(atomic)==frame && RpPTankAtomicGetMaximumParticlesCount(atomic)==16 &&
        !RpPTankAtomicGetActiveParticlesCount(atomic));
    auto* material=RpGeometryGetMaterial(RpAtomicGetGeometry(atomic),0);
    check(RpMaterialGetTexture(material)==texture && texture->refCount==2);
    check(RwTextureDestroy(texture));texture=nullptr;
    auto* retained=RpMaterialGetTexture(material);check(retained && retained->refCount==1);
    RwBlendFunction src{},dst{};RpPTankAtomicGetBlendModes(atomic,&src,&dst);
    check(src==rwBLENDSRCALPHA && dst==rwBLENDINVSRCALPHA && RpPTankAtomicGetVertexAlpha(atomic));
    state.WriteWord(100u,std::bit_cast<std::uint32_t>(2.0f));
    state.WriteWord(104u,std::bit_cast<std::uint32_t>(3.0f));
    state.WriteWord(108u,std::bit_cast<std::uint32_t>(4.0f));
    state.WriteWord(96u,std::bit_cast<std::uint32_t>(90.0f));
    state.WriteWord(112u,std::bit_cast<std::uint32_t>(5.0f));
    state.WriteWord(116u,std::bit_cast<std::uint32_t>(6.0f));
    state.WriteWord(120u,std::bit_cast<std::uint32_t>(7.0f));
    check(runtime.UpdateFrame(state));
    const auto* matrix=RwFrameGetMatrix(frame);
    check(matrix && std::abs(matrix->right.x)<0.00001f && std::abs(matrix->right.y-2.0f)<0.00001f &&
        std::abs(matrix->up.x+3.0f)<0.00001f && std::abs(matrix->at.z-4.0f)<0.00001f &&
        matrix->pos.x==5.0f && matrix->pos.y==6.0f && matrix->pos.z==7.0f);
    auto* parent=RwFrameCreate();check(parent && RwFrameAddChild(parent,frame));
    runtime.Destroy(state);
    check(!state.ReadByte(0u) && !state.ReadWord(40u) && !state.ReadWord(44u));
    unsigned children{};
    (void)RwFrameForAllChildren(parent,[](RwFrame* f,void* p)->RwFrame* {++*static_cast<unsigned*>(p);return f;},&children);
    check(!children && RwFrameDestroy(parent));
    runtime.Destroy(state); // Repeated teardown has no live SDK resource.
    state.WriteWord(200u,8u);check(runtime.Initialize(state,false,0u,0u,1.0f,1.0f,1.0f,false));
    runtime.Destroy(state);
    // Both original collection origins and parent-frame modes use real
    // geoPlane visuals, retained textures, emission and SDK draw callbacks.
    NativeParticleFrameRuntime frames(memory);
    for(bool point : {false,true}) for(bool parented : {false,true}) {
        runtime.Construct(state);
        raster=RwRasterCreate(2,2,32,rwRASTERTYPETEXTURE|rwRASTERFORMAT8888);
        check(raster!=nullptr);
        auto* pixels=RwRasterLock(raster,0,rwRASTERLOCKWRITE);
        check(pixels!=nullptr);
        for(unsigned y=0;y<2;++y) for(unsigned x=0;x<8;++x) pixels[y*raster->stride+x]=255;
        check(RwRasterUnlock(raster));
        texture=RwTextureCreate(raster);check(texture!=nullptr);
        state.WriteWord(124u,1u);state.WriteWord(200u,4u);state.WriteByte(53u,parented);
        check(runtime.Initialize(state,!point,reinterpret_cast<std::uint32_t>(texture),2u,4,5,6,true));
        check(!state.ReadWord(40u) && state.ReadWord(44u) &&
            state.ReadWord(180u)-state.ReadWord(176u)==16u && texture->refCount==5);
        runtime.SetCollectionEnabled(state,true);
        for(unsigned i=0;i<4;++i) {
            const auto visual=memory.ReadDword(state.ReadWord(176u)+i*4u);
            const auto& v=*reinterpret_cast<const EffectVisualObjectState*>(visual);
            check(v.active && v.enabled && v.frame && v.geometry && v.transform_mode==!point);
            check((RwFrameGetParent(reinterpret_cast<RwFrame*>(v.frame))!=nullptr)==parented);
        }
        NativeParticleProbeCamera camera;camera.Begin();
        (void)frames.Update(state,0.1,reinterpret_cast<std::uint32_t>(camera.camera));
        check(state.ReadWord(204u)>0 && state.ReadWord(208u)>0);
        (void)frames.Draw(state);
        (void)camera.EndPixels();
        runtime.SetCollectionEnabled(state,false);
        check(!memory.ReadByte(memory.ReadDword(state.ReadWord(176u))+16u));
        runtime.Reset(state);runtime.Reset(state);runtime.Destroy(state);runtime.Destroy(state);
        check(texture->refCount==1 && !memory.OutstandingAllocations());
        check(RwTextureDestroy(texture));texture=nullptr;
    }
}
} // namespace pal4::test
