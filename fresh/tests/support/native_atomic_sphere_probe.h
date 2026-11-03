#pragma once
#include "pal4/native_player_transform_runtime.h"
#include "pal4/native_actor_clump_asset.h"
#include "pal4/native_animated_actor_asset.h"
#include <rwcore.h>
#include <rpworld.h>
#include <array>
#include <algorithm>
#include <bit>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace pal4::test {
inline void VerifyNativeActorLoadPoseBounds(const std::filesystem::path& root) {
    NativeActorClumpAsset raw;
    NativeAnimatedActorAsset animated;
    if(!raw.Load(root,"101") || !animated.Load(root,"101"))
        throw std::runtime_error("load real101 raw/animated clumps for bounds-order probe");
    NativePlayerTransformRuntime runtime;
    GameResourceManager raw_bounds,animated_bounds;
    raw_bounds.words[5]=reinterpret_cast<std::uint32_t>(raw.Main());
    animated_bounds.words[5]=reinterpret_cast<std::uint32_t>(animated.Main());
    (void)raw_bounds.CalculateBoundingBox(runtime);
    (void)animated_bounds.CalculateBoundingBox(runtime);
    std::cout<<"101 bounds order probe raw_radius_bits="<<raw_bounds.words[55]
             <<" animated_radius_bits="<<animated_bounds.words[55]<<'\n';
    // Recorded original live75 radius, same101 source asset and original load
    // order. The animated pose must not silently replace this cached geometry.
    if(raw_bounds.words[55]!=1109196445u || animated_bounds.words[55]==raw_bounds.words[55])
        throw std::runtime_error("raw101 bounds hypothesis does not reproduce original cached radius");
    if(!std::equal(animated.InitialBounds().begin(),animated.InitialBounds().end(),
                   raw_bounds.words.begin()+52u))
        throw std::runtime_error("animated asset must retain all initial DFF-pose bounds words");
    const auto initial=animated.InitialBounds();
    if(!animated.SelectAction(animated.ActionAt(0)) || !animated.Advance(0.1f) ||
       animated.InitialBounds()!=initial)
        throw std::runtime_error("animation must not mutate cached resource load bounds");
    animated.Unload();
    if(animated.InitialBounds()!=std::array<std::uint32_t,20>{})
        throw std::runtime_error("asset unload must retire its bounds cache");
}
inline void VerifyNativeAtomicSpheres() {
    auto require=[](bool value,const char* message){if(!value)throw std::runtime_error(message);};
    auto* clump=RpClumpCreate();
    require(clump!=nullptr,"sphere probe clump");
    struct Cleanup {RpClump* value;~Cleanup(){RpClumpDestroy(value);}} cleanup{clump};
    auto* root=RwFrameCreate();RpClumpSetFrame(clump,root);
    std::array<RpAtomic*,2> atomics{};
    for(auto& atomic:atomics){
        atomic=RpAtomicCreate();require(atomic!=nullptr,"sphere probe atomic");
        RpAtomicSetFrame(atomic,RwFrameCreate());
        RwFrameAddChild(root,RpAtomicGetFrame(atomic));
        RpClumpAddAtomic(clump,atomic);
        atomic->interpolator.flags&=~rpINTERPOLATORDIRTYSPHERE;
    }
    NativePlayerTransformRuntime runtime;
    RwV3d delta{};float expected_radius{};bool distinguishes=false;
    for(unsigned i=20;i<1000 && !distinguishes;++i){
        delta={0.07f*static_cast<float>(i),0.31f,0.53f};
        const float distance=(RwV3dLength)(&delta);
        expected_radius=static_cast<float>((static_cast<double>(distance)+2.0+1.0)*0.5);
        distinguishes=std::bit_cast<unsigned>(expected_radius)!=std::bit_cast<unsigned>(
            (std::sqrt(delta.x*delta.x+delta.y*delta.y+delta.z*delta.z)+3.0f)*0.5f);
    }
    require(distinguishes,
            "sphere fixture distinguishes the actual SDK table from host sqrt");
    atomics[0]->boundingSphere={{0,0,0},2};
    atomics[1]->boundingSphere={delta,1};
    std::array<std::uint32_t,4> sphere{};
    runtime.ProcessAtomicBounds2(reinterpret_cast<std::uint32_t>(clump),0,1,sphere.data());
    const auto radius=std::bit_cast<float>(sphere[3]);
    std::cout<<"Atomic sphere table probe actual="<<radius<<" expected="<<expected_radius<<'\n';
    require(sphere[3]==std::bit_cast<unsigned>(expected_radius),"native sphere merge must use original SDK RwV3dLength");
    sphere=std::bit_cast<std::array<std::uint32_t,4>>(std::array<float,4>{0,0,0,1000});
    const auto before=sphere;
    runtime.ProcessAtomicBounds2(reinterpret_cast<std::uint32_t>(clump),1,0,sphere.data());
    require(sphere==before,"existing containing sphere is retained without initialization");
    atomics[1]->boundingSphere=atomics[0]->boundingSphere;
    runtime.ProcessAtomicBounds2(reinterpret_cast<std::uint32_t>(clump),0,1,sphere.data());
    require(std::isnan(std::bit_cast<float>(sphere[0])) && std::bit_cast<float>(sphere[3])==2.0f,
            "coincident equal spheres preserve original unchecked zero-distance behavior");
    std::cout<<"PASS: native SDK sphere merge, retained containing sphere and original degenerate boundary\n";
}
}
