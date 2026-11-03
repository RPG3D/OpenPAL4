#pragma once
#include "pal4/native_effect_xml_objects.h"
#include "pal4/native_effect_group_frame.h"
#include "pal4/native_effect_active_frame.h"
#include "pal4/native_event_document_runtime.h"
#include "pal4/native_package_archive.h"
#include "native_particle_probe_camera.h"
#include <rpusrdat.h>
#include <rpworld.h>
#include <array>
#include <cstring>
#include <iostream>
#include <source_location>

namespace pal4::test {
inline void VerifyNativeEffectXml(const std::filesystem::path& root,
    NativeBaseObjectMemory& memory,NativeEventDocumentRuntime& documents,
    NativeEffectTextureDictionary& textures,NativeEffectTreeStorage& trees,NativePkFileManagerRuntime& packages,
    std::uint32_t group,
    IEffectHandlePropertyRuntime& properties,NativeEffectActiveFrame& active,NativeEffectGroupFrame& frames) {
    const auto check=[](bool good,const std::source_location at=std::source_location::current()) {
        if(!good) throw std::runtime_error("native XML effect line "+std::to_string(at.line()));
    };
    const auto address=[](const void* p){return reinterpret_cast<std::uint32_t>(p);};
    struct Hooks {
        RpClump* clump=RpClumpCreate();RwFrame* root=RwFrameCreate();
        std::array<RwFrame*,5> frames{};
        ~Hooks(){if(clump) RpClumpDestroy(clump);if(root) RwFrameDestroyHierarchy(root);}
    } hooks;
    check(hooks.clump && hooks.root && RpClumpSetFrame(hooks.clump,hooks.root));
    const char* names[]{"hook_blade_a","hook_blade_b","hook_blade_c","hook_blade_d","hook_effect"};
    for(unsigned i=0;i<5u;++i) {
        auto* atomic=RpAtomicCreate();auto* geometry=RpGeometryCreate(1,0,rpGEOMETRYPOSITIONS);
        hooks.frames[i]=RwFrameCreate();check(atomic && geometry && hooks.frames[i]);
        RpMorphTargetGetVertices(RpGeometryGetMorphTarget(geometry,0))[0]={0.0f,0.0f,0.0f};
        char key[]="name";const auto index=RpGeometryAddUserDataArray(geometry,key,rpSTRINGUSERDATA,1);check(index>=0);
        RpUserDataArraySetString(RpGeometryGetUserDataArray(geometry,index),0,const_cast<char*>(names[i]));
        check(RpAtomicSetGeometry(atomic,geometry,0));RpGeometryDestroy(geometry);
        check(RpAtomicSetFrame(atomic,hooks.frames[i]) && RwFrameAddChild(hooks.root,hooks.frames[i]));
        check(RpClumpAddAtomic(hooks.clump,atomic));
        const RwV3d p{-10.0f,(i%2u) ? 10.0f : -10.0f,0.0f};
        check(RwFrameTranslate(hooks.frames[i],&p,rwCOMBINEREPLACE));
    }
    const auto initial=memory.OutstandingAllocations();
    std::array<std::uint32_t,15> doc{};const auto d=address(doc.data());
    (void)documents.InitializeDocument(d,"native-xml-effect-probe",0u);
    NativePackageArchive archive;check(archive.Open((root/"gamedata/Effect.cpk").string().c_str()));
    const auto bytes=archive.ReadMember("script\\foot_2.eft");check(bytes && documents.LoadBinaryDocument(d,*bytes));
    const auto config=documents.FindChild(documents.FindChild(d,"OxnList"),"RtkPrtStdAtomicObject");check(config);
    const auto tex=documents.AttributeToString(documents.FindNumericAttribute(config,"Attr_Prt_Texture"));check(!tex.empty());
    documents.WriteIntAttribute(config,"Hook_Tag",0);
    documents.WriteStringAttribute(config,"Attr_StripSystem_Texture",tex);
    documents.WriteFloatAttribute(config,"Attr_StripSystem_Life",0.25f);
    documents.WriteIntAttribute(config,"Attr_StripSystem_AlphaBlend",1);
    const auto baseline=memory.OutstandingAllocations();
    {
        NativeEffectXmlObjects objects(memory,documents,textures,trees,packages,group);
        NativeEffectHandleContextRuntime handles(memory,trees,group,properties,objects);
        frames.BindContextEffects(&objects);
        struct Unbind {
            NativeEffectGroupFrame& frames;NativeEffectActiveFrame& active;
            ~Unbind(){active.Cleanup();frames.BindContextEffects(nullptr);}
        } unbind{frames,active};
        NativeParticleProbeCamera camera;camera.Begin();frames.Draw(0u);check(!camera.EndPixels());
        const auto packages_before=packages.LoadedPackageCount();
        const auto documents_before=documents.DocumentCount();
        const auto allocations_before=memory.OutstandingAllocations();
        const auto file_id=memory.ReadDword(group+8u);
        check(!objects.LoadFile("__missing_native_hef__",address(hooks.clump)).system);
        check(memory.ReadDword(group+8u)==file_id && !objects.Count());
        check(packages.LoadedPackageCount()==packages_before && documents.DocumentCount()==documents_before);
        // Load an unmodified original HEF through the real scoped package path.
        // The geometry hooks are constructed; this is not weapon-name selection.
        for(unsigned pass=0;pass<2u;++pass) {
            const auto original=objects.LoadFile("ZL-20",address(hooks.clump));
            check(original.system==group && original.id==file_id+pass && objects.Count()==1u);
            check(packages.LoadedPackageCount()==packages_before && documents.DocumentCount()==documents_before);
            frames.Update(0.05);
            camera.Begin();frames.Draw(0u);(void)camera.EndPixels();
            EffectSystemHandleWords file_handle{original.id,original.system};
            EffectHandle::RemoveContext(file_handle,handles);
            check(!file_handle.system && !objects.Count());
            check(memory.OutstandingAllocations()==allocations_before);
        }
        memory.WriteByte(group+4u,0u);
        check(!objects.LoadFile("ZL-20",address(hooks.clump)).system);
        memory.WriteByte(group+4u,1u);
        check(packages.LoadedPackageCount()==packages_before && documents.DocumentCount()==documents_before);
        check(memory.ReadDword(group+8u)==file_id+2u);
        const auto next_id=memory.ReadDword(group+8u);
        auto invalid=objects.Load(config,0u);check(invalid.id==0xffffffffu && !invalid.system && !objects.Count());
        check(memory.ReadDword(group+8u)==next_id);
        auto hook=objects.Load(config,address(hooks.clump));
        EffectSystemHandleWords h{hook.id,hook.system};
        check(hook.id==next_id && hook.system==group && objects.Count()==1u &&
              memory.ReadDword(group+240u)==1u && EffectHandle::IsReadyToShow(h,handles));
        frames.Update(0.05);
        camera.Begin();frames.Draw(0u);const auto hook_pixels=camera.EndPixels();check(hook_pixels>0u);
        const auto stale=h;
        EffectHandle::DisplayAndCleanup(h,handles);
        check(!h.system && !memory.ReadDword(group+240u) && memory.ReadDword(group+256u)==1u);
        check(!EffectHandle::IsReadyToShow(stale,handles));
        camera.Begin();frames.Draw(0u);check(camera.EndPixels()>0u);
        frames.Update(20.0);check(!memory.ReadDword(group+256u) && !objects.Count());
        documents.WriteIntAttribute(config,"Hook_Tag",1);
        auto strip=objects.Load(config,address(hooks.clump));
        EffectSystemHandleWords s{strip.id,strip.system};check(s.system==group && objects.Count()==1u);
        const auto node=trees.LowerBoundContext(group+228u,s.id);
        const auto object=memory.ReadDword(node+16u);
        check(memory.ReadDword(object)==0x84b21cu); // Actual fallback is Strip.
        frames.Update(0.01);
        const RwV3d move{20.0f,0.0f,0.0f};
        for(unsigned i=0;i<4u;++i) check(RwFrameTranslate(hooks.frames[i],&move,rwCOMBINEPOSTCONCAT));
        frames.Update(0.01);
        camera.Begin();frames.Draw(0u);const auto strip_pixels=camera.EndPixels();check(strip_pixels>0u);
        EffectHandle::DisplayAndCleanup(s,handles);frames.Update(0.3);
        check(!objects.Count() && !memory.ReadDword(group+256u));
        auto immediate=objects.Load(config,address(hooks.clump));
        EffectSystemHandleWords i{immediate.id,immediate.system};
        EffectHandle::RemoveContext(i,handles);check(!i.system && !objects.Count());
        // Populate both trees, then exercise the actual shared group cleanup.
        auto timed=objects.Load(config,address(hooks.clump));
        EffectSystemHandleWords t{timed.id,timed.system};frames.Update(0.01);
        EffectHandle::DisplayAndCleanup(t,handles);
        check(objects.Load(config,address(hooks.clump)).system==group && objects.Count()==2u);
        active.Cleanup();active.Cleanup();
        check(!objects.Count() && !memory.ReadDword(group+240u) && !memory.ReadDword(group+256u));
        check(memory.OutstandingAllocations()==baseline);
        std::cout<<"Native XML effect group: Hook "<<hook_pixels<<" pixels, Strip "<<strip_pixels
            <<" pixels; ID registration, fallback, real handles, context/timed frames, expiry and populated cleanup verified\n";
        std::cout<<"Native original HEF: ZL-20 scoped package load/update/draw/remove twice, missing and disabled gates; temporary documents and package refs released\n";
    }
    documents.DestroyDocument(d);check(memory.OutstandingAllocations()==initial);
}
}
