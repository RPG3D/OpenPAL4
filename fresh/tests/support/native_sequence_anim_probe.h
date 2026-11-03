#pragma once
#include "pal4/native_effect_object_queues.h"
#include "pal4/native_particle_group_frames.h"
#include "pal4/native_particle_effect_factory.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_event_document_runtime.h"
#include "pal4/native_package_archive.h"
#include "pal4/sequence_anim_object.h"
#include "pal4/geo_plane.h"
#include "native_particle_probe_camera.h"
#include <bit>
#include <functional>
#include <iostream>
#include <source_location>

namespace pal4::test {
inline void VerifyNativeSequenceAnimFactory(NativeBaseObjectMemory& memory,
    NativeEventDocumentRuntime& documents,NativeParticleEffectFactory& objects,
    NativeEffectObjectQueues& queues,NativeParticleGroupFrames& frames,
    NativePackageArchive& archive,std::uint32_t root,
    std::array<std::uint32_t,42>& system,std::array<std::uint32_t,53>& context) {
    const auto check=[](bool ok,const std::source_location at=std::source_location::current()) {
        if(!ok)throw std::runtime_error("native sequence atlas line="+std::to_string(at.line()));
    };
    const auto address=[](const void* p){return reinterpret_cast<std::uint32_t>(p);};
    documents.CleanupDocument(root);
    const auto data=archive.ReadMember("script\\H_083.eft");
    check(data && documents.LoadBinaryDocument(root,*data));
    std::uint32_t config{};
    std::function<void(std::uint32_t)> visit=[&](std::uint32_t node) {
        const auto a=documents.FindNumericAttribute(node,"Attr_Class_Name");
        if(a && documents.AttributeToString(a)=="SequnceAnimObject") {check(!config);config=node;}
        for(std::uint32_t i=0;i<documents.ChildCount(node);++i)visit(documents.ChildAt(node,i));
    };
    visit(root);check(config && !objects.LiveObjects() && !system[41]);
    const auto baseline=memory.OutstandingAllocations();
    for(unsigned pass=0;pass<2u;++pass) {
        const auto object=objects.Create(config,0,address(context.data()));
        check(object && system[41]==1u && objects.ObjectTypeName(object)=="SequnceAnimObject");
        auto& state=*reinterpret_cast<SequenceAnimObjectState*>(object);
        auto& plane=*reinterpret_cast<GeoPlaneState*>(memory.ReadDword(state.words[30]+48u));
        auto* geometry=RpAtomicGetGeometry(reinterpret_cast<RpAtomic*>(plane.atomic));
        check(state.words[14]*state.words[15]>1u && geometry->numVertices==4 &&
            RpMaterialGetTexture(RpGeometryGetMaterial(geometry,0)));
        const auto show=objects.ResolveFacet(object,"ShowobjTask"),position=objects.ResolveFacet(object,"PositionTask"),
            tint=objects.ResolveFacet(object,"ChangeColorTask"),scale=objects.ResolveFacet(object,"ScaleTask"),
            rotation=objects.ResolveFacet(object,"RotateTask");
        check(show==object+20 && position==object+24 && tint==object+36 && scale==object+40 &&
            rotation==object+44 && objects.ResolveFacet(object,"MessageTask")==object+48 &&
            !objects.ResolveFacet(object,"RumpShahdowTask"));
        queues.QueueEffect(address(context.data()),object);objects.Show(show);
        const auto queue=objects.UseFallbackQueue(object)?200u:128u;
        check(queues.Count(queue)==1u && queues.Contains(queue,object));
        objects.Move(position,{3,4,0},false);objects.Move(position,{-3,-4,0},true);
        check(objects.ReadPosition(position)==std::array<float,3>{0,0,0});
        objects.SetScale(scale,{2,3,0},0);objects.SetScale(scale,{1,1,0},1);
        std::array<float,3> values{};objects.ReadScale(scale,values);check(values==std::array<float,3>{3,4,0});
        objects.ApplyRotation(rotation,{15,90,90},false);objects.ApplyRotation(rotation,{5,0,0},true);
        objects.ReadRotation(rotation,values);check(values==std::array<float,3>{20,0,0});
        objects.SetTrackPosition(objects.ResolveFacet(object,"TrackTask"),{1,2,3});
        check(objects.ReadPosition(position)==std::array<float,3>{1,2,3});
        objects.SetColor(tint,{11,22,33,200});objects.SetMode(object,1);
        std::array<std::uint8_t,4> color{};objects.ReadColor(tint,color);check(color==std::array<std::uint8_t,4>{11,22,33,200});
        check(objects.ResetPlayback(object));objects.ReadColor(tint,color);
        check(std::bit_cast<std::uint32_t>(color)==(state.words[35]&0xfeffffffu));
        objects.Move(position,{0,0,0},false);objects.SetScale(scale,{1,1,0},0);objects.ApplyRotation(rotation,{0,0,0},false);
        const auto uv_before=RpGeometryGetVertexTexCoords(geometry,0)[0];
        NativeParticleProbeCamera camera;camera.Begin();const auto old_camera=context[12];context[12]=address(camera.camera);
        frames.Update(address(context.data()),std::bit_cast<float>(state.words[13])*2.0);
        const auto uv_after=RpGeometryGetVertexTexCoords(geometry,0)[0];
        check((uv_before.u!=uv_after.u || uv_before.v!=uv_after.v) && state.words[16]==1u && !state.words[20]);
        frames.Draw(address(context.data()),address(camera.camera));const auto pixels=camera.EndPixels();check(pixels>0);
        objects.SetColor(tint,{11,22,33,200});objects.SetMode(object,1);
        frames.Update(address(context.data()),0.4);objects.ReadColor(tint,color);
        check(color[0]==11 && color[1]==22 && color[2]==33 && color[3]>=99 && color[3]<=100);
        objects.Hide(show);const auto stopped_column=state.words[16];
        frames.Update(address(context.data()),0.1);check(state.words[16]==stopped_column);
        camera.Begin();frames.Draw(address(context.data()),address(camera.camera));check(!camera.EndPixels());
        auto* parent=RwFrameCreate();check(parent);
        check(RwFrameAddChild(parent,reinterpret_cast<RwFrame*>(objects.Frame(object))));
        objects.ReleasePlayback(object,queues);
        check(!parent->child && !state.words[30] && !system[41] && !queues.Count(queue));
        objects.Destroy(object);(void)RwFrameDestroy(parent);context[12]=old_camera;
        check(!objects.LiveObjects() && memory.OutstandingAllocations()==baseline);
        std::cout<<"Original H_083 sequence: atlas UV advances in SDK geometry, GPU pixels="<<pixels
            <<", native properties/reset/fade/hide, both release passes and exact resource/counter/queue/parent balance verified\n";
    }
}
}
