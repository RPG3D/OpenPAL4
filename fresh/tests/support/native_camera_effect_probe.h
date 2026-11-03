#pragma once
#include "pal4/native_camera_effect_object.h"
#include "pal4/native_effect_dialog_targets.h"
#include "pal4/native_dialog_object.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_event_document_runtime.h"
#include "pal4/native_dialog_particle_effects.h"
#include <rwcore.h>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <iostream>
#include <source_location>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeCameraEffect(bool named_targets=false) {
    const auto check=[](bool ok,const std::source_location at=std::source_location::current()) {
        if(!ok) throw std::runtime_error("native camera effect probe line="+std::to_string(at.line()));
    };
    const auto address=[](const void* p){return reinterpret_cast<std::uint32_t>(p);};
    NativeBaseObjectMemory memory;
    NativeEventDocumentRuntime documents(memory);
    std::array<std::uint32_t,42> system{};
    std::array<std::uint32_t,65> group{};
    std::array<std::uint32_t,15> xml{};
    std::array<std::uint32_t,23> dialog{};
    auto* camera=RwCameraCreate();check(camera!=nullptr);
    auto* frame=RwFrameCreate();check(frame!=nullptr);
    check(RwCameraSetFrame(camera,frame)!=nullptr);
    group[12]=address(camera);
    const RwV3d initial{71,82,93};check(RwFrameTranslate(frame,&initial,rwCOMBINEREPLACE)!=nullptr);
    RwMatrix original=*RwFrameGetMatrix(frame);
    struct Queues final : INativeDialogParticleQueues {
        std::uint32_t expected_parent{},expected_object{};
        bool queued{};
        unsigned adds{},removes{};
        void QueueEffect(std::uint32_t p,std::uint32_t o) override {
            if(p!=expected_parent || o!=expected_object) throw std::invalid_argument("camera probe queue owner");
            queued=true;++adds;
        }
        void RemoveEffect(std::uint32_t p,std::uint32_t o) override {
            if(p && (p!=expected_parent || o!=expected_object)) throw std::invalid_argument("camera probe removal owner");
            queued=false;++removes;
        }
    } queues;
    queues.expected_parent=address(group.data());
    documents.InitializeDocument(address(xml.data()),"CameraObject",0u);
    try {
        documents.WriteIntAttribute(address(xml.data()),"Attr_Camera_Lock_LookAt",1);
        documents.WriteFloatAttribute(address(xml.data()),"Attr_Camera_Position_X",10);
        documents.WriteFloatAttribute(address(xml.data()),"Attr_Camera_Position_Y",20);
        documents.WriteFloatAttribute(address(xml.data()),"Attr_Camera_Position_Z",30);
        const auto allocations=memory.OutstandingAllocations();const auto count=documents.DocumentCount();
        {
            NativeCameraEffectObject effect(memory,documents,address(system.data()),queues);
            auto& s=effect.State();queues.expected_object=address(&s);
            s.words[3]=address(dialog.data());s.words[4]=queues.expected_parent;
            check(system[41]==1 && effect.Load(address(xml.data())) && effect.Frame()!=0);
            effect.Start();check(!effect.Ready() && !queues.queued); // Disabled Dialog gate.
            dialog[22]=1;
            effect.Start();check(effect.Ready() && queues.queued && s.words[32]!=0);
            effect.Advance();const auto& pos=RwFrameGetLTM(frame)->pos;
            check(std::abs(pos.x-10)<0.001f && std::abs(pos.y-20)<0.001f && std::abs(pos.z-30)<0.001f);
            std::array<float,3> v{};effect.ReadPosition(v);check(v==std::array<float,3>{10,20,30});
            effect.Move({2,3,4},true);effect.Advance();effect.ReadPosition(v);check(v==std::array<float,3>{12,23,34});
            effect.Rotate({0,90,0},false);effect.ReadRotation(v);check(v==std::array<float,3>{0,90,0});
            effect.SetLockedLookAt(false);check(!effect.LockedLookAt());
            effect.Rotate({0,0,0},false);effect.Move({10,0,0},false);
            effect.Revolve({0,0,0},{0,0,90},{0,0,0});effect.ReadPosition(v);
            check(std::abs(v[0])<0.001f && std::abs(v[1]-10.f)<0.001f && std::abs(v[2])<0.001f);
            effect.Revolve({0,0,0},{0,0,0},{0,2,0});effect.ReadPosition(v);
            check(std::abs(v[1]-8.f)<0.001f);
            effect.Pause();check(!effect.Ready() && !s.words[32]);
            const auto& restored=*RwFrameGetMatrix(frame);
            for(unsigned i:{0u,1u,2u,4u,5u,6u,8u,9u,10u,12u,13u,14u})
                check(std::abs(reinterpret_cast<const float*>(&restored)[i]-reinterpret_cast<const float*>(&original)[i])<0.001f);
            check(effect.Reset());effect.Start();check(effect.Ready());effect.SetMode(1);check(!effect.Ready());
            effect.Start();effect.Advance();effect.ReleasePlayback();effect.ReleasePlayback();
            check(effect.Frame()==0 && !queues.queued && !effect.Ready());
        }
        check(system[41]==0 && memory.OutstandingAllocations()==allocations && documents.DocumentCount()==count);
        // Inherit mode applies each wobble delta directly to the camera frame.
        documents.WriteIntAttribute(address(xml.data()),"Attr_Camera_Inherit_Current",1);
        {
            NativeCameraEffectObject effect(memory,documents,address(system.data()),queues);
            queues.expected_object=address(&effect.State());effect.State().words[4]=queues.expected_parent;
            check(effect.Load(address(xml.data())));effect.Start();check(effect.InheritCurrent());
            effect.Move({1,2,3},true);effect.Advance();const auto p=RwFrameGetLTM(frame)->pos;
            check(std::abs(p.x-72)<0.001f && std::abs(p.y-84)<0.001f && std::abs(p.z-96)<0.001f);
            const auto before_revolve=effect.State().words;
            effect.Revolve({0,0,0},{0,0,90},{1,2,3});
            check(effect.State().words==before_revolve); // Original inherit44 gate.
            effect.Pause();check(std::abs(RwFrameGetLTM(frame)->pos.x-71)<0.001f);
        }
        if(named_targets) {
        // Named targets use actual Dialog properties: live facet lookup and
        // fixed configured target are deliberately different calls in607FF0.
        documents.WriteIntAttribute(address(xml.data()),"Attr_Camera_Inherit_Current",0);
        documents.WriteIntAttribute(address(xml.data()),"Attr_Camera_Target_Mode",1);
        documents.WriteStringAttribute(address(xml.data()),"Attr_Camera_Target_Name","T1");
        for(const auto& [key,value]:std::array<std::pair<const char*,float>,3>{{
            {"Attr_Camera_LookAt_x",2},{"Attr_Camera_LookAt_y",3},{"Attr_Camera_LookAt_z",4}}})
            documents.WriteFloatAttribute(address(xml.data()),key,value);
        {
            NativeDialogObject owner(memory,documents);
            std::array<std::uint32_t,15> target_xml{};const auto root=address(target_xml.data());
            documents.InitializeDocument(root,"Effect",0);
            (void)documents.CreateChild(root,"OxnList");const auto list=documents.CreateChild(root,"TargetList");
            const auto target=documents.CreateChild(list,"Target");
            documents.WriteStringAttribute(target,"TargetName","T1");
            documents.WriteFloatAttribute(target,"TargetPosX",3);documents.WriteFloatAttribute(target,"TargetPosY",4);documents.WriteFloatAttribute(target,"TargetPosZ",5);
            check(owner.Load(root));documents.DestroyDocument(root);owner.State().words[22]=1;
            NativeEffectDialogTargets targets([&](std::uint32_t d)->NativeDialogProperties& {
                check(d==owner.Address());return owner.Properties();
            });
            const std::array<float,3> stored{3,4,5};
            check(targets.ReadStored(owner.Address(),"T1")==stored && targets.ReadLive(owner.Address(),"T1")==stored);
            check(targets.ReadLive(owner.Address(),"missing")==std::array<float,3>{});
            std::array<float,3> live{10,20,30};owner.Properties().SetFacet("T1",address(live.data()));
            bool unbound=false;try{(void)targets.ReadLive(owner.Address(),"T1");}
            catch(const std::logic_error& e){unbound=std::string_view(e.what())=="native Dialog facet vector reader is not bound";}
            check(unbound);
            unsigned reads{};
            targets.BindFacetReader([&](std::uint32_t facet){
                check(facet==address(live.data()));++reads;
                bool busy=false;try{targets.BindFacetReader({});}catch(const std::logic_error&){busy=true;}check(busy);
                return live;
            });
            auto* parent=RwFrameCreate();check(parent);
            struct Parent {RwFrame* frame;~Parent(){(void)RwFrameDestroy(frame);}} parent_owner{parent};
            const RwV3d parent_position{7,8,9};(void)RwFrameTranslate(parent,&parent_position,rwCOMBINEREPLACE);
            {
                NativeCameraEffectObject effect(memory,documents,address(system.data()),queues,&targets);
                queues.expected_object=address(&effect.State());effect.State().words[3]=owner.Address();effect.State().words[4]=queues.expected_parent;
                check(effect.Load(address(xml.data())));(void)RwFrameAddChild(parent,reinterpret_cast<RwFrame*>(effect.Frame()));
                effect.Start();std::array<float,3> out{};effect.ReadTarget(out);
                check(std::abs(out[0]-2)<0.001f && std::abs(out[1]-11)<0.001f && std::abs(out[2]-20)<0.001f);
                check(targets.ReadStored(owner.Address(),"T1")==stored);
                live={20,30,40};effect.ReadTarget(out);
                check(std::abs(out[0]-12)<0.001f && std::abs(out[1]-21)<0.001f && std::abs(out[2]-30)<0.001f);
                effect.Advance();const auto* matrix=RwFrameGetMatrix(reinterpret_cast<RwFrame*>(effect.Frame()));
                std::array<float,3> position{};effect.ReadPosition(position);
                RwV3d expected{out[0]-position[0],out[1]-position[1],out[2]-position[2]};(void)RwV3dNormalize(&expected,&expected);
                check(std::abs(matrix->at.x-expected.x)<0.001f && std::abs(matrix->at.y-expected.y)<0.001f && std::abs(matrix->at.z-expected.z)<0.001f);
                check(reads>=3);effect.Pause();
            }
            targets.BindFacetReader({});owner.ClearFacets();
            check(targets.ReadLive(owner.Address(),"T1")==stored);
        }
        std::cout<<"Camera named targets: actual Dialog live/stored/missing lookup, changing facet, parent transform, SDK facing, service guard and balanced retirement verified\n";
        }
        check(system[41]==0 && queues.adds>=3 && queues.removes>=queues.adds);
        documents.DestroyDocument(address(xml.data()));
    } catch(...) {
        documents.DestroyDocument(address(xml.data()));RwCameraSetFrame(camera,nullptr);RwCameraDestroy(camera);RwFrameDestroy(frame);throw;
    }
    check(documents.DocumentCount()==0 && memory.OutstandingAllocations()==0);
    RwCameraSetFrame(camera,nullptr);check(RwCameraDestroy(camera)!=0);check(RwFrameDestroy(frame)!=0);
    std::cout<<"Camera effect: real SDK camera takeover/restore, Dialog readiness, XML reset, position/rotation, inherit delta, mode pause, repeat release and allocation balance verified\n";
}
}
