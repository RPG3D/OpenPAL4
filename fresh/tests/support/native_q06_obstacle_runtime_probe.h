#pragma once
#include "pal4/native_gob_obstacle_runtime.h"
#include "pal4/native_gob_clump_asset.h"
#include "pal4/native_gob_animation_binding.h"
#include "pal4/native_scene_gob_assets.h"
#include "pal4/native_scene_movement_asset.h"
#include "pal4/gob_obstacle.h"
#include <algorithm>
#include <bit>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace pal4::test {
inline void VerifyNativeQ06ObstacleRuntime(const std::filesystem::path& root,
    IGameResourceManagerRuntime& transforms,NativeBaseObjectMemory& memory,
    IRwDictSchemaRuntime& schema,INativeGobEffectService& effects){
    const auto check=[](bool ok,const char* text){if(!ok)throw std::runtime_error(text);};
    NativeSceneGobCatalog catalog;check(catalog.Load(root,"Q06","Q06Q"),"Obstacle real Q06 catalog");
    const auto found=std::find_if(catalog.Records().begin(),catalog.Records().end(),
        [](const auto& r){return r.name=="burning01";});
    check(found!=catalog.Records().end() && found->type==10u,"Obstacle actual burning01 type10");
    const auto allocations=memory.AllocationAddresses();
    {
        NativeGobClumpAsset model;check(model.Load(root,found->resource_dir,found->model),"Obstacle actual JDumy clump");
        NativeGobObjectImage object(memory,schema);const auto owner=object.Construct(10u);
        object.BindRecord(*found,reinterpret_cast<std::uint32_t>(model.Clump()),transforms);
        check(memory.ReadDword(owner)==GobObstacle::kVtable && memory.ReadDword(owner+0x2d4u)==owner &&
            memory.ReadDword(owner+0x170u)==0u && memory.ReadDword(owner+0x2f8u)==2u &&
            memory.ReadDword(owner+0x2d8u)==0u && memory.ReadDword(owner+0x2dcu)==0u &&
            memory.ReadDword(owner+0x304u)==std::bit_cast<std::uint32_t>(10.f),
            "Obstacle real constructor/helper owner and force/status config");
        NativeGobAnimationBinding animation(object,model,memory);
        check(animation.Load(found->model),"Obstacle inherited original animation loading");
        NativeSceneGobAssets targets;NativeCollisionEventRouter router;
        NativeGobMachineServices services;
        unsigned refreshes{},readds{};
        std::vector<std::string> script_calls;
        const std::string module="gamedata\\script\\Q06.csb["+found->script_entry+"]";
        services.script.initialize=[&]{script_calls.push_back("initialize");};
        services.script.cleanup=[&]{script_calls.push_back("cleanup");};
        services.script.has_module=[&](std::string_view key){check(key==module,"Obstacle original script cache key");script_calls.push_back("cache");return false;};
        services.script.load_module=[&](std::string_view key){check(key==module,"Obstacle original script load key");script_calls.push_back("load");};
        services.script.push_module=[&](std::string_view key,std::string_view entry){check(key==module && entry==found->script_entry,"Obstacle original script entry");script_calls.push_back("push");};
        services.refresh_minimap=[&]{++refreshes;};
        services.queue_fade_out=[&](std::uint32_t address){check(address==owner,"Obstacle research readd owner");++readds;};
        NativeGobMachineRuntime machine(object,*found,memory,services);
        std::array<std::uint32_t,2> control{}; // Actual no-current-actor branch.
        unsigned actor_reads{},forces{},floors{},statuses{};
        NativeGobObstacleActorServices actors;
        actors.actor_control=[&]{++actor_reads;return reinterpret_cast<std::uint32_t>(control.data());};
        actors.world_force=[&](const auto&){++forces;};
        actors.floor_sound=[&](float){++floors;};
        actors.apply_status=[&](float){throw std::runtime_error("actual burning01 must select status2");};
        actors.start_status=[&](std::uint32_t a,std::uint32_t b,float value){
            check(a==std::bit_cast<std::uint32_t>(3.f) && b==std::bit_cast<std::uint32_t>(3.f) && value==10.f,
                "Obstacle collision dispatches original status timing/hurt");++statuses;
        };
        actors.movement_status=[&](std::uint32_t){throw std::runtime_error("actual burning01 must select status2");};
        actors.script_path=[]{return std::string("gamedata\\script\\Q06.csb");};
        // The borrowed collision owner must also outlive runtime unwinding if
        // a check fails while its helper is attached.
        NativeSceneMovementAsset movement;check(movement.Load(root,"Q06","Q06Q"),"Obstacle actual Q06 movement resource");
        const auto prior=movement.SceneBorrowerCount();
        NativeGobObstacleRuntime obstacle(object,machine,memory,targets,router,actors,services,effects);
        check(object.ObstacleService()==&obstacle && !router.Count(),"Obstacle borrowed service binds before collision entry");
        (void)object.SetVisible(1u);obstacle.Advance(.017f);
        check(actor_reads==1u && !forces && !floors,"Obstacle actual no-current-actor frame does not invent forces");
        obstacle.AttachMovement(movement);
        check(router.Count()==1u && movement.SceneBorrowerCount()==prior+1u &&
            movement.IsObjectClumpRegistered(model.Clump()),"Obstacle helper acquires actual primary clump registration");
        bool duplicate{};try{obstacle.AttachMovement(movement);}catch(const std::logic_error&){duplicate=true;}
        check(duplicate && router.Count()==1u,"Obstacle duplicate attachment retains existing live ownership");
        const auto helper=obstacle.HelperIdentity();router.Enter(helper,0u);router.Leave(helper);
        check(script_calls==std::vector<std::string>{"initialize","cache","load","push","cleanup"},
            "Obstacle real retained func7001 dispatch has balanced package/load/push scope");
        check(statuses==1u && memory.ReadDword(movement.PrimaryAddress()+88u)==1u && !readds && !refreshes,
            "Obstacle helper Enter dispatches status and movement flag once; Leave is original no-op");
        obstacle.DetachMovement(movement);
        movement.MutableState().words[22u]=0u;
        obstacle.DetachMovement(movement); // Registered entry clear, then original manager-wide Leave.
        check(movement.MutableState().words[22u]==1u,"repeat removal retains original primary dirty flag");
        check(!router.Count() && movement.SceneBorrowerCount()==prior &&
            !movement.IsObjectClumpRegistered(model.Clump()),"Obstacle detaches callback identity with primary clump");
        bool stale{};try{router.Enter(helper,0u);}catch(const std::invalid_argument&){stale=true;}
        check(stale && statuses==1u,"Obstacle retired helper cannot dispatch into a freed scene owner");
    }
    check(memory.AllocationAddresses()==allocations,"Obstacle real resource/runtime/helper normal cleanup has zero allocation delta");
    std::cout<<"Native Q06 Obstacle runtime: real burning01 config/JDumy, inherited Machine, actual primary helper registration/status2/leave/detach/stale rejection and cleanup verified; actor status service observed, not live damage acceptance\n";
}
}
