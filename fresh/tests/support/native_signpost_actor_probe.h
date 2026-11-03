#pragma once
#include "pal4/native_arena_signpost_actor.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_pk_file_manager_runtime.h"
#include "pal4/native_pal_game_iv_texture_storage.h"
#include <rwcore.h>
#include <rpworld.h>
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
inline std::size_t SignpostWorldClumps(RpWorld* world){
    std::size_t count{};
    (void)RpWorldForAllClumps(world,[](RpClump* c,void* p)->RpClump*{++*static_cast<std::size_t*>(p);return c;},&count);
    return count;
}
inline void VerifyNativeSignpostActor(const std::filesystem::path& root,
    NativePalGameIVTextureStorage& game,NativePkFileManagerRuntime& packages,std::uint32_t arena){
    const auto check=[](bool v){if(!v)throw std::runtime_error("actual signpost actor lifecycle assertion");};
    const auto scripts=packages.LoadedPackageCount();
    NativeBaseObjectMemory memory;
    NativeArenaSignpostActor actor(memory,game,packages,root);
    check(actor.Address() && actor.State().words[0]==0x8448a0u &&
          memory.ReadDword(actor.Address()+368u)==0x8448ccu && !actor.Clump());
    bool rejected=false;try{actor.Load(0u);}catch(const std::invalid_argument&){rejected=true;}
    check(rejected && !actor.Clump());
    actor.Load(1u);
    check(actor.Clump() && actor.State().words[5]==reinterpret_cast<std::uint32_t>(actor.Clump()) &&
          actor.State().words[9]==1u && packages.LoadedPackageCount()==scripts);
    check(memory.ReadDword(actor.Address()+376u)==0u); // Original absent M001 TAB.
    const auto allocations=memory.OutstandingAllocations();
    rejected=false;try{actor.Load(2u);}catch(const std::logic_error&){rejected=true;}
    check(rejected && memory.OutstandingAllocations()==allocations);
    actor.SetPosition({12.0f,3.0f,-7.0f});actor.SetYaw(90.0f);
    const auto* frame=RwFrameGetLTM(RpClumpGetFrame(actor.Clump()));
    const auto* position=RwMatrixGetPos(frame);
    check(std::fabs(position->x-12.0f)<0.001f && std::fabs(position->y-3.0f)<0.001f &&
          std::fabs(position->z+7.0f)<0.001f);
    actor.Attach(arena);
    auto* world=RpClumpGetWorld(actor.Clump());check(world!=nullptr);
    const auto attached=SignpostWorldClumps(world);
    actor.Detach(arena);check(!RpClumpGetWorld(actor.Clump()) && SignpostWorldClumps(world)+1u==attached);
    actor.Attach(arena);check(SignpostWorldClumps(world)==attached);
    actor.Close();actor.Close();
    check(!actor.Address() && !actor.Clump() && !memory.OutstandingAllocations() &&
          SignpostWorldClumps(world)+1u==attached && packages.LoadedPackageCount()==scripts);
    {
        NativeArenaSignpostActor failure(memory,game,packages,root/"missing-signpost-root");
        rejected=false;try{failure.Load(1u);}catch(const std::runtime_error&){rejected=true;}
        check(rejected && packages.LoadedPackageCount()==scripts);
        failure.Close();check(!memory.OutstandingAllocations());
    }
    std::cout<<"Native signpost actor: original492 ctor/load, M001, position/yaw, actual world attach/detach, attached cleanup, failed load and zero allocations pass\n";
}
}
