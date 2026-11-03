#pragma once

#include "pal4/native_strip_visual_runtime.h"
#include "pal4/native_strip_effect_object.h"
#include "pal4/native_dynamic_strip_object.h"
#include "pal4/native_dialog_particle_effects.h"
#include "pal4/effect_system_callbacks.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_actor_user_data.h"
#include "pal4/native_event_document_runtime.h"
#include "pal4/native_effect_texture_dictionary.h"
#include "pal4/native_package_archive.h"
#include "pal4/dynamic_strip_object_update.h"
#include "native_particle_probe_camera.h"
#include <rpworld.h>
#include <rpusrdat.h>
#include <array>
#include <algorithm>
#include <bit>
#include <cstring>
#include <iostream>
#include <source_location>

namespace pal4::test {
inline void VerifyNativeStripVisual(const std::filesystem::path& root) {
    const auto check=[](bool ok,const std::source_location at=std::source_location::current()) {
        if(!ok) throw std::runtime_error("native Strip visual line "+std::to_string(at.line()));
    };
    const auto address=[](const void* p){return reinterpret_cast<std::uint32_t>(p);};
    struct Resources {
        RpClump* clump=RpClumpCreate();
        RwFrame* root=RwFrameCreate();
        RwRaster* raster=RwRasterCreate(4,4,32,rwRASTERTYPETEXTURE|rwRASTERFORMAT8888);
        RwTexture* texture{};
        ~Resources() {
            if(texture) RwTextureDestroy(texture);
            else if(raster) RwRasterDestroy(raster);
            if(clump) RpClumpDestroy(clump);
            if(root) RwFrameDestroyHierarchy(root);
        }
    } resources;
    check(resources.clump && resources.root && resources.raster);
    check(RpClumpSetFrame(resources.clump,resources.root));
    auto* pixels=RwRasterLock(resources.raster,0,rwRASTERLOCKWRITE);
    check(pixels);
    for(int y=0;y<4;++y) std::memset(pixels+y*resources.raster->stride,255,16);
    check(RwRasterUnlock(resources.raster));
    resources.texture=RwTextureCreate(resources.raster);check(resources.texture);
    std::array<RwFrame*,4> frames{};
    const char* names[]{"hook_blade_a","hook_blade_b","hook_blade_c","hook_blade_d"};
    for(unsigned i=0;i<4u;++i) {
        auto* atomic=RpAtomicCreate();auto* geometry=RpGeometryCreate(1,0,rpGEOMETRYPOSITIONS);
        frames[i]=RwFrameCreate();check(atomic && geometry && frames[i]);
        char array_name[]="name";
        const auto index=RpGeometryAddUserDataArray(geometry,array_name,rpSTRINGUSERDATA,1);
        check(index>=0);
        RpUserDataArraySetString(RpGeometryGetUserDataArray(geometry,index),0,const_cast<char*>(names[i]));
        check(RpAtomicSetGeometry(atomic,geometry,0));RpGeometryDestroy(geometry);
        check(RpAtomicSetFrame(atomic,frames[i]) && RwFrameAddChild(resources.root,frames[i]));
        check(RpClumpAddAtomic(resources.clump,atomic));
        const RwV3d pos{-10.0f,(i%2u) ? 10.0f : -10.0f,0.0f};
        check(RwFrameTranslate(frames[i],&pos,rwCOMBINEREPLACE));
        check(NativeActorUserData::FindAtomic(resources.clump,names[i])==atomic);
    }
    NativeBaseObjectMemory memory;
    NativeStripVisualRuntime runtime(memory);
    struct Guarded {
        std::uint32_t before=0x12345678u;
        std::array<std::uint32_t,34> words{};
        std::uint32_t after=0x87654321u;
    } state;
    const StripVisualBlockView block{state.words};
    runtime.Construct(block);
    check(memory.OutstandingAllocations()==1u);
    check(!runtime.Setup(block,address(resources.texture),0u,2.0f,true));
    const auto refs=resources.texture->refCount;
    check(runtime.Setup(block,address(resources.texture),address(resources.clump),2.0f,true));
    state.words[13u]=0xffffffffu;
    check(resources.texture->refCount==refs+1 && state.words[25u]==address(frames[0]) &&
          state.words[26u]==address(frames[1]));
    runtime.Update(block,0.01,false);check(state.words[29u]==1u);
    const RwV3d move{20.0f,0.0f,0.0f};
    check(RwFrameTranslate(frames[0],&move,rwCOMBINEPOSTCONCAT) &&
          RwFrameTranslate(frames[1],&move,rwCOMBINEPOSTCONCAT));
    runtime.Update(block,0.01,false);check(state.words[29u]==2u);
    const auto first=memory.ReadDword(state.words[28u]);
    DynamicStripSegment segment{};
    std::memcpy(&segment,reinterpret_cast<const void*>(first+8u),sizeof(segment));
    check(segment.first[0]==10.0f && segment.second[0]==10.0f);
    NativeParticleProbeCamera camera;
    camera.Begin();
    const std::uint32_t states[]{27u,19u,20u,7u,14u};
    RwUInt32 saved[5]{};
    for(unsigned i=0;i<5u;++i) RwD3D9GetRenderState(states[i],&saved[i]);
    RwUInt32 cull{};check(RwRenderStateGet(static_cast<RwRenderState>(20u),&cull));
    runtime.Draw(StripVisualBlockConstView{state.words});
    for(unsigned i=0;i<5u;++i) { RwUInt32 value{};RwD3D9GetRenderState(states[i],&value);check(value==saved[i]); }
    RwUInt32 after_cull{};check(RwRenderStateGet(static_cast<RwRenderState>(20u),&after_cull) && after_cull==cull);
    const auto colored=camera.EndPixels();check(colored>0u);
    // Drain must no longer sample emitter frames: make the stored frame pointers
    // invalid after initial capture, then expire all nodes without dereferencing.
    state.words[25u]=state.words[26u]=0u;
    runtime.Update(block,3.0,true);check(!state.words[29u]);
    runtime.Cleanup(block);runtime.Cleanup(block);
    check(resources.texture->refCount==refs && memory.OutstandingAllocations()==1u);
    check(runtime.Setup(block,address(resources.texture),address(resources.clump),1.0f,false));
    check(state.words[25u]==address(frames[2]) && state.words[26u]==address(frames[3]));
    runtime.Update(block,0.01,false);
    runtime.Destroy(block);runtime.Destroy(block);
    check(!memory.OutstandingAllocations() && resources.texture->refCount==refs);
    for(auto* f:frames) check(RwFrameGetParent(f)==resources.root);
    check(state.before==0x12345678u && state.after==0x87654321u);
    std::cout<<"Native Strip visual: "<<colored<<" GPU pixels, real SDK hook pairs, moving segments, "
        <<"drain without frame reads, texture refs, borrowed frames and allocation balance verified\n";

    // Reuse an actual original texture and the real binary-XML adapter. Strip
    // attributes are explicitly authored here; this is not an original HEF.
    NativeEventDocumentRuntime documents(memory);
    std::array<std::uint32_t,15> document{};
    const auto d=address(document.data());
    (void)documents.InitializeDocument(d,"native-strip-probe",0u);
    NativePackageArchive archive;
    check(archive.Open((root/"gamedata/Effect.cpk").string().c_str()));
    const auto bytes=archive.ReadMember("script\\foot_2.eft");
    check(bytes && documents.LoadBinaryDocument(d,*bytes));
    const auto config=documents.FindChild(documents.FindChild(d,"OxnList"),"RtkPrtStdAtomicObject");
    check(config);
    const auto texture_name=documents.AttributeToString(documents.FindNumericAttribute(config,"Attr_Prt_Texture"));
    check(!texture_name.empty());
    documents.WriteStringAttribute(config,"Attr_StripSystem_Texture",texture_name);
    documents.WriteIntAttribute(config,"Attr_StripSystem_AlphaBlend",1);
    documents.WriteFloatAttribute(config,"Attr_StripSystem_Life",1.0f);
    documents.WriteIntAttribute(config,"Hook_Tag",0);
    NativeEffectTextureDictionary textures(root);
    const auto baseline=memory.OutstandingAllocations();
    {
        NativeStripEffectObject effect(memory,documents,textures);
        check(!effect.Load(config,0u,address(resources.root)) && !effect.Address());
        documents.WriteIntAttribute(config,"Hook_Tag",0);
        check(!effect.Load(config,address(resources.clump),address(resources.root)) && !effect.Address());
        check(memory.OutstandingAllocations()==baseline);
        documents.WriteIntAttribute(config,"Hook_Tag",1);
        documents.WriteFloatAttribute(config,"Attr_StripSystem_Life",0.0f);
        check(!effect.Load(config,address(resources.clump),address(resources.root)) && !effect.Address());
        check(memory.OutstandingAllocations()==baseline);
        documents.WriteFloatAttribute(config,"Attr_StripSystem_Life",1.0f);
        check(effect.Load(config,address(resources.clump),address(resources.root)));
        const auto e=effect.Address();
        check(memory.ReadByte(e+4u) && memory.ReadByte(e+6u));
        check(memory.OutstandingAllocations()==baseline+3u);
        effect.Update(0.01);
        const RwV3d shift{-20.0f,0.0f,0.0f};
        for(auto* f:frames) check(RwFrameTranslate(f,&shift,rwCOMBINEPOSTCONCAT));
        effect.Update(0.01);
        check(memory.ReadDword(e+124u)==2u && memory.ReadDword(e+260u)==2u);
        camera.Begin();effect.Draw(0u);const auto effect_pixels=camera.EndPixels();check(effect_pixels>0u);
        check(effect.DrainTime()==1.0f && memory.ReadByte(e+5u));
        effect.Update(2.0);
        check(!memory.ReadDword(e+124u) && !memory.ReadDword(e+260u));
        effect.Stop();effect.Stop();
        check(!memory.ReadByte(e+4u) && !memory.ReadByte(e+6u));
        std::cout<<"Native StripSystemEffect: dual embedded blocks, original texture, "<<effect_pixels
            <<" GPU pixels, XML gates, drain and repeated stop verified\n";
    }
    check(memory.OutstandingAllocations()==baseline);
    auto* optional_hook=NativeActorUserData::FindAtomic(resources.clump,"hook_blade_c");
    check(optional_hook && RpClumpRemoveAtomic(resources.clump,optional_hook));
    {
        NativeStripEffectObject effect(memory,documents,textures);
        check(effect.Load(config,address(resources.clump),address(resources.root)));
        check(memory.ReadByte(effect.Address()+4u) && !memory.ReadByte(effect.Address()+6u));
        effect.Update(0.01);
        check(memory.ReadDword(effect.Address()+124u)==1u && !memory.ReadDword(effect.Address()+260u));
    }
    check(memory.OutstandingAllocations()==baseline && RpClumpAddAtomic(resources.clump,optional_hook));
    auto* required_hook=NativeActorUserData::FindAtomic(resources.clump,"hook_blade_a");
    check(required_hook && RpClumpRemoveAtomic(resources.clump,required_hook));
    {
        NativeStripEffectObject effect(memory,documents,textures);
        check(!effect.Load(config,address(resources.clump),address(resources.root)) && !effect.Address());
    }
    check(memory.OutstandingAllocations()==baseline && RpClumpAddAtomic(resources.clump,required_hook));
    documents.WriteFloatAttribute(config,"Attr_StripSystem_Width",20.0f);
    const auto dynamic_baseline=memory.OutstandingAllocations();
    {
        std::array<std::uint32_t,42> system{};
        struct Queues final : INativeDialogParticleQueues {
            unsigned removes{};
            void QueueEffect(std::uint32_t,std::uint32_t) override{}
            void RemoveEffect(std::uint32_t,std::uint32_t) override{++removes;}
        } queues;
        NativeDynamicStripObject effect(memory,documents,textures,address(system.data()));
        const auto image_addresses=memory.AllocationAddresses();
        // Factory image and the shared constructor's 36-byte list sentinel.
        check(memory.OutstandingAllocations()==dynamic_baseline+2u &&
            std::find(image_addresses.begin(),image_addresses.end(),address(&effect.State()))!=image_addresses.end() &&
            std::find(image_addresses.begin(),image_addresses.end(),effect.State().words[41])!=image_addresses.end());
        check(system[41]==1 && effect.Load(config) && effect.Frame());
        auto& s=effect.State();
        check((s.words[1]&0xffu) && memory.OutstandingAllocations()==dynamic_baseline+2u);
        std::array<float,3> scale{};effect.ReadScale(scale);check(scale==std::array<float,3>{1,1,1});
        effect.SetScale({2,1,1},0);effect.Move({0,-10,0},false);effect.Rotate({0,0,0},false);
        std::array<float,3> position{};effect.ReadPosition(position);check(position==std::array<float,3>{0,-10,0});
        check(std::abs(RwFrameGetLTM(reinterpret_cast<RwFrame*>(effect.Frame()))->right.x-2)<0.001f);
        effect.Show(true);effect.Advance(0.01);check(s.words[42]==1);
        effect.SetTrackPosition({0,10,0});effect.Advance(0.01);check(s.words[42]==2);
        effect.SetColor({255,255,255,255});
        class Proximity final : public IEffectProximityRuntime {
            std::uint8_t ReadByte(std::uint32_t) override{throw std::logic_error("unexpected DynamicStrip proximity read");}
            std::uint32_t ReadWord(std::uint32_t) override{throw std::logic_error("unexpected DynamicStrip proximity read");}
            float ReadFloat(std::uint32_t) override{throw std::logic_error("unexpected DynamicStrip proximity read");}
            EffectPosition ObjectPosition(std::uint32_t) override{throw std::logic_error("unexpected DynamicStrip proximity read");}
        } proximity;
        camera.Begin();effect.Draw(0,0,proximity);const auto width_pixels=camera.EndPixels();check(width_pixels>0);
        const auto frame=effect.Frame();effect.SetMode(1);effect.Advance(2.0);check(!s.words[42] && effect.Frame()==frame);
        check(effect.Reset() && !s.words[42]);effect.ReadScale(scale);effect.ReadPosition(position);
        check(scale==std::array<float,3>{1,1,1} && position==std::array<float,3>{});
        effect.Show(true);effect.Advance(0.01);check(s.words[42]==1);
        effect.Show(false);effect.Advance(2.0);check(s.words[42]==1); // Hidden update gate.
        effect.ReleasePlayback(queues);effect.ReleasePlayback(queues);
        check(!effect.Frame() && queues.removes==1 && memory.OutstandingAllocations()==dynamic_baseline+2u);
        std::cout<<"Native DynamicStrip: owned width endpoints, real texture, "<<width_pixels
            <<" GPU pixels, correct scale/position fields, track move, drain/reset/hidden gate and retirement verified\n";
    }
    check(memory.OutstandingAllocations()==dynamic_baseline);
    textures.Cleanup();documents.DestroyDocument(d);
    check(!memory.OutstandingAllocations());
}
}  // namespace pal4::test
