#pragma once
#include "pal4/rw_texture_dictionary.h"
#include "pal4/native_particle_configuration.h"
#include "pal4/native_effect_texture_dictionary.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_event_document_runtime.h"
#include "pal4/native_package_archive.h"
#include "pal4/particle_visual_object.h"
#include "native_particle_frame_probe.h"
#include <rwcore.h>
#include <rpworld.h>
#include <rpptank.h>
#include <array>
#include <bit>
#include <cmath>
#include <source_location>
#include <stdexcept>

namespace pal4::test {
inline void VerifyNativeParticleConfiguration(const std::filesystem::path& game_root) {
    const auto check=[](bool ok,const std::source_location at=std::source_location::current()) {
        if(!ok) throw std::runtime_error("native particle configuration line "+std::to_string(at.line()));
    };
    NativeBaseObjectMemory memory;
    NativeEventDocumentRuntime documents(memory);
    std::array<std::uint32_t,15> storage{};
    const auto root=reinterpret_cast<std::uint32_t>(storage.data());
    (void)documents.InitializeDocument(root,"particle-test",0u);
    {
        RwTextureDictionaryState dictionary;
        NativeEffectTextureDictionary textures(game_root,dictionary);
        check(!dictionary.dictionary);
        textures.Initialize();check(dictionary.dictionary);
        NativePackageArchive archive;
        check(archive.Open((game_root/"gamedata/Effect.cpk").string().c_str()));
        for(const auto name:{"foot_2","H_212"}) {
            auto data=archive.ReadMember((std::string("script\\")+name+".eft").c_str());
            check(data && documents.LoadBinaryDocument(root,*data));
            const auto list=documents.FindChild(root,"OxnList");check(list);
            unsigned loaded{};
            for(std::uint32_t i=0;i<documents.ChildCount(list);++i) {
                const auto node=documents.ChildAt(list,i);
                if(documents.NodeName(node)!="RtkPrtStdAtomicObject") continue;
                ++loaded;
                const auto baseline=memory.OutstandingAllocations();
                auto* previous=RwTexDictionaryGetCurrent();
                {
                    NativeParticleConfiguration owner(memory,documents,textures);
                    check(!owner.Load(0u) && owner.Load(node));
                    auto& state=owner.State();check((state.words[1]&0xffu)==1u && state.words[23]);
                    auto* resource=reinterpret_cast<ParticleVisualState*>(state.words[23]);
                    auto* atomic=reinterpret_cast<RpAtomic*>(resource->ReadWord(40u));
                    auto* frame=reinterpret_cast<RwFrame*>(resource->ReadWord(44u));
                    check(atomic && frame && RwTexDictionaryGetCurrent()==previous);
                    auto* texture=RpMaterialGetTexture(RpGeometryGetMaterial(RpAtomicGetGeometry(atomic),0));
                    check(texture && texture->refCount==1 &&
                        RwTextureGetDictionary(texture)==reinterpret_cast<RwTexDictionary*>(dictionary.dictionary));
                    check(RpPTankAtomicGetMaximumParticlesCount(atomic)==(std::string_view(name)=="foot_2"?9:50));
                    check(resource->ReadWord(260u)==(std::string_view(name)=="foot_2"?7u:30u));
                    check(std::bit_cast<float>(resource->ReadWord(276u))==(std::string_view(name)=="foot_2"?0.7f:1.5f));
                    RwBlendFunction src{},dst{};RpPTankAtomicGetBlendModes(atomic,&src,&dst);
                    check(src==rwBLENDSRCALPHA && dst==rwBLENDONE);
                    const auto* matrix=RwFrameGetMatrix(frame);
                    if(std::string_view(name)=="foot_2")
                        check(std::abs(matrix->right.x-std::cos(5.0f*3.14159265358979323846f/180.0f))<0.00001f);
                    else check(matrix->pos.y==50.0f);
                    bool busy=false;try{(void)owner.Load(node);}catch(const std::logic_error&){busy=true;}check(busy);
                    VerifyNativeParticleFrame(owner,memory);
                    {
                        NativeParticleConfiguration second(memory,documents,textures);
                        check(second.Load(node));
                        auto* other=reinterpret_cast<ParticleVisualState*>(second.State().words[23]);
                        auto* other_atomic=reinterpret_cast<RpAtomic*>(other->ReadWord(40u));
                        check(RpMaterialGetTexture(RpGeometryGetMaterial(RpAtomicGetGeometry(other_atomic),0))==texture && texture->refCount==2);
                        second.Clear();second.Clear();check(texture->refCount==1);
                    }
                    owner.Clear();check(!owner.State().words[23]);
                    // Actual XML collection variant creates and renders the same
                    // original texture through geoPlane owners instead of PTank.
                    documents.WriteIntAttribute(node,"Attr_Prt_Is_Billboard",1);
                    check(owner.Load(node));
                    const auto* collection=reinterpret_cast<const ParticleVisualState*>(owner.State().words[23]);
                    check(collection && collection->ReadWord(124u)==1u && !collection->ReadWord(40u) &&
                        collection->ReadWord(180u)-collection->ReadWord(176u)==collection->ReadWord(200u)*4u);
                    NativeParticleProbeCamera collection_camera;collection_camera.Begin();
                    (void)owner.Update(0.01,reinterpret_cast<std::uint32_t>(collection_camera.camera));
                    (void)owner.Draw();(void)collection_camera.EndPixels();
                    owner.Clear();owner.Clear();check(!owner.State().words[23] && RwTexDictionaryGetCurrent()==previous);
                    documents.WriteIntAttribute(node,"Attr_Prt_Is_Billboard",0);
                    const auto texture_attribute=documents.FindNumericAttribute(node,"Attr_Prt_Texture");
                    const auto texture_name=documents.AttributeToString(texture_attribute);
                    documents.SetAttributeStringValue(texture_attribute,"HY\\__missing_native_particle_probe.png");
                    bool rejected=false;
                    try{(void)owner.Load(node);}catch(const std::runtime_error&){rejected=true;}
                    check(rejected && !owner.State().words[23] && RwTexDictionaryGetCurrent()==previous);
                    documents.SetAttributeStringValue(texture_attribute,texture_name);
                    check(owner.Load(node)); // Automatic destruction of a live tank.
                }
                check(memory.OutstandingAllocations()==baseline);
            }
            check(loaded==1u);
        }
    }
    documents.DestroyDocument(root);
    check(!documents.DocumentCount() && !memory.OutstandingAllocations());
}
}
