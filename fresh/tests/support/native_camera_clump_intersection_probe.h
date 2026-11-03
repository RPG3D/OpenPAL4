#pragma once
#include "pal4/native_camera_clump_intersection.h"
#include <rwcore.h>
#include <rpworld.h>
#include <rtpick.h>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace pal4::test {
inline void VerifyNativeCameraClumpIntersection(CameraDataState& data){
    const auto check=[](bool ok,const char* m){if(!ok)throw std::runtime_error(m);};
    const auto saved=data;
    struct Restore{CameraDataState& data;CameraDataState saved;~Restore(){data=saved;}} restore{data,saved};
    std::uint32_t index=0xfeedu;
    auto* camera=reinterpret_cast<RwCamera*>(data.At(0u));
    check(camera!=nullptr,"intersection probe actual CombatCamera");
    check(RwCameraBeginUpdate(camera)!=nullptr,"intersection probe updates actual SDK frustum");
    check(RwCameraEndUpdate(camera)!=nullptr,"intersection probe ends camera update");
    auto* raster=RwCameraGetRaster(camera);
    const std::array<float,2> pixel{RwRasterGetWidth(raster)*0.37f,RwRasterGetHeight(raster)*0.41f};
    const auto before=data;
    check(!IntersectNativeCameraClump(data,nullptr,pixel,index) && data.words==before.words && index==0xfeedu,
          "null clump preserves original query and sticky atomic index");
    CameraDataState empty;empty.At(12u)=0x12345678u;
    check(!IntersectNativeCameraClump(empty,nullptr,pixel,index) && empty.At(12u)==0x12345678u && index==0xfeedu,
          "absent camera preserves query");
    RwLine ray{};const RwV2d point{pixel[0],pixel[1]};
    check(RwCameraCalcPixelRay(camera,&ray,&point)!=nullptr,"probe SDK reference ray");
    auto* clump=RpClumpCreate();auto* root=RwFrameCreate();
    check(clump && root,"intersection probe clump resources");RpClumpSetFrame(clump,root);
    struct Cleanup{RpClump* clump;~Cleanup(){RpClumpDestroy(clump);}} cleanup{clump};
    std::array<RpAtomic*,3> atomics{};
    for(auto& atomic:atomics){
        atomic=RpAtomicCreate();auto* frame=RwFrameCreate();auto* geometry=RpGeometryCreate(3,1,rpGEOMETRYPOSITIONS);
        auto* material=RpMaterialCreate();check(atomic && frame && geometry && material,"intersection probe triangle resources");
        RwFrameAddChild(root,frame);RpAtomicSetFrame(atomic,frame);RpClumpAddAtomic(clump,atomic);
        auto* triangle=RpGeometryGetTriangles(geometry);
        check(RpGeometryTriangleSetVertexIndices(geometry,triangle,0,2,1) &&
              RpGeometryTriangleSetMaterial(geometry,triangle,material),"intersection probe triangle topology");
        (void)RpMaterialDestroy(material);RpAtomicSetGeometry(atomic,geometry,0u);RpGeometryDestroy(geometry);
    }
    std::vector<RpAtomic*> order;
    RpClumpForAllAtomics(clump,[](RpAtomic* atomic,void* p){static_cast<std::vector<RpAtomic*>*>(p)->push_back(atomic);return atomic;},&order);
    check(order.size()==3u,"intersection actual SDK traversal order");
    const auto* matrix=RwFrameGetLTM(RwCameraGetFrame(camera));
    const auto right=*RwMatrixGetRight(matrix),up=*RwMatrixGetUp(matrix);
    std::cout<<"Native intersection ray: start="<<ray.start.x<<","<<ray.start.y<<","<<ray.start.z<<" end="<<ray.end.x<<","<<ray.end.y<<","<<ray.end.z<<" right="<<right.x<<","<<right.y<<","<<right.z<<" up="<<up.x<<","<<up.y<<","<<up.z<<"\n";
    const auto fill=[&](RpAtomic* atomic,float fraction,float lateral){
        auto* geometry=RpAtomicGetGeometry(atomic);RpGeometryLock(geometry,rpGEOMETRYLOCKVERTICES);
        auto* morph=RpGeometryGetMorphTarget(geometry,0);auto* vertices=RpMorphTargetGetVertices(morph);
        const RwV3d center{ray.start.x+(ray.end.x-ray.start.x)*fraction+right.x*lateral,
            ray.start.y+(ray.end.y-ray.start.y)*fraction+right.y*lateral,
            ray.start.z+(ray.end.z-ray.start.z)*fraction+right.z*lateral};
        constexpr std::array<std::array<float,2>,3> offsets{{{-3,-3},{3,-3},{0,3}}};
        for(unsigned i=0;i<3u;++i)vertices[i]={center.x+right.x*offsets[i][0]+up.x*offsets[i][1],
            center.y+right.y*offsets[i][0]+up.y*offsets[i][1],center.z+right.z*offsets[i][0]+up.z*offsets[i][1]};
        RwSphere sphere{};RpMorphTargetCalcBoundingSphere(morph,&sphere);RpMorphTargetSetBoundingSphere(morph,&sphere);
        check(RpGeometryUnlock(geometry)!=nullptr,"intersection probe unlock geometry");
        // SetGeometry(same pointer) is an SDK no-op. Retain across detach so
        // reattachment copies the newly calculated morph sphere into the atomic.
        (void)RpGeometryAddRef(geometry);(void)RpAtomicSetGeometry(atomic,nullptr,0u);
        check(RpAtomicSetGeometry(atomic,geometry,0u)!=nullptr,"intersection probe refresh atomic sphere");
        (void)RpGeometryDestroy(geometry);
    };
    fill(order[0],0.65f,0);fill(order[1],0.35f,0);fill(order[2],0.35f,0);
    auto* hit=IntersectNativeCameraClump(data,clump,pixel,index);
    const auto distance=std::bit_cast<float>(data.At(44u));
    std::cout<<"Native intersection operands: hit="<<hit<<" expected="<<order[1]<<" index="<<index<<" fraction="<<distance<<" visited="<<data.At(48u)<<"\n";
    check(hit==order[1] && index==1u && std::fabs(distance-0.35f)<0.0001f && data.At(48u)==3u,
          "SDK nearest atomic and strict equal-distance traversal policy");
    check(data.At(36u)==1u && data.At(40u)==reinterpret_cast<std::uint32_t>(hit),"original query type and atomic fields");
    check(!std::memcmp(reinterpret_cast<const char*>(&data)+12u,&ray,sizeof(ray)),"query stores SDK floating pixel ray without integer conversion");
    check(data.At(8u)==before.At(8u) && data.At(52u)==before.At(52u),"intersection adjacent camera fields preserved");
    for(auto* atomic:order)fill(atomic,0.35f,10000.f);
    check(!IntersectNativeCameraClump(data,clump,pixel,index) && index==1u && data.At(44u)==0x7f7fffffu && data.At(48u)==3u,
          "miss retains last hit index and resets nearest result while traversing all atomics");
    std::cout<<"Native camera clump intersection: SDK float pixel ray, nearest/tie, miss sticky index, null gates, camera query bounds pass\n";
}
}
