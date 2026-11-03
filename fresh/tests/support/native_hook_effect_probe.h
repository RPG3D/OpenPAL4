#pragma once
#include "pal4/native_hook_effect_object.h"
#include "pal4/native_particle_skin_runtime.h"
#include "pal4/native_actor_user_data.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_event_document_runtime.h"
#include "pal4/native_effect_texture_dictionary.h"
#include "pal4/native_package_archive.h"
#include "native_particle_probe_camera.h"
#include <rpworld.h>
#include <rpptank.h>
#include <rpskin.h>
#include <rphanim.h>
#include <array>
#include <bit>
#include <cmath>
#include <iostream>
#include <source_location>

namespace pal4::test {
inline void VerifyNativeHookBoneTransform() {
    NativeBaseObjectMemory memory;
    NativeParticleSkinRuntime runtime(memory);
    struct Resources {
        RpSkin* skin{};
        RwFrame* frame=RwFrameCreate();
        ~Resources() { if(skin) RpSkinDestroy(skin);if(frame) RwFrameDestroy(frame); }
    } resources;
    if(!resources.frame) throw std::runtime_error("native hook bone test frame");
    RwMatrix inverse;RwMatrixSetIdentity(&inverse);
    const RwV3d offset{1.0f,0.0f,0.0f},axis{0.0f,0.0f,1.0f},position{10.0f,20.0f,30.0f};
    RwMatrixTranslate(&inverse,&offset,rwCOMBINEREPLACE);
    RwMatrixWeights weights{1.0f,0.0f,0.0f,0.0f};RwUInt32 indices{};
    resources.skin=RpSkinCreate(1u,1u,&weights,&indices,&inverse);
    if(!resources.skin) throw std::runtime_error("native hook bone test skin");
    (void)RwFrameRotate(resources.frame,&axis,90.0f,rwCOMBINEREPLACE);
    (void)RwFrameTranslate(resources.frame,&position,rwCOMBINEPOSTCONCAT);
    RpHAnimNodeInfo node{};node.pFrame=resources.frame;
    RpHAnimHierarchy hierarchy{};hierarchy.numNodes=1;hierarchy.pNodeInfo=&node;
    // Deliberately omit pMatrixArray: original645560 uses each node frame LTM.
    const auto s=reinterpret_cast<std::uint32_t>(resources.skin);
    const auto p=runtime.TransformBoneVertex(s,reinterpret_cast<std::uint32_t>(&hierarchy),0u,{2.0f,0.0f,0.0f});
    if(std::abs(p.x-10.0f)>0.0001f || std::abs(p.y-23.0f)>0.0001f || std::abs(p.z-30.0f)>0.0001f ||
        runtime.BoneIndices(s,0u)!=std::array<std::uint8_t,4>{0u,0u,0u,0u} ||
        runtime.BoneWeights(s,0u)!=std::array<float,4>{1.0f,0.0f,0.0f,0.0f})
        throw std::runtime_error("native hook SDK bone transform/order");
}
inline void VerifyNativeHookEffect(const std::filesystem::path& root,RpClump* clump) {
    VerifyNativeHookBoneTransform();
    const auto check=[](bool ok,const std::source_location at=std::source_location::current()) {
        if(!ok) throw std::runtime_error("native HookEffect line "+std::to_string(at.line()));
    };
    const auto address=[](const void* p){return reinterpret_cast<std::uint32_t>(p);};
    auto* emitter=NativeActorUserData::FindAtomic(clump,"hook_effect");
    std::cout<<"Original weapon HookEffect emitter="<<(emitter!=nullptr)<<'\n';
    check(emitter && RpGeometryGetNumMorphTargets(RpAtomicGetGeometry(emitter))==1);
    check(RpGeometryGetNumTriangles(RpAtomicGetGeometry(emitter))>1);
    NativeBaseObjectMemory memory;
    NativeEventDocumentRuntime documents(memory);
    std::array<std::uint32_t,15> document{};
    const auto d=address(document.data());
    (void)documents.InitializeDocument(d,"native-hook-probe",0u);
    NativePackageArchive archive;
    check(archive.Open((root/"gamedata/Effect.cpk").string().c_str()));
    const auto data=archive.ReadMember("script\\foot_2.eft");
    check(data && documents.LoadBinaryDocument(d,*data));
    const auto list=documents.FindChild(d,"OxnList");check(list);
    const auto config=documents.FindChild(list,"RtkPrtStdAtomicObject");check(config);
    std::array<std::uint32_t,13> owner{};
    NativeEffectTextureDictionary textures(root);textures.Initialize();
    const auto baseline=memory.OutstandingAllocations();
    {
        NativeHookEffectObject hook(memory,documents,textures);
        check(!hook.Load(config,0u,address(owner.data())) && !hook.Address());
        check(hook.Load(config,address(clump),address(owner.data())));
        auto* particle=hook.Particle();check(particle && particle->ReadByte(55u));
        auto* frame=reinterpret_cast<RwFrame*>(particle->ReadWord(44u));
        check(RwFrameGetParent(frame)==RpAtomicGetFrame(emitter));
        hook.Update(0.01);
        check(particle->ReadWord(208u)>0u && particle->ReadWord(64u) &&
            (particle->ReadWord(68u)-particle->ReadWord(64u))/12u==
                static_cast<std::uint32_t>(RpGeometryGetNumVertices(RpAtomicGetGeometry(emitter))));
        auto* tank=reinterpret_cast<RpAtomic*>(particle->ReadWord(40u));
        check(RpPTankAtomicGetActiveParticlesCount(tank)>0);
        const auto before=*RwFrameGetLTM(frame);
        const auto life=std::bit_cast<float>(particle->ReadWord(276u))+
            std::bit_cast<float>(particle->ReadWord(280u));
        check(hook.DrainTime()==life && !RwFrameGetParent(frame));
        const auto after=*RwFrameGetLTM(frame);
        check(std::abs(before.pos.x-after.pos.x)<0.0001f &&
            std::abs(before.pos.y-after.pos.y)<0.0001f && std::abs(before.pos.z-after.pos.z)<0.0001f);
        const auto slots=particle->ReadWord(204u);
        hook.Update(static_cast<double>(life)+0.1);
        check(!particle->ReadWord(208u) && particle->ReadWord(204u)==slots);
        hook.Stop();hook.Stop();check(!hook.Particle() && hook.Address());
    }
    check(memory.OutstandingAllocations()==baseline);
    // The same original hook emitter also supports collection visuals.
    documents.WriteIntAttribute(config,"Attr_Prt_Is_Billboard",1);
    {
        NativeHookEffectObject hook(memory,documents,textures);
        check(hook.Load(config,address(clump),address(owner.data())));
        check(hook.Particle() && hook.Particle()->ReadWord(124u)==1u);
        {
            NativeParticleProbeCamera camera;camera.Begin();
            owner[12]=address(camera.camera);
            hook.Update(0.01);hook.Draw(0u);
            (void)camera.EndPixels();owner[12]=0u;
        }
        hook.Stop();hook.Stop();check(!hook.Particle());
        documents.WriteIntAttribute(config,"Attr_Prt_Is_Billboard",0);
        NativeHookEffectObject tank_hook(memory,documents,textures);
        check(tank_hook.Load(config,address(clump),address(owner.data())));
        tank_hook.Update(0.01);
        // Render actual PTank contents before expiry, using the same small SDK
        // camera as other probes; its position follows the original hook.
        NativeParticleProbeCamera camera;
        const auto pos=RwFrameGetLTM(reinterpret_cast<RwFrame*>(tank_hook.Particle()->ReadWord(44u)))->pos;
        camera.BeginAt({pos.x,pos.y,pos.z-80.0f});
        tank_hook.Draw(0u);const auto pixels=camera.EndPixels();
        check(pixels>0u);
        std::cout<<"Native HookEffect original WTA emitter, foot_2 configuration, "
            <<pixels<<" GPU pixels; detach/drain/collection/allocation balance verified\n";
    }
    check(memory.OutstandingAllocations()==baseline);
    textures.Cleanup();documents.DestroyDocument(d);
    check(!memory.OutstandingAllocations());
}
}
