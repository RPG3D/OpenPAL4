#pragma once
#include "pal4/native_scene_movement_asset.h"
#include "pal4/native_arena_collision_tree.h"
#include "pal4/native_game_arena_manager_owner.h"
#include "pal4/native_collision_actor_path_runtime.h"
#include "pal4/native_collision_actor_movement_runtime.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/collision_actor.h"
#include <bit>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace pal4::test {
// live251 MN001's actual first walk. Uses the original M07 floor and production
// path/follow adapters; no live game memory or script completion is changed.
inline void VerifyNativeM07NpcPath(const std::filesystem::path& root) {
    const auto require=[](bool ok,const char* message) {
        if(!ok) throw std::runtime_error(message);
    };
    NativeSceneMovementAsset scene;
    require(scene.Load(root,"M07","1") && scene.Floor(),"load original M07 NPC floor");
    NativeGameArenaManagerOwner arena;
    const auto address=arena.Construct();
    NativeArenaCollisionTree tree;
    require(tree.Build(address,scene.Floor()) && tree.FaceCount()>0,"build M07 NPC floor faces");
    NativeBaseObjectMemory memory;
    CollisionActorState actor{};
    (void)CollisionActor::Construct(actor,0u,memory.Path());
    struct Retire {CollisionActorState& actor;NativeBaseObjectMemory& memory;
        ~Retire(){(void)CollisionActor::Destroy(actor,memory.Path());}} retire{actor,memory};
    const auto actor_address=reinterpret_cast<std::uint32_t>(&actor);
    const D3dTriangleVector3 start{-386.0024108886719f,.8971719741821289f,-935.677734375f};
    auto target=D3dTriangleVector3{-403.83209228515625f,.8971719741821289f,-791.2979125976562f};
    const auto start_face=tree.SelectFace(start);
    const auto target_face=tree.SnapPointToFace(target);
    require(start_face && target_face,"M07 walk endpoints have floor faces");
    (void)CollisionActor::Place(actor,actor_address,tree.TreeAddress(),0x41873333u,
        start,start_face,0x42248358u,tree);
    (void)CollisionActor::SetSpeedBits(actor,std::bit_cast<std::uint32_t>(45.0f));
    (void)CollisionActor::SetCallback108(actor,0x444390u);
    NativeCollisionActorPathRuntime paths(tree,memory);
    require(CollisionActor::RequestPath(actor,actor_address,start,target,target_face,0u,paths),
        "M07 NPC path is planned before following");
    require(actor.words[22u]==7u,"M07 first NPC path has seven original waypoints");
    NativeCollisionActorMovementRuntime movement(tree,memory);
    unsigned frames=0;
    // World0 isolates floor reachability: the existing clearance adapter treats
    // a missing world as clear. It still runs real floor edge crossing/clamping.
    while(CollisionActor::PathEnabled(actor) && frames<1200u) {
        (void)CollisionActor::UpdateMovement(actor,actor_address,0u,1.0f/60.0f,movement);
        ++frames;
    }
    const auto x=std::bit_cast<float>(actor.words[3u]);
    const auto y=std::bit_cast<float>(actor.words[4u]);
    const auto z=std::bit_cast<float>(actor.words[5u]);
    std::cout<<"M07 original seven-point NPC path: frames="<<frames<<" position="
        <<x<<','<<y<<','<<z<<" status="<<actor.words[36u]<<'\n';
    require(!CollisionActor::PathEnabled(actor) && frames<1200u &&
        actor.words[3u]==std::bit_cast<std::uint32_t>(target.x) &&
        actor.words[4u]==std::bit_cast<std::uint32_t>(target.y) &&
        actor.words[5u]==std::bit_cast<std::uint32_t>(target.z),
        "original M07 NPC path must reach the real endpoint, not stop at an extended edge");
}
} // namespace pal4::test
