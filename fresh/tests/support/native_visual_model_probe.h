#pragma once
#include "pal4/native_visual_model_runtime.h"
#include "native_particle_probe_camera.h"
#include <rpworld.h>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace pal4::test {
inline void VerifyNativeVisualModel() {
    struct Owner {
        NativeVisualModelRuntime runtime;
        VisualModelState model{};
        ~Owner() {VisualModel::Destroy(model,runtime);}
    } owner;
    const auto check=[](bool value) {
        if(!value) throw std::runtime_error("native VisualModel cylinder contract");
    };
    const auto closeEnough=[](float a,float b){return std::abs(a-b)<0.0002f;};
    CylinderCreateOptions options{};
    options.border_count=8;options.height=30;options.first_radius=15;
    options.second_radius=12;options.arc_degrees=360;options.centered=true;
    owner.runtime.CreateCylinder(owner.model,options);
    auto* clump=reinterpret_cast<RpClump*>(owner.model.clump);
    auto* frame=reinterpret_cast<RwFrame*>(owner.model.frame);
    struct GeometryCheck {unsigned atomics{},vertices{},triangles{};bool valid=true;RpGeometry* first{};} geometry;
    (void)RpClumpForAllAtomics(clump,[](RpAtomic* atomic,void* data)->RpAtomic* {
        auto& result=*static_cast<GeometryCheck*>(data);
        auto* mesh=RpAtomicGetGeometry(atomic);
        if(!result.first) result.first=mesh;
        ++result.atomics;result.vertices+=RpGeometryGetNumVertices(mesh);
        result.triangles+=RpGeometryGetNumTriangles(mesh);
        const auto color=*RpMaterialGetColor(RpGeometryGetMaterial(mesh,0));
        const auto* vertices=RpMorphTargetGetVertices(RpGeometryGetMorphTarget(mesh,0));
        const auto* uv=RpGeometryGetVertexTexCoords(mesh,rwTEXTURECOORDINATEINDEX0);
        const auto* triangle=RpGeometryGetTriangles(mesh);
        RwUInt16 a{},b{},c{};
        (void)RpGeometryTriangleGetVertexIndices(mesh,triangle,&a,&b,&c);
        result.valid=result.valid && RpGeometryGetNumVertices(mesh)==4 &&
            RpGeometryGetNumTriangles(mesh)==2 && a==0 && b==1 && c==2 &&
            color.alpha==0xfe && color.red==255 &&
            std::abs(vertices[0].y-15.0f)<0.0002f &&
            std::abs(vertices[1].y+15.0f)<0.0002f && uv[0].v==0.0f && uv[1].v==1.0f &&
            RpGeometryGetPreLightColors(mesh)[0].alpha==255 &&
            RpMorphTargetGetBoundingSphere(RpGeometryGetMorphTarget(mesh,0))->radius>0;
        return atomic;
    },&geometry);
    unsigned children{};
    (void)RwFrameForAllChildren(frame,[](RwFrame* f,void* data)->RwFrame* {
        ++*static_cast<unsigned*>(data);return f;
    },&children);
    check(owner.model.active && geometry.valid && geometry.atomics==8 &&
          geometry.vertices==32 && geometry.triangles==16 && children==8);
    (void)VisualModel::SetScale(owner.model,{2,3,4},false,owner.runtime);
    (void)VisualModel::SetRotation(owner.model,{0,90,0},false,owner.runtime);
    (void)VisualModel::SetPosition(owner.model,{10,20,30},false,owner.runtime);
    const auto* matrix=RwFrameGetLTM(frame);
    const auto* right=RwMatrixGetRight(matrix);
    const auto* up=RwMatrixGetUp(matrix);
    const auto* at=RwMatrixGetAt(matrix);
    const auto* position=RwMatrixGetPos(matrix);
    check(closeEnough(position->x,10) && closeEnough(position->y,20) && closeEnough(position->z,30) &&
          closeEnough(right->x,0) && closeEnough(std::abs(right->z),2) && closeEnough(up->y,3) &&
          closeEnough(std::abs(at->x),4));
    (void)VisualModel::SetScale(owner.model,{1,1,1},false,owner.runtime);
    (void)VisualModel::SetRotation(owner.model,{},false,owner.runtime);
    (void)VisualModel::SetPosition(owner.model,{},false,owner.runtime);
    (void)VisualModel::SetMaterialFlags(owner.model,0xffa0c0e0u,owner.runtime);
    check(owner.model.material_flags==0xfea0c0e0u);
    {
        NativeParticleProbeCamera camera;camera.Begin();
        constexpr std::array<std::uint32_t,5> states{27,19,20,7,14};
        std::array<std::uint32_t,5> previous{};
        for(std::size_t i=0;i<states.size();++i) previous[i]=owner.runtime.GetRenderState(states[i]);
        const auto device=owner.runtime.GetDeviceState(20);
        (void)VisualModel::Render(owner.model,0,0,owner.runtime);
        for(std::size_t i=0;i<states.size();++i) check(owner.runtime.GetRenderState(states[i])==previous[i]);
        check(owner.runtime.GetDeviceState(20)==device);
        const auto pixels=camera.EndPixels();check(pixels>0);
        std::cout << "Native GeoCylinder/VisualModel: 8 atomics,32 vertices,16 triangles; "
                  << pixels << " GPU pixels; transforms and render-state restore verified\n";
    }
    VisualModel::Destroy(owner.model,owner.runtime);
    check(!owner.model.active && !owner.model.clump && !owner.model.frame);
    // Repeated construction must use fresh SDK resources, including all owned
    // atomic child frames. Border mode follows the shared original algorithm.
    options.bordered=true;options.border_count=3;options.border_width=2;
    for(unsigned i=0;i<3;++i) {
        owner.runtime.CreateCylinder(owner.model,options);
        check(owner.model.active && RpClumpGetNumAtomics(reinterpret_cast<RpClump*>(owner.model.clump))==12);
        VisualModel::Destroy(owner.model,owner.runtime);
    }
    options.border_count=2;
    bool rejected{};
    try {owner.runtime.CreateCylinder(owner.model,options);}
    catch(const std::runtime_error&) {rejected=true;}
    check(rejected && !owner.model.active && !owner.model.frame && !owner.model.clump);
}
} // namespace pal4::test

