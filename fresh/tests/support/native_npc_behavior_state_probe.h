#pragma once
#include "pal4/native_actor_behavior_state_runtime.h"
#include "pal4/native_actor_behavior_resources.h"
#include "pal4/native_actor_behavior_orientation.h"
#include "pal4/native_behavior_grid_elements.h"
#include "pal4/native_behavior_helper_configuration.h"
#include "pal4/native_behavior_movement.h"
#include "pal4/native_npc_behavior_session.h"
#include "pal4/native_behavior_random_movement.h"
#include "pal4/native_behavior_grid_association_runtime.h"
#include "pal4/native_arena_npc_scene_loader.h"
#include "pal4/native_actor_animation_state_binding.h"
#include "pal4/native_npc_info_reader.h"
#include "pal4/native_world_path_planner_owner.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_event_document_runtime.h"
#include "pal4/native_scene_movement_asset.h"
#include "pal4/native_player_transform_runtime.h"
#include "pal4/native_config_composite_collision_runtime.h"
#include "pal4/game_resource_manager.h"
#include "pal4/grid_node_object.h"
#include "pal4/game_arena_scene_record.h"
#include "pal4/spatial_grid_wrapper.h"
#include <array>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <set>
#include <iostream>
#include <stdexcept>

namespace pal4::test {
inline void VerifyNativeNpcOptionalScene(const std::filesystem::path& root,NativeBaseObjectMemory& memory,
    IRwDictSchemaRuntime& schema,IGameObjectEmbeddedAnimationRuntime& animation,
    NativePlayerTransformRuntime& transforms,NativeWorldPathPlannerOwner& paths,
    NativeSceneMovementAsset& movement,NativeArenaCollisionTree& collision,
    std::span<const GameArenaSceneRecord> scenes,std::uint32_t game,IBaseObjectMaterialRuntime& materials) {
    const auto check=[](bool value,const char* message) {if(!value)throw std::runtime_error(message);};
    const auto baseline=memory.OutstandingAllocations();
    NativeActorBehaviorOrientationCaches caches{};
    std::uint32_t identifier=0u;
    NativeArenaNpcSceneLoader empty(root,memory,schema,animation,transforms);
    std::set<std::pair<std::string,std::string>> visited;
    unsigned absent=0u;
    std::uint32_t manager_identity=0u,nil_identity=0u;
    std::size_t retained_allocations=0u;
    for(const auto& scene:scenes) {
        if(!visited.emplace(scene.world,scene.scene).second)continue;
        NativeNpcInfoReader source;
        if(source.Load(root,scene.world,scene.scene))continue;
        check(source.Status()==NativeNpcInfoReader::LoadStatus::MissingMember,
              "original catalog audit does not suppress unreadable NPC members");
        ++absent;
        empty.Load(scene.world,scene.scene);
        if(!manager_identity) {
            manager_identity=empty.Manager().Address();nil_identity=empty.Manager().TreeGlobals().nil_address;
        }
        check(empty.Manager().Address()==manager_identity && empty.Manager().TreeGlobals().nil_address==nil_identity,
              "optional scene reload preserves original manager and tree nil");
        check(!empty.Count() && !empty.Manager().InfoCount() && !empty.Manager().ObjectCount() &&
                  empty.Manager().GridAddress()!=0u,"original absent optional NPC member creates empty native manager/grid");
        empty.BindBehaviorRuntime(paths,movement,transforms,caches,identifier,[] {return 16384u;});
        check(empty.UpdateWorldFrame(1.f/60.f,0u,collision,transforms)==0u &&
                  !empty.BehaviorRuntime()->LiveHelpers(),"empty optional scene dispatches no actors or behavior helpers");
        empty.CleanupScene(game,materials);
        if(absent==1u)retained_allocations=empty.Manager().OutstandingAllocations();
        check(!empty.Manager().GridAddress() && !empty.BehaviorRuntime() &&
                  empty.Manager().OutstandingAllocations()==retained_allocations &&
                  memory.OutstandingAllocations()==baseline,"empty optional scene cleanup/reload has no native allocation growth");
    }
    check(absent!=0u,"original scene catalog contains optional absent NPC members");
    empty.Clear();
    check(!empty.Manager().Address() && !empty.Manager().OutstandingAllocations(),"optional scene final teardown frees retained manager");
    bool rejected=false;
    try {empty.Load("Q01/other","N01");} catch(const std::runtime_error&) {rejected=true;}
    check(rejected,"invalid names remain errors at scene-loader boundary");
    NativeArenaNpcSceneLoader unavailable(root/"__missing_package__",memory,schema,animation,transforms);
    rejected=false;
    try {unavailable.Load("Q01","N01");} catch(const std::runtime_error&) {rejected=true;}
    check(rejected,"missing package is not accepted as an optional missing NPC member");
    std::cout<<"PASS: original catalog optional NPC scenes="<<absent
        <<" empty grid/frame, cleanup/reload and strict error boundaries\n";
}
inline void VerifyNativeNpcAutomaticBehavior(NativeArenaNpcSceneLoader& npcs,
    NativeArenaCollisionTree& collision,NativePlayerTransformRuntime& transforms,NativeBaseObjectMemory& memory) {
    const auto check=[](bool value,const char* message) {if(!value)throw std::runtime_error(message);};
    auto* session=npcs.BehaviorRuntime();
    check(session!=nullptr,"actual scene owns native NPC behavior session");
    // This isolated acceptance test must not rely on the wall-clock game seed
    // choosing class2 in a short observation window. All scheduler/helper draws
    // still use the actual common host CRT stream and original branch logic.
    std::srand(1u);
    const auto before=session->Stats();
    for(unsigned frame=0;frame<1800;++frame)
        (void)npcs.UpdateWorldFrame(1.f/60.f,0u,collision,transforms);
    const auto& after=session->Stats();
    std::cout<<"Q02 automatic NPC session1800 frames: accepted="<<after.accepted-before.accepted
        <<" released="<<after.released-before.released<<" grid="<<after.grid_frames-before.grid_frames
        <<" targets="<<after.target_updates-before.target_updates<<" live="<<session->LiveHelpers()<<'\n';
    check(after.accepted>before.accepted && after.grid_frames>before.grid_frames &&
              after.target_updates>before.target_updates,
          "actual manager/scheduler automatically accepts, moves and applies native NPC helpers");
    for(unsigned kind=0;kind<5;++kind)
        std::cout<<"Q02 automatic class"<<kind<<" accepted="<<after.accepted_by_kind[kind]
            <<" released="<<after.released_by_kind[kind]<<'\n';
    check(after.accepted_by_kind[0] && after.accepted_by_kind[2] && after.accepted_by_kind[3] &&
              after.released_by_kind[2],"original Q02 naturally schedules all present types and completes animation tasks");
    const auto npc=npcs.Manager().FindObject("4001");
    const auto info=npcs.Manager().FindInfo("4001");
    check(npc && info,"failure admission probe uses actual original4001 class0");
    npcs.CleanupEventsAndScripts(npc,0u);
    check(session->LiveHelpers(npc)==0u,"pause cancels actual scheduled helper before failure probe");
    auto& documents=npcs.BehaviorDocuments(npc);
    const auto document=memory.ReadDword(npc+0x668u);
    const auto saved=documents.CloneDocument(document);
    documents.WriteIntAttribute(document,"AAB_NPC_memory_last_behavior_id",0);
    // Shared SetTime rejects negative time before touching the SDK. Admission
    // has already registered its helper and selected C02 when it reaches this.
    documents.WriteFloatAttribute(document,"AAB_NPC_memory_actiontime",-1.f);
    const auto rollbacks=session->Stats().rolled_back;
    npcs.ProcessEventsAndClear(npc,info,0u);
    bool failed=false;
    for(unsigned frame=0;frame<1800 && !failed;++frame) {
        try {(void)npcs.UpdateWorldFrame(1.f/60.f,0u,collision,transforms);}
        catch(const std::runtime_error& error) {
            if(std::string_view(error.what())!="actor behavior animation time rejected")throw;
            failed=true;
        }
    }
    check(failed && session->Stats().rolled_back==rollbacks+1u && !session->LiveHelpers(npc),
          "actual scheduler admission failure rolls back registered helper and task factory frees once");
    npcs.CleanupEventsAndScripts(npc,0u);
    documents.AssignDocument(document,saved);documents.DestroyDocument(saved);(void)memory.Free(saved);
    npcs.ProcessEventsAndClear(npc,info,0u);
    for(unsigned frame=0;frame<1800 && !session->LiveHelpers(npc);++frame)
        (void)npcs.UpdateWorldFrame(1.f/60.f,0u,collision,transforms);
    check(session->LiveHelpers(npc)==1u,"original NPC can be accepted again after failed admission and cancellation");
    std::cout<<"PASS: Q02 actual scheduler admission rollback after grid registration, cancel and successful retry\n";
    std::cout<<"PASS: Q02 original automatic task/helper/grid and natural NPC animation frames\n";
}
inline void VerifyNativeNpcBehaviorState(NativeArenaNpcSceneLoader& npcs,
    NativeWorldPathPlannerOwner& paths,NativeBaseObjectMemory& memory,
    NativeSceneMovementAsset& movement,NativePlayerTransformRuntime& transforms,
    const std::filesystem::path& game_root) {
    const auto check=[](bool value,const char* message) { if(!value)throw std::runtime_error(message); };
    NativeNpcInfoReader reader;
    check(reader.Load(game_root,"Q02","Q02"),"NPC state probe loads original Q02 behavior");
    NativeBehaviorGridAssociationRuntime associations(memory);
    std::uint32_t identifier=0u;
    const auto random=[] { return 16384u; };
    NativeBehaviorHelperStorage storage(memory,identifier,
        [&](std::uint32_t association,bool free_self) { associations.DestroyAssociation(association,free_self); },
        random);
    NativeBehaviorHelperConfiguration configuration(memory,associations,random);
    NativeBehaviorRandomMovement random_movement(associations,random);
    NativeBehaviorMovement path_movement(associations,random);
    auto grid_elements=std::make_shared<NativeBehaviorGridElements>(storage,
        [](std::uint32_t,std::uint32_t,std::uint32_t) {
            throw std::logic_error("state-only probe must not execute unbound helper physics");
        });
    npcs.Manager().BindGridElements(grid_elements);
    NativeActorBehaviorResources resources(npcs,paths);
    NativeActorBehaviorOrientationCaches orientation_caches{};
    NativeActorBehaviorOrientation orientation(npcs,memory,movement,transforms,orientation_caches);
    std::uint32_t orientation_updates{},floor_hits{},physics_frames{};
    std::array<bool,5> tested{};
    for(const auto& record:reader.Records()) {
        const auto npc=npcs.Manager().FindObject(record.name);
        check(npc!=0u,"original NPC exists in actual scene");
        check(std::isfinite(std::bit_cast<float>(memory.ReadDword(npc+0xdcu))) &&
                  std::bit_cast<float>(memory.ReadDword(npc+0xdcu))>0.f,
              "scene NPC resource has original nonzero initialized bounding sphere radius");
        for(const auto& group:record.behavior.Root().children) {
            for(const auto& atomic:group.children) {
                const auto* attribute=atomic.FindAttribute("NPCINFO_Behavior_ObjectType");
                if(!attribute)continue;
                const auto kind=static_cast<std::int32_t>(attribute->ToInt(0xffffffffu));
                if(kind<0 || kind>4 || tested[kind])continue;
                bool parameters=kind==2;
                for(const auto& child:atomic.children) parameters|=child.name=="AAB_NPC_parameters";
                if(!parameters)continue;
                auto& documents=npcs.BehaviorDocuments(npc);
                const auto memory_document=memory.ReadDword(npc+0x668u);
                const auto saved_document=documents.CloneDocument(memory_document);
                const auto original_config=memory.ReadDword(npc+0x6d8u);
                auto& animation=npcs.AnimationBinding(npc);
                const std::string original_action(animation.CurrentRecordName());
                const auto original_time=animation.Time();
                const auto original_restart=memory.ReadDword(npc+0x170u+17u*4u);
                const auto original_completed=memory.ReadDword(npc+0x170u+18u*4u);
                const ResourceVec3 original_rotation{
                    std::bit_cast<float>(memory.ReadDword(npc+0xa0u)),
                    std::bit_cast<float>(memory.ReadDword(npc+0xa4u)),
                    std::bit_cast<float>(memory.ReadDword(npc+0xa8u))};
                const ResourceVec3 original_position{
                    std::bit_cast<float>(memory.ReadDword(npc+0xacu)),
                    std::bit_cast<float>(memory.ReadDword(npc+0xb0u)),
                    std::bit_cast<float>(memory.ReadDword(npc+0xb4u))};
                memory.WriteDword(npc+0x6d8u,static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&group)));
                NativeActorBehaviorStateRuntime state(memory,documents,npcs.Manager(),configuration,resources);
                const auto node_count=[&] {
                    auto& wrapper=*reinterpret_cast<SpatialGridWrapperState*>(npcs.Manager().GridWrapperAddress());
                    return SpatialGridWrapper::CountNodes(wrapper,associations);
                };
                check(node_count()==0u,"state probe starts with no spatial helper nodes");
                const auto saved_kind=kind;
                documents.WriteIntAttribute(memory_document,"AAB_NPC_memory_last_behavior_id",saved_kind);
                documents.WriteFloatAttribute(memory_document,"AAB_NPC_memory_speed",1.25f);
                documents.WriteFloatAttribute(memory_document,"AAB_NPC_memory_actiontime",0.125f);
                documents.WriteIntAttribute(memory_document,"AABPedestrian_NPC_memory_pathDirection",-1);
                documents.WriteIntAttribute(memory_document,"AABPedestrian_NPC_memory_wander",1);
                documents.WriteFloatAttribute(memory_document,"AABWander_NPC_memory_start_x",3.f);
                documents.WriteFloatAttribute(memory_document,"AABWander_NPC_memory_start_y",4.f);
                documents.WriteFloatAttribute(memory_document,"AABWander_NPC_memory_start_z",5.f);
                const auto helper_baseline=memory.OutstandingAllocations();
                float expected_speed=1.25f;
                for(unsigned cycle=0;cycle<2;++cycle) {
                    const auto helper=memory.Allocate(NativeBehaviorHelperStorage::ImageSize(kind));
                    storage.Construct(kind,helper);
                    (void)state.Restore(kind,npc,helper);
                    check(memory.ReadByte(helper+0xb8u)==1u && node_count()==1u,
                          "original restore activates helper in actual manager grid");
                    const auto association=memory.ReadDword(helper+0xa8u);
                    check(reinterpret_cast<const GridNodeObjectState*>(association)->node.payload==helper,
                          "restored native association references actual helper");
                    const auto rows=resources.Transform(npc,memory.ReadDword(npc+0x14u));
                    const auto position=NativeBehaviorHelperConfiguration::ReadVector(
                        *reinterpret_cast<BehaviorPathEndpointState*>(helper),0x18u);
                    check(position==rows[3],"original restore copies real actor LTM position");
                    check(std::fabs(animation.Time()-0.125f)<0.01f,"original restore seeks real actor clip to saved time");
                    const auto selected=animation.CurrentRecordName();
                    check(kind==2 ? (selected=="C13" || selected=="C01" || selected=="Z01") :
                        (kind==1 ? (selected=="C03" || selected=="Z02") : (selected=="C02" || selected=="Z02")),
                        "original restore selects the expected live actor action or original fallback");
                    if(kind==0 || kind==1 || kind==3)
                        check(std::bit_cast<float>(memory.ReadDword(helper+100u))==expected_speed,
                              "original restore consumes actual native memory speed");
                    if(kind==3) check(std::bit_cast<float>(memory.ReadDword(helper+0xd4u))==3.f &&
                        std::bit_cast<float>(memory.ReadDword(helper+0xd8u))==4.f &&
                        std::bit_cast<float>(memory.ReadDword(helper+0xdcu))==5.f,
                        "original random restore recovers saved start point");
                    if(kind==2) {
                        const auto completed=memory.ReadDword(npc+0x170u+18u*4u);
                        memory.WriteDword(npc+0x170u+18u*4u,0u);
                        check(!state.IsComplete(npc),"class2 polls actual incomplete animation flag");
                        memory.WriteDword(npc+0x170u+18u*4u,1u);
                        check(state.IsComplete(npc),"class2 polls actual complete animation flag");
                        memory.WriteDword(npc+0x170u+18u*4u,completed);
                    }
                    if(kind!=2) {
                        // Controlled movement output fixture. This verifies the
                        // actual writeback stage, not autonomous steering.
                        check(!orientation.Update(kind,npc,helper),
                              "helper B9 clear gates native floor/orientation writeback");
                        auto& cache=orientation_caches[kind<2?kind:kind==3?2:3];
                        check(cache.guard==3u,"original per-variant extent cache initializes before activity gate");
                        const float heading=cycle==0?1.f:-1.f;
                        auto& image=*reinterpret_cast<BehaviorPathEndpointState*>(helper);
                        NativeBehaviorHelperConfiguration::SetVector(image,0x14u,{heading,0.f,0.f});
                        BehaviorVector3 candidate{rows[3][0]+0.05f,
                            rows[3][1]+cache.first_limit*0.25f,rows[3][2]+0.05f};
                        NativeBehaviorHelperConfiguration::SetVector(image,0x1cu,candidate);
                        memory.WriteByte(helper+0xb9u,1u);
                        auto expected=candidate;
                        NativeConfigCompositeCollisionRuntime collision;
                        const auto floor_hit=ConfigCompositeCollision::CheckFloorClumpIntersection(
                            movement.State(),candidate,cache.first_limit,cache.second_limit,
                            &expected,nullptr,nullptr,collision)!=0u;
                        floor_hits+=floor_hit;
                        std::cout<<"Q02 NPC orientation npc="<<record.name<<" cycle="<<cycle
                            <<" candidate="<<candidate[0]<<','<<candidate[1]<<','<<candidate[2]
                            <<" extent="<<cache.first_limit<<','<<cache.second_limit<<" floor="<<floor_hit<<'\n';
                        check(orientation.Update(kind,npc,helper),"active helper applies native orientation target");
                        const auto corrected=NativeBehaviorHelperConfiguration::ReadVector(image,0x18u);
                        const auto applied=resources.Transform(npc,memory.ReadDword(npc+0x14u));
                        for(unsigned axis=0;axis<3;++axis) {
                            check(std::fabs(corrected[axis]-expected[axis])<0.001f,
                                  "helper writeback matches actual original floor intersection");
                            check(std::fabs(applied[3][axis]-corrected[axis])<0.001f,
                                  "shared orientation updates actual actor LTM position");
                        }
                        const auto yaw=std::bit_cast<float>(memory.ReadDword(npc+0xa4u));
                        check(std::fabs(yaw-(cycle==0?90.f:270.f))<0.01f,
                              "native orientation applies both original acos/sign half-planes");
                        ++orientation_updates;
                        const auto start=corrected;
                        const auto maximum=std::bit_cast<float>(memory.ReadDword(helper+108u));
                        for(unsigned frame=0;frame<60;++frame) {
                            // Original steering, real integration/node notification.
                            // Saved-memory start/initial orientation are controlled.
                            if(kind==3) {
                                const auto obstacles=npcs.Manager().GridObstacleCandidates();
                                check(random_movement.Update(helper,obstacles,1.f/60.f).active,
                                      "original class3 generates steering through native random/collision update");
                            } else {
                                const auto obstacles=npcs.Manager().GridObstacleCandidates();
                                check(path_movement.UpdatePath(helper,obstacles,1.f/60.f).active,
                                      "original endpoint generates steering through native path/collision update");
                            }
                            check(orientation.Update(kind,npc,helper),"physics output reaches real NPC orientation callback");
                            const auto actual=resources.Transform(npc,memory.ReadDword(npc+0x14u));
                            const auto final=NativeBehaviorHelperConfiguration::ReadVector(image,0x18u);
                            for(unsigned axis=0;axis<3;++axis)
                                check(std::fabs(actual[3][axis]-final[axis])<0.001f,
                                      "continuous shared physics position reaches actual actor LTM");
                            check(std::bit_cast<float>(memory.ReadDword(helper+100u))<=maximum+0.001f,
                                  "actual NPC integration obeys original configured speed cap");
                            ++physics_frames;
                        }
                        const auto finish=NativeBehaviorHelperConfiguration::ReadVector(image,0x18u);
                        check(std::fabs(finish[0]-start[0])+std::fabs(finish[2]-start[2])>0.001f,
                              "original NPC moves continuously from integrated velocity");
                        expected_speed=std::bit_cast<float>(memory.ReadDword(helper+100u));
                    }
                    (void)state.Save(kind,npc,helper); // Grid removal deletes the helper.
                    check(node_count()==0u,"original save deletes helper and unlinks actual grid node");
                    check(animation.CurrentRecordName()=="C01" || animation.CurrentRecordName()=="Z01",
                          "original save returns actual actor to the expected idle action");
                    const auto type=documents.FindNumericAttribute(memory_document,"AAB_NPC_memory_last_behavior_id");
                    check(type && documents.AttributeToInt(type,-1)==kind,"native save writes original behavior type");
                    if(kind==0 || kind==1 || kind==3) {
                        const auto speed=documents.FindNumericAttribute(memory_document,"AAB_NPC_memory_speed");
                        check(speed && documents.AttributeToFloat(speed,-1.f)==expected_speed,
                              "native save persists actual integrated speed for next restore");
                    }
                    const auto time=documents.FindNumericAttribute(memory_document,"AAB_NPC_memory_actiontime");
                    check(time && std::fabs(documents.AttributeToFloat(time,-1.f)-0.125f)<0.01f,
                          "native save reads real actor animation time before idle selection");
                    check(memory.OutstandingAllocations()==helper_baseline,
                          "native restore/save helper cycle has no allocation growth");
                }
                memory.WriteDword(npc+0x6d8u,original_config);
                documents.AssignDocument(memory_document,saved_document);
                documents.DestroyDocument(saved_document);
                (void)memory.Free(saved_document);
                auto& resource=*reinterpret_cast<GameResourceManager*>(npc);
                resource.UpdateClumpRotation(original_rotation,false,transforms);
                (void)resource.UpdateClumpTransform(original_position,false,transforms);
                check(animation.Select(original_action) && animation.SetTime(original_time),
                      "NPC state probe restores original live action and time");
                memory.WriteDword(npc+0x170u+17u*4u,original_restart);
                memory.WriteDword(npc+0x170u+18u*4u,original_completed);
                tested[kind]=true;
                std::cout<<"PASS: original Q02 NPC "<<record.name<<" class"<<kind
                    <<" actual restore/save/restore with live LTM, animation, XML and deleting grid\n";
            }
        }
    }
    npcs.Manager().BindGridElements({});
    check(tested[0] && tested[2] && tested[3],"actual original Q02 tests all three present behavior classes");
    check(orientation_updates>=4u && floor_hits!=0u && physics_frames>=240u,
          "original Q02 native orientation writes class0/3 outputs and intersects actual floor");
    std::cout<<"PASS: Q02 native orientation output updates="<<orientation_updates
        <<" real floor hits="<<floor_hits<<"; automatic task binding still pending\n";
    std::cout<<"PASS: Q02 actual helper/actor physics frames="<<physics_frames
        <<" with original speed caps; class0 and class3 generated steering\n";
    check(path_movement.Stats().frames>=120u && path_movement.Stats().sphere_queries!=0u &&
              path_movement.Stats().path_corrections!=0u,
          "original Q02 class0 exercises path steering, spatial queries and real path correction");
    std::cout<<"PASS: Q02 class0 generated-steering frames="<<path_movement.Stats().frames
        <<" actual neighbor queries="<<path_movement.Stats().sphere_queries
        <<" path corrections="<<path_movement.Stats().path_corrections
        <<" drift updates="<<path_movement.Stats().drift_updates
        <<" RNG draws="<<path_movement.Stats().random_draws<<'\n';
    check(random_movement.Stats().frames>=120u && random_movement.Stats().sphere_queries!=0u,
          "original Q02 class3 exercises generated steering and real spatial queries");
    std::cout<<"PASS: Q02 class3 generated-steering frames="<<random_movement.Stats().frames
        <<" actual neighbor queries="<<random_movement.Stats().sphere_queries
        <<" RNG draws="<<random_movement.Stats().random_draws<<'\n';
}
}
