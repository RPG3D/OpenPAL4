#pragma once
#include "pal4/native_scene_gob_catalog.h"
#include "pal4/native_gob_clump_asset.h"
#include "pal4/native_gob_object_image.h"
#include "pal4/native_gob_animation_binding.h"
#include "pal4/gob_machine.h"
#include "pal4/game_object_embedded_animation.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeStudyMachineAnimation(const std::filesystem::path& root,
    IGameResourceManagerRuntime& transforms,NativeBaseObjectMemory& memory,
    IRwDictSchemaRuntime& schema) {
    const auto require=[](bool value,const char* message){if(!value)throw std::runtime_error(message);};
    NativeSceneGobCatalog catalog;
    require(catalog.Load(root,"Q03","XN06"),"study chest original GOB catalog unavailable");
    const auto found=std::find_if(catalog.Records().begin(),catalog.Records().end(),
        [](const auto& record){return record.name=="item101";});
    require(found!=catalog.Records().end() && found->type==GobMachine::TypeKey(),
        "study item101 is an original Machine");
    const auto allocations=memory.OutstandingAllocations();
    {
        NativeGobClumpAsset model;
        require(model.Load(root,found->resource_dir,found->model),"study chest original clump unavailable");
        NativeGobObjectImage object(memory,schema);
        (void)object.Construct(found->type);
        object.BindRecord(*found,reinterpret_cast<std::uint32_t>(model.Clump()),transforms);
        NativeGobAnimationBinding animation(object,model,memory);
        require(animation.Load(found->model) && animation.Bound(),"study chest real animation unavailable");
        auto& state=*reinterpret_cast<GameObjectEmbeddedAnimationState*>(object.Address()+GobMachine::kStateOffset);
        require(state.words[19]==1u && state.words[17]==0u && animation.Time()==0.0f,
            "study machine must start paused without restart (IDA5A015A/5A0163)");
        for(const auto elapsed:{0.125f,0.75f,2.0f}){
            (void)animation.AdvanceFrame(elapsed);
            require(animation.Time()==0.0f,"untouched study chest must retain closed pose over real frame deltas");
        }
        const auto action=memory.ReadDword(state.words[3]+20u);
        require(action && animation.Select(reinterpret_cast<const char*>(action)),"machine action selection remains available");
        require(state.words[19]==0u,"explicit machine action releases initialization pause");
        (void)animation.AdvanceFrame(0.05f);
        require(animation.Time()>0.0f,"selected machine action still advances actual SDK interpolator");
    }
    require(memory.OutstandingAllocations()==allocations,"study chest isolated owners release their allocations");
    std::cout<<"Native study Machine animation: original item101 paused/no-restart, closed across deltas, explicit action advances, cleanup pass\n";
}
}
