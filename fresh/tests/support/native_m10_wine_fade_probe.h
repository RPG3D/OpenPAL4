#pragma once
#include "pal4/native_game_world_fade_queues.h"
#include "pal4/native_game_arena_manager_owner.h"
#include "pal4/native_arena_gob_scene_entry.h"
#include "pal4/native_gob_object_image.h"
#include "pal4/native_gob_obstacle_runtime.h"
#include "pal4/native_gob_animation_binding.h"
#include "pal4/native_scene_gob_assets.h"
#include "pal4/native_collision_event_router.h"
#include "pal4/native_gob_clump_asset.h"
#include "pal4/native_scene_movement_asset.h"
#include <rwcore.h>
#include <rpworld.h>
#include <algorithm>
#include <bit>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace pal4::test {
inline void VerifyNativeM10WineFade(const std::filesystem::path& root,
    IGameResourceManagerRuntime& transforms,NativeBaseObjectMemory& memory,IRwDictSchemaRuntime& schema,INativeGobEffectService& effects){
    const auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    const auto allocations=memory.AllocationAddresses();
    {
        NativeSceneGobCatalog catalog;check(catalog.Load(root,"M10","1"),"wine fade original M10 catalog");
        const auto found=std::find_if(catalog.Records().begin(),catalog.Records().end(),[](const auto& r){return r.name=="wine001";});
        check(found!=catalog.Records().end() && found->type==10u && found->model=="SWG" && found->script_entry=="func7001",
            "wine fade actual original wine001 Obstacle/SWG/func7001");
        NativeSceneMovementAsset movement;check(movement.Load(root,"M10","1"),"wine fade original M10 movement");
        NativeGobClumpAsset model;check(model.Load(root,found->resource_dir,found->model),"wine fade original model");
        NativeGobObjectImage image(memory,schema);const auto object=image.Construct(found->type);
        image.BindRecord(*found,reinterpret_cast<std::uint32_t>(model.Clump()),transforms);
        NativeGobAnimationBinding animation(image,model,memory);
        check(animation.Load(found->model),"wine fade actual inherited animation");
        NativeSceneGobAssets targets;NativeCollisionEventRouter router;
        NativeGobMachineServices machine_services;
        machine_services.refresh_minimap=[]{};
        machine_services.queue_fade_out=[](auto){throw std::runtime_error("wine fade fixture unexpectedly researched object");};
        NativeGobMachineRuntime machine(image,*found,memory,machine_services);
        NativeGobObstacleActorServices actors;
        actors.actor_control=[]{throw std::runtime_error("wine fade fixture unexpectedly advanced actor");return 0u;};
        actors.world_force=[](const auto&){throw std::runtime_error("wine fade fixture unexpected actor force");};
        actors.floor_sound=[](float){throw std::runtime_error("wine fade fixture unexpected floor sound");};
        actors.apply_status=[](float){throw std::runtime_error("wine fade fixture unexpected actor status");};
        actors.start_status=[](auto,auto,float){throw std::runtime_error("wine fade fixture unexpected timed status");};
        actors.movement_status=[](auto){throw std::runtime_error("wine fade fixture unexpected movement status");};
        actors.script_path=[]{return std::string("gamedata\\script\\M10.csb");};
        NativeGobObstacleRuntime obstacle(image,machine,memory,targets,router,actors,machine_services,effects);
        NativeGameArenaManagerOwner arena;const auto owner=arena.Construct();
        check(owner!=0u,"wine fade isolated arena");
        RwBBox bounds{{100000.f,100000.f,100000.f},{-100000.f,-100000.f,-100000.f}};
        auto* world=RpWorldCreate(&bounds);check(world!=nullptr,"wine fade isolated SDK world");
        const auto word=[](std::uint32_t a)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(a);};
        // Borrowing is scoped: the SDK world outlives all entry callbacks, while
        // arena destruction must see no borrowed world or scene-ready fields.
        struct Scope {
            NativeGameArenaManagerOwner& arena;NativeGobObjectImage& image;
            NativeSceneMovementAsset& movement;RpWorld* world;bool entered{};
            ~Scope(){
                if(entered && (RpClumpGetWorld(reinterpret_cast<RpClump*>(*reinterpret_cast<std::uint32_t*>(image.Address()+20u))) ||
                    movement.IsObjectClumpRegistered(reinterpret_cast<RpClump*>(*reinterpret_cast<std::uint32_t*>(image.Address()+20u)))))
                    NativeArenaGobSceneEntry::Leave(image,arena.Address(),movement,[]{});
                arena.ClearActiveSceneRecords();
                *reinterpret_cast<std::uint32_t*>(arena.Address()+16u)=0u;
                *reinterpret_cast<std::uint32_t*>(arena.Address()+48u)=0u;
                RpWorldDestroy(world);
            }
        } scope{arena,image,movement,world};
        word(owner+16u)=reinterpret_cast<std::uint32_t>(world);word(owner+48u)=1u;
        // These checks retain the original resource flags, including camera
        // exclusion, rather than modifying the fixture to avoid dependencies.
        check(word(object+0x184u)==1u && word(object+0x170u)!=0u && word(object+0x194u)==0u,
            "wine001 original visible/animated/no-camera flags");
        NativeArenaGobSceneEntry::Enter(object,owner,movement,nullptr,&image);scope.entered=true;
        check(RpClumpGetWorld(model.Clump())==world && movement.IsObjectClumpRegistered(model.Clump()) && router.Count()==1u,
            "wine001 actual scene entry attaches world and collision once");
        check(arena.AddActiveSceneObject(object,1.f)==1u && arena.AddActiveSceneObject(object,1.f)==0u,
            "wine001 queue append and original duplicate-name rejection");
        auto record=word(owner+620u);
        check(word(owner+624u)==record+12u && word(record)==object &&
            word(record+4u)==std::bit_cast<unsigned>(1.f) && word(record+8u)==0u,"wine001 original twelve-byte fade record");
        unsigned refreshes{};NativeGameWorldFadeServices services;
        services.resolve_gob=[&](auto address){return address==object?&image:nullptr;};
        services.refresh_minimap=[&]{++refreshes;};
        const auto colors=[&]{
            std::vector<unsigned> result;
            RpClumpForAllAtomics(model.Clump(),[](RpAtomic* atomic,void* context)->RpAtomic*{
                auto& result=*static_cast<std::vector<unsigned>*>(context);
                auto* geometry=RpAtomicGetGeometry(atomic);
                for(int i=0;i<RpGeometryGetNumMaterials(geometry);++i){const auto* c=RpMaterialGetColor(RpGeometryGetMaterial(geometry,i));
                    result.push_back(unsigned(c->red)|(unsigned(c->green)<<8u)|(unsigned(c->blue)<<16u)|(unsigned(c->alpha)<<24u));}
                return atomic;
            },&result);return result;
        };
        auto result=NativeGameWorldFadeQueues::Advance(arena,movement,nullptr,.5f,services);
        const auto half=colors();check(!half.empty() && std::all_of(half.begin(),half.end(),[](auto c){return c==0x80808080u;}),
            "wine001 actual SDK materials fade halfway");
        check(result.before[0]==1u && result.after[0]==1u && !result.removed_gobs &&
            RpClumpGetWorld(model.Clump())==world && movement.IsObjectClumpRegistered(model.Clump()),"wine001 halfway retains attachments");
        result=NativeGameWorldFadeQueues::Advance(arena,movement,nullptr,.5f,services);
        const auto zero=colors();check(result.after[0]==1u && !result.removed_gobs &&
            std::all_of(zero.begin(),zero.end(),[](auto c){return c==0u;}) && word(object+0x184u)==1u,
            "wine001 exact duration transparent but still queued and visible");
        result=NativeGameWorldFadeQueues::Advance(arena,movement,nullptr,.017f,services);
        check(result.removed_gobs==1u && !result.after[0] && !word(object+0x184u) &&
            !RpClumpGetWorld(model.Clump()) && !movement.IsObjectClumpRegistered(model.Clump()) && !router.Count(),
            "wine001 beyond duration removes actual world/movement/visibility");
        result=NativeGameWorldFadeQueues::Advance(arena,movement,nullptr,1.f,services);
        check(!result.removed_gobs && !result.material_updates,"wine001 empty advance has no repeated effects");
    }
    check(memory.AllocationAddresses()==allocations,"wine fade native image teardown balances creating memory");
    std::cout<<"Native M10 wine001 fade: actual original type10 Obstacle/Machine/clump/movement/helper, deferred duplicate rejection, material half/exact/strict completion, real detach and cleanup pass\n";
}
}
