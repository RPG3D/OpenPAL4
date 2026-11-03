#include "native_monster_path_replay.h"
#include "pal4/native_world_asset.h"
#include "pal4/native_scene_movement_asset.h"
#include "pal4/native_arena_collision_tree.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_collision_actor_path_runtime.h"
#include "pal4/native_collision_actor_movement_runtime.h"
#include <rwcore.h>
#include <rpworld.h>
#include <array>
#include <bit>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
namespace pal4::test {
namespace {
void Require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
void Vector(const D3dTriangleVector3& p){std::cout<<'['<<p.x<<','<<p.y<<','<<p.z<<']';}
D3dTriangleVector3 Position(const CollisionActorState& s){return {std::bit_cast<float>(s.words[3]),std::bit_cast<float>(s.words[4]),std::bit_cast<float>(s.words[5])};}
}
void ReplayNativeMonsterPaths(const std::filesystem::path& root,const std::filesystem::path& fixture){
    std::ifstream input(fixture);Require(bool(input),"open actual Monster path fixture");
    NativeWorldAsset world;if(!world.LoadArenaSection(root,"M02","1"))throw std::runtime_error("M02/1 world: "+world.LastError());
    NativeSceneMovementAsset movement;if(!movement.Load(root,"M02","1") || !movement.Floor())throw std::runtime_error("M02/1 floor: "+movement.LastError());
    std::array<std::uint32_t,128> arena{};arena[4]=reinterpret_cast<std::uint32_t>(world.World());arena[12]=1u;
    NativeArenaCollisionTree tree;Require(tree.Build(reinterpret_cast<std::uint32_t>(arena.data()),movement.Floor()),"build actual M02/1 collision faces");
    std::string label;unsigned cases{};std::cout<<std::setprecision(9);
    while(input>>label){
        Require(++cases<=16u,"bounded Monster path fixture count");
        std::array<std::uint32_t,6> bits{};for(auto& word:bits)Require(bool(input>>word),"complete source/target float bits");
        D3dTriangleVector3 start{std::bit_cast<float>(bits[0]),std::bit_cast<float>(bits[1]),std::bit_cast<float>(bits[2])};
        D3dTriangleVector3 target{std::bit_cast<float>(bits[3]),std::bit_cast<float>(bits[4]),std::bit_cast<float>(bits[5])};
        std::uint32_t height_bits{},radius_bits{},use_world{};Require(bool(input>>height_bits>>radius_bits>>use_world) && use_world<=1u,"actual height/radius and world-context selector");
        const auto supplied_target=target;const auto target_face=tree.SnapPointToFace(target);Require(target_face!=0u,"actual target projects to floor face");
        NativeBaseObjectMemory memory;CollisionActorState actor{};(void)CollisionActor::Construct(actor,0u,memory.Path());actor.words[1]=tree.TreeAddress();
        NativeCollisionActorPathRuntime path(tree,memory);
        const auto address=reinterpret_cast<std::uint32_t>(&actor);
        Require(CollisionActor::RequestPath(actor,address,start,target,target_face,0u,path),"actual Monster pursuit path is accepted");
        std::cout<<"{\"case\":"<<std::quoted(label)<<",\"start\":";Vector(start);std::cout<<",\"target\":";Vector(supplied_target);
        std::cout<<",\"projected_target\":";Vector(target);std::cout<<",\"waypoints\":[";
        unsigned nodes{};const auto head=actor.words[21];
        for(auto node=memory.ReadDword(head);node!=head;node=memory.ReadDword(node)){
            Require(node && ++nodes<=128u,"bounded owned path node traversal");if(nodes>1u)std::cout<<',';
            Vector({std::bit_cast<float>(memory.ReadDword(node+8u)),std::bit_cast<float>(memory.ReadDword(node+12u)),std::bit_cast<float>(memory.ReadDword(node+16u))});}
        std::cout<<"],\"world_context\":"<<(use_world?"true":"false")<<",\"steps\":[";
        (void)CollisionActor::SetSpeedBits(actor,std::bit_cast<std::uint32_t>(150.0f));
        actor.words[28]=height_bits;actor.words[29]=radius_bits;(void)CollisionActor::SetCallback108(actor,0x444390u);
        const auto context=use_world?reinterpret_cast<std::uint32_t>(world.World()):0u;
        NativeCollisionActorMovementRuntime update(tree,memory);
        for(unsigned i=0u;i<3u;++i){
            const auto before=Position(actor);(void)CollisionActor::UpdateMovement(actor,address,context,0.017f,update);const auto after=Position(actor);
            if(i)std::cout<<',';std::cout<<"{\"before\":";Vector(before);std::cout<<",\"after\":";Vector(after);
            auto yaw=std::atan2(double(after.x-before.x),double(after.z-before.z))*180.0/3.14159265358979323846;if(yaw<0.0)yaw+=360.0;
            std::cout<<",\"yaw\":"<<yaw<<'}';}
        std::cout<<"]}\n";
        (void)CollisionActor::Destroy(actor,memory.Path());Require(memory.OutstandingAllocations()==0u,"replay path owner releases all temporary nodes");
    }
    Require(input.eof() && cases!=0u,"fully consumed nonempty path fixture");
    tree.Release();movement.Unload();world.Unload();
    std::cout<<"Replay completed using existing path/floor/movement code; callback27 and actual collision dimensions are bound; this is a bounded path-context replay, not complete game equivalence.\n";
}
}
