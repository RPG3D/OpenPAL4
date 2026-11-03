#pragma once
#include "native_m07_machine_probe.h"
#include "pal4/gob_obstacle.h"
#include "pal4/gob_obstacle_snapshot.h"

namespace pal4::test {
// Object-image boundary only. The scene must retain its rejection until the
// separately owned physics/status/helper services are composed and tested.
inline void VerifyNativeQ06ObstacleImage(const std::filesystem::path& root,
    IGameResourceManagerRuntime& transforms,NativeBaseObjectMemory& memory,
    IRwDictSchemaRuntime& schema){
    const auto check=[](bool ok,const char* text){if(!ok)throw std::runtime_error(text);};
    NativeSceneGobCatalog catalog;check(catalog.Load(root,"Q06","Q06Q"),"Q06 actual Obstacle catalog");
    const auto it=std::find_if(catalog.Records().begin(),catalog.Records().end(),
        [](const auto& r){return r.name=="burning01";});
    check(it!=catalog.Records().end() && it->type==10u,"Q06 burning01 is original type10");
    const auto allocated=memory.OutstandingAllocations();
    for(unsigned pass=0;pass<2u;++pass){
        {
            NativeGobClumpAsset model;check(model.Load(root,it->resource_dir,it->model),"Q06 original JDumy model");
            NativeGobObjectImage object(memory,schema);const auto p=object.Construct(it->type);
            check(memory.ReadDword(p)==GobObstacle::kVtable &&
                memory.ReadDword(p+GobObstacle::kStateOffset)==GobObstacle::kStateVtable,
                "Obstacle preserves derived object/animation vtables");
            object.BindRecord(*it,reinterpret_cast<std::uint32_t>(model.Clump()),transforms);
            check(memory.ReadDword(p+0x2d4u)==p && memory.ReadDword(p+0x170u)==0u,
                "Obstacle retains helper owner and original selfvisibility");
            check(memory.ReadDword(p+0x2d8u)==0u && memory.ReadDword(p+0x2dcu)==0u &&
                memory.ReadDword(p+0x2f8u)==2u &&
                memory.ReadDword(p+0x2fcu)==std::bit_cast<std::uint32_t>(3.f) &&
                memory.ReadDword(p+0x300u)==std::bit_cast<std::uint32_t>(3.f) &&
                memory.ReadDword(p+0x304u)==std::bit_cast<std::uint32_t>(10.f),
                "Obstacle retains original force/status fields");
            check(memory.ReadDword(p+0x30cu)==0u && memory.ReadDword(p+0x324u)==0u &&
                memory.ReadDword(p+0x31cu)==0u && memory.ReadDword(p+0x32cu)==0xffffffffu &&
                memory.ReadDword(p+0x330u)==0u,"empty launch strings do not overwrite adjacent fields");
            (void)object.SetVisible(1u);check(memory.ReadDword(p+0x184u)==1u &&
                memory.ReadDword(p+0x170u)==0u,"Obstacle Machine visibility preserves self-hidden geometry");
            check(!object.CaptureSaveDocument(),"original burning01 store0 does not serialize");
            // The opt-in store flag exercises the real name-only snapshot ABI,
            // not a gameplay bypass. This is an isolated resource fixture.
            memory.WriteDword(p+0x1a4u,1u);
            const auto saved=object.CaptureSaveDocument();check(saved.has_value(),"Obstacle name-only snapshot capture");
            check(saved->Root().name=="soGOBObstacle" && saved->Root().attributes.size()==1u,
                "Obstacle snapshot contains exactly valName");
            object.ApplySaveDocument(*saved);
            auto wrong_root=saved->Root();wrong_root.attributes.front().value=std::string("different_obstacle");
            const auto wrong_bytes=BinaryXmlConfig::Serialize(wrong_root);check(wrong_bytes.has_value(),"wrong-owner fixture serializes");
            BinaryXmlConfig wrong;check(wrong.Parse(*wrong_bytes),"wrong-owner fixture parses");
            bool rejected{};try{object.ApplySaveDocument(wrong);}catch(const std::invalid_argument&){rejected=true;}
            check(rejected,"Obstacle named restore rejects a different owner");
            object.ReloadConfiguration(*it);
            check(memory.ReadDword(p+0x2d4u)==p && memory.ReadDword(p+0x31cu)==0u,
                "Obstacle config reload retains owner and resets launch state");
            auto launch_root=it->config.Root();
            for(auto& parameters:launch_root.children)
                for(auto& node:parameters.children)
                    if(node.name=="PAL4_GameObject-obstacle"){
                        node.attributes.push_back({"PAL4_GameObject-machine-repeat",1u,std::int32_t(1)});
                        for(auto& attribute:node.attributes){
                            if(attribute.name=="CSTR_GAMEOBJECT_obstacle_launch_name")attribute.value=std::string("fixture_target");
                            if(attribute.name=="CSTR_GAMEOBJECT_obstacle_launchEff")attribute.value=std::string("fixture_effect");
                        }
                    }
            const auto launch_bytes=BinaryXmlConfig::Serialize(launch_root);
            check(launch_bytes.has_value(),"launch-string fixture serializes");
            auto launch_record=*it;check(launch_record.config.Parse(*launch_bytes),"launch-string fixture parses");
            object.ReloadConfiguration(launch_record);
            check(memory.ReadDword(p+0x280u)==1u,"inherited Machine reads the derived Obstacle config section");
            check(memory.ReadDword(p+0x30cu)==14u && memory.ReadDword(p+0x324u)==14u &&
                std::strcmp(reinterpret_cast<const char*>(memory.ReadDword(p+0x308u)),"fixture_target")==0 &&
                std::strcmp(reinterpret_cast<const char*>(memory.ReadDword(p+0x320u)),"fixture_effect")==0 &&
                memory.ReadDword(p+0x314u)==0u && memory.ReadDword(p+0x32cu)==0xffffffffu && memory.ReadDword(p+0x330u)==0u,
                "nonempty twelve-byte launch strings preserve adjacent time/effect records");
            object.ReloadConfiguration(*it);
            check(memory.ReadDword(p+0x308u)==0u && memory.ReadDword(p+0x320u)==0u,
                "reload to original empty launch strings releases both allocations");
        }
        check(memory.OutstandingAllocations()==allocated,"Obstacle image/snapshot/reload destruction cleans up");
    }
    std::cout<<"Native Q06 Obstacle image: real burning01 constructor/config/visibility/name-only snapshot/reload and repeated ownership cleanup pass; physics/helper scene composition pending\n";
}
}
