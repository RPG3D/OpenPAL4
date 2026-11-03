#pragma once
#include "pal4/thunder_object.h"
#include "pal4/native_thunder_effect_object.h"
#include "pal4/native_effect_dialog_targets.h"
#include "pal4/native_dialog_properties.h"
#include "native_particle_probe_camera.h"
#include <source_location>

namespace pal4::test {
inline void VerifyNativeThunderFactory(NativeBaseObjectMemory& memory,
    NativeEventDocumentRuntime& documents,NativeParticleEffectFactory& objects,
    NativeEffectObjectQueues& queues,NativeParticleGroupFrames& frames,
    NativePackageArchive& archive,NativeEffectTextureDictionary& textures,std::uint32_t root,
    std::array<std::uint32_t,42>& system,std::array<std::uint32_t,53>& context) {
    const auto check=[](bool ok,const std::source_location at=std::source_location::current()) {
        if(!ok) throw std::runtime_error("native Thunder original-resource line="+std::to_string(at.line()));
    };
    const auto address=[](const void* p){return reinterpret_cast<std::uint32_t>(p);};
    documents.CleanupDocument(root);
    const auto bytes=archive.ReadMember("script\\HW_XS013.eft");
    check(bytes && documents.LoadBinaryDocument(root,*bytes));
    std::uint32_t config{};
    std::function<void(std::uint32_t)> visit=[&](std::uint32_t node) {
        const auto a=documents.FindNumericAttribute(node,"Attr_Class_Name");
        if(a && documents.AttributeToString(a)=="ThunderObject") {check(!config);config=node;}
        for(std::uint32_t i=0;i<documents.ChildCount(node);++i)visit(documents.ChildAt(node,i));
    };
    visit(root);check(config && !objects.LiveObjects() && !system[41]);
    check(documents.AttributeToInt(documents.FindNumericAttribute(config,"Attr_Thunder_Local_Model"),0)==1);
    for(unsigned pass=0;pass<2;++pass) {
        // First pass uses unchanged original path settings. The second tests
        // the original optional smoothing branch through the bundled SDK.
        if(pass) documents.WriteIntAttribute(config,"Attr_Thunder_Sample",1);
        const auto before=memory.OutstandingAllocations();
        const auto object=objects.Create(config,0,address(context.data()));
        check(object && system[41]==1 && objects.ObjectTypeName(object)=="ThunderObject");
        auto& state=*reinterpret_cast<ThunderObjectState*>(object);
        check((state.words[1]&0xffu)==1 && state.words[28] && state.words[29] && state.words[34]);
        const auto show=objects.ResolveFacet(object,"ShowobjTask"),position=objects.ResolveFacet(object,"PositionTask"),
            scale=objects.ResolveFacet(object,"ScaleTask"),rotation=objects.ResolveFacet(object,"RotateTask"),
            tint=objects.ResolveFacet(object,"ChangeColorTask"),track=objects.ResolveFacet(object,"TrackTask");
        check(show==object+40 && position==object+28 && scale==object+20 && rotation==object+24 &&
            tint==object+44 && track==object+36 && objects.ResolveFacet(object,"MessageTask")==object+52);
        std::array<float,3> v{};objects.ReadScale(scale,v);check(v==std::array<float,3>{1,1,1});
        objects.Move(position,{3,4,5},false);objects.Move(position,{-3,-4,-5},true);
        check(objects.ReadPosition(position)==std::array<float,3>{0,0,0});
        objects.SetScale(scale,{2,3,4},0);objects.ReadScale(scale,v);check(v==std::array<float,3>{2,3,4});
        objects.ApplyRotation(rotation,{10,20,30},false);objects.ReadRotation(rotation,v);check(v==std::array<float,3>{10,20,30});
        objects.SetColor(tint,{12,34,56,180});objects.SetColorFacetEnabled(tint,1);
        std::array<std::uint8_t,4> color{};objects.ReadColor(tint,color);check(color==std::array<std::uint8_t,4>{12,34,56,180});
        check(objects.ControlEnabled(object+48)==1);objects.SetControlEnabled(object+48,0);
        check(objects.ControlEnabled(object+48)==0);objects.SetControlEnabled(object+48,1);
        objects.SetTrackPosition(track,{1,2,3});check(objects.ReadPosition(position)==std::array<float,3>{1,2,3});
        check(objects.ResetPlayback(object));
        check(objects.ReadPosition(position)==std::array<float,3>{0,500,0});
        objects.ReadColor(tint,color);check(color==std::array<std::uint8_t,4>{255,255,255,255});
        queues.QueueEffect(address(context.data()),object);objects.Show(show);
        check(queues.Count(92)==1 && queues.Contains(92,object));
        NativeParticleProbeCamera camera;camera.Begin(240);
        const auto old_camera=context[12];context[12]=address(camera.camera);
        std::array<RwUInt32,5> saved{};constexpr std::array<RwUInt32,5> states{7,14,19,20,27};
        for(unsigned i=0;i<states.size();++i)RwD3D9GetRenderState(states[i],&saved[i]);
        frames.Update(address(context.data()),0.04);frames.Draw(address(context.data()),address(camera.camera));
        for(unsigned i=0;i<states.size();++i){RwUInt32 value{};RwD3D9GetRenderState(states[i],&value);check(value==saved[i]);}
        const auto pixels=camera.EndPixels();
        check(pixels>0 && state.words[48]>state.words[47]);
        const auto segments=(state.words[48]-state.words[47])/24u;
        check(segments>=4 && (!pass || segments%4==0));
        objects.Hide(show);camera.Begin(240);frames.Draw(address(context.data()),address(camera.camera));check(!camera.EndPixels());
        auto* parent=RwFrameCreate();check(parent && RwFrameAddChild(parent,reinterpret_cast<RwFrame*>(objects.Frame(object))));
        objects.ReleasePlayback(object,queues);objects.ReleasePlayback(object,queues);
        check(!system[41] && !queues.Count(92) && !parent->child && !state.words[28] && !state.words[29] && !state.words[34]);
        objects.Destroy(object);(void)RwFrameDestroy(parent);context[12]=old_camera;
        check(!objects.LiveObjects() && memory.OutstandingAllocations()==before);
        std::cout<<"Original HW_XS013 Thunder: sample="<<pass<<" path_segments="<<segments<<" GPU_pixels="<<pixels
            <<" facets/reset/queue92/render-state/hide/parent/counter/name/texture cleanup verified\n";
    }
    {
        // Reproduce an SDK allocation exception after texture retention and
        // frame creation but before the initialized byte has been published.
        const auto held=textures.Load("gamedata\\effect\\tex\\HY\\Bolt.png");check(held);
        auto* texture=reinterpret_cast<RwTexture*>(held);const auto references=texture->refCount;
        auto* parent=RwFrameCreate();check(parent);
        {
            NativeThunderEffectObject effect(memory,documents,textures,address(system.data()),nullptr);
            auto& s=effect.State();RwTextureAddRef(texture);s.words[29]=held;
            s.words[28]=address(RwFrameCreate());s.words[34]=address(RwFrameCreate());
            check(s.words[28] && s.words[34] && RwFrameAddChild(parent,reinterpret_cast<RwFrame*>(s.words[28])));
            check(!(s.words[15]&0xffu));effect.ReleasePlayback(queues);
            check(!system[41] && !s.words[28] && !s.words[29] && !s.words[34] && !parent->child);
            bool retired{};try{(void)effect.Load(config);}catch(const std::logic_error&){retired=true;}check(retired);
        }
        check(texture->refCount==references);textures.Release(held);(void)RwFrameDestroy(parent);
    }
    // Missing required owner target services must roll back the provisional
    // object and counter; never silently replace the requested world target.
    documents.WriteIntAttribute(config,"Attr_Thunder_Local_Model",0);
    const auto before=memory.OutstandingAllocations();bool missing_target{};
    try{(void)objects.Create(config,1,address(context.data()));}
    catch(const std::logic_error&){missing_target=true;}
    check(missing_target && !objects.LiveObjects() && !system[41] && memory.OutstandingAllocations()==before);
    {
        NativeDialogObject owner(memory,documents);
        std::array<std::uint32_t,15> target_xml{};const auto target_root=address(target_xml.data());
        documents.InitializeDocument(target_root,"Effect",0);
        (void)documents.CreateChild(target_root,"OxnList");const auto list=documents.CreateChild(target_root,"TargetList");
        const auto target=documents.CreateChild(list,"Target");
        documents.WriteStringAttribute(target,"TargetName","1");
        documents.WriteFloatAttribute(target,"TargetPosX",3);documents.WriteFloatAttribute(target,"TargetPosY",4);documents.WriteFloatAttribute(target,"TargetPosZ",5);
        check(owner.Load(target_root));documents.DestroyDocument(target_root);owner.State().words[22]=1;
        NativeEffectDialogTargets targets([&](std::uint32_t d)->NativeDialogProperties& {check(d==owner.Address());return owner.Properties();});
        std::array<float,3> live{10,20,30};owner.Properties().SetFacet("1",address(live.data()));
        targets.BindFacetReader([&](std::uint32_t facet){check(facet==address(live.data()));return live;});
        const auto allocations=memory.OutstandingAllocations();
        {
            NativeThunderEffectObject effect(memory,documents,textures,address(system.data()),&targets);
            effect.State().words[3]=owner.Address();check(effect.Load(config));
            auto& s=effect.State();
            const auto vector=[&](unsigned index){return std::bit_cast<std::array<float,3>>(std::array<std::uint32_t,3>{s.words[index],s.words[index+1],s.words[index+2]});};
            check(vector(37)==live);
            live={20,30,40};check(effect.Reset());
            check(vector(37)==live && vector(70)==live);
            effect.Show(true);live={30,40,50};effect.Advance(.01);
            check(vector(37)==live && vector(70)==std::array<float,3>{20,30,40});
            s.words[14]|=1u;const auto position=vector(24);effect.SetTrackPosition({60,70,80});
            check(vector(37)==std::array<float,3>{60,70,80} && vector(24)==position);
            check(system[41]==1);

        }
        check(!system[41] && memory.OutstandingAllocations()==allocations);
    }
}
}
