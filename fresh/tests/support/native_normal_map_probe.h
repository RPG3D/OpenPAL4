#pragma once
#include "pal4/native_clump_normal_map.h"
#include "pal4/native_gob_clump_asset.h"
#include <rpnormmap.h>
#include <bit>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeNormalMap(const std::filesystem::path& root){
    const auto check=[](bool ok,const char* m){if(!ok)throw std::runtime_error(m);};
    const auto address=[](const void* p){return reinterpret_cast<std::uint32_t>(p);};
    NativeGobClumpAsset asset;
    check(asset.Load(root,"gamedata\\PALWeapon\\WTA\\","WTA"),"normal-map original WTA fixture");
    auto* pipeline=RpNormMapGetAtomicPipeline(rpNORMMAPATOMICSTATICPIPELINE);
    check(pipeline,"normal-map static SDK pipeline");
    std::array<std::uint32_t,3> config{0u,std::bit_cast<std::uint32_t>(1.0f),0u};
    check(!NativeClumpNormalMap::Apply(0u,config),"normal-map null clump gate");
    check(NativeClumpNormalMap::Apply(address(asset.Clump()),config) && config[0],"normal-map generates missing texture from real diffuse");
    struct Texture {RwTexture* p;~Texture(){if(p)RwTextureDestroy(p);}} generated{reinterpret_cast<RwTexture*>(config[0])};
    check(RwRasterGetNumLevels(RwTextureGetRaster(generated.p))>0,"normal-map generated raster has mip levels");
    struct Inspect {RwTexture* normal;RwTexture* env{};RwFrame* frame{};RxPipeline* pipeline;unsigned materials{},atomics{};bool valid{true};};
    const auto inspect=[](RpAtomic* a,void* p)->RpAtomic*{
        auto& r=*static_cast<Inspect*>(p);++r.atomics;r.valid &= a->pipeline==r.pipeline && RpNormMapAtomicIsInitialized(a);
        RpGeometryForAllMaterials(RpAtomicGetGeometry(a),[](RpMaterial* m,void* p)->RpMaterial*{
            auto& r=*static_cast<Inspect*>(p);++r.materials;
            r.valid &= RpNormMapMaterialGetNormMapTexture(m)==r.normal && RpNormMapMaterialGetEnvMapTexture(m)==r.env;
            if(r.frame)r.valid &= RpNormMapMaterialGetEnvMapFrame(m)==r.frame && RpNormMapMaterialGetEnvMapCoefficient(m)==0.5f && RpNormMapMaterialIsEnvMapModulated(m);
            return m;
        },p);return a;
    };
    Inspect basic{generated.p,nullptr,nullptr,pipeline};RpClumpForAllAtomics(asset.Clump(),inspect,&basic);
    check(basic.valid && basic.atomics && basic.materials,"normal-map real basic material refs and SDK static pipeline");
    // Repeat with an explicit texture: this must reuse the supplied map.
    check(NativeClumpNormalMap::Apply(address(asset.Clump()),config) && config[0]==address(generated.p),"normal-map supplied texture reused");
    struct Frame {RwFrame* p{RwFrameCreate()};~Frame(){if(p)RwFrameDestroy(p);}} frame;
    check(frame.p,"normal-map environment frame");
    std::array<std::uint32_t,8> extended{address(frame.p),address(generated.p),std::bit_cast<std::uint32_t>(0.5f),1u,1u,std::bit_cast<std::uint32_t>(2.0f),0u,0u};
    check(NativeClumpNormalMap::ApplyExtended(address(asset.Clump()),extended) && extended[6],"normal-map extended missing texture generation");
    Texture extended_texture{reinterpret_cast<RwTexture*>(extended[6])};
    Inspect extra{extended_texture.p,generated.p,frame.p,pipeline};RpClumpForAllAtomics(asset.Clump(),inspect,&extra);
    check(extra.valid && extra.materials==basic.materials,"normal-map extended real normal/env/frame/coefficient/modulation");
    // Release the clump's material refs while the borrowed environment frame
    // and the two caller-owned temporary texture references are still alive.
    asset.Unload();
    check(generated.p->refCount==1 && extended_texture.p->refCount==1,"normal-map materials release texture refs on clump unload");
    std::cout<<"Native normal-map material: original WTA diffuse generation, basic/extended SDK pipelines, explicit reuse, env frame/coefficient/modulation, mip raster and reference retirement verified\n";
}
}
