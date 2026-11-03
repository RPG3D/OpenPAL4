#pragma once
#include "pal4/native_arena_collision_tree.h"
#include "pal4/native_game_arena_manager_owner.h"
#include <array>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

namespace pal4::test {
inline void VerifyNativeCollisionCache(RpClump* original_floor) {
    const auto check=[](bool ok) {
        if (!ok) throw std::runtime_error("native collision cache probe");
    };
    NativeGameArenaManagerOwner arena;
    const auto address=arena.Construct();
    auto* words=reinterpret_cast<std::uint32_t*>(address);
    std::array<std::uint32_t,14> scene{};
    words[66]=reinterpret_cast<std::uint32_t>(scene.data());
    std::string world="COLLISION_CACHE_WORLD_LONGER_THAN_INITIAL_TEXT_CAPACITY";
    std::string area="COLLISION_CACHE_AREA_A_LONGER_THAN_INITIAL_TEXT_CAPACITY";
    const auto names=[&] {
        words[2]=reinterpret_cast<std::uint32_t>(world.c_str());
        words[3]=reinterpret_cast<std::uint32_t>(area.c_str());
    };
    names();
    NativeArenaCollisionTree tree;
    tree.BindCache(arena);
    const auto build=[&] { names(); check(tree.Build(address,original_floor)); };
    build();
    const auto first=tree.TreeAddress();
    const auto faces=tree.FaceCount();
    const auto edges=tree.LinkedEdges();
    const auto allocations=tree.OutstandingAllocations();
    check(first && faces>0 && edges>0 && allocations>1 && tree.OwnedTreeCount()==1);
    tree.LeaveScene();
    check(!words[101] && !tree.TreeAddress() && tree.FaceCount()==0 &&
          words[133]==1 && words[135]==first && tree.OutstandingAllocations()==allocations);
    std::transform(world.begin(),world.end(),world.begin(),[](unsigned char c){return std::tolower(c);});
    std::transform(area.begin(),area.end(),area.begin(),[](unsigned char c){return std::tolower(c);});
    build();
    check(tree.TreeAddress()==first && tree.FaceCount()==faces && tree.LinkedEdges()==edges &&
          words[134]==1 && tree.OutstandingAllocations()==allocations);
    tree.LeaveScene();
    check(!words[101] && tree.OwnedTreeCount()==1 && words[135]==first && !words[144]);
    for (const char suffix : {'B','C'}) {
        area="COLLISION_CACHE_AREA_"+std::string(1,suffix)+"_LONGER_THAN_INITIAL_TEXT_CAPACITY";
        build(); tree.LeaveScene();
    }
    check(tree.OwnedTreeCount()==3 && tree.OutstandingAllocations()==3*allocations &&
          words[135] && words[144] && words[153]);
    area="COLLISION_CACHE_AREA_D_LONGER_THAN_INITIAL_TEXT_CAPACITY";
    build();
    const auto replacement=tree.TreeAddress();
    check(tree.OwnedTreeCount()==4);
    tree.LeaveScene();
    check(words[135]==replacement && words[135]!=first && words[133]==1 && !words[134] &&
          tree.OwnedTreeCount()==3 && tree.OutstandingAllocations()==3*allocations);
    // Scene type1 bypasses cache lookup and insertion, even for a matching name.
    scene[13]=1;
    build();
    check(tree.TreeAddress()!=replacement && tree.OwnedTreeCount()==4);
    tree.LeaveScene();
    check(tree.OwnedTreeCount()==3 && tree.OutstandingAllocations()==3*allocations &&
          words[135]==replacement && !words[101]);
    words[66]=0; // Missing Scene record also bypasses lookup/insertion.
    build(); tree.LeaveScene();
    check(tree.OwnedTreeCount()==3 && tree.OutstandingAllocations()==3*allocations);
    tree.Release();
    check(!tree.OwnedTreeCount() && !tree.OutstandingAllocations() && !words[101]);
    for (const auto base : {133u,142u,151u})
        check(!words[base] && !words[base+1] && !words[base+2]);
    tree.Release();
    check(!tree.OutstandingAllocations());
    words[2]=words[3]=0;
    arena.Destroy();
    check(!arena.OutstandingAllocations());
}
}  // namespace pal4::test
