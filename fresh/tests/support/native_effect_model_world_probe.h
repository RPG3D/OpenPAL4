#pragma once
#include "pal4/native_arena_effect_services.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_gob_clump_asset.h"
#include "pal4/game_world_object.h"
#include <array>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeEffectModelWorld(const std::filesystem::path& root) {
    const auto check=[](bool ok,const char* m){if(!ok)throw std::runtime_error(m);};
    const auto address=[](const void* p){return reinterpret_cast<std::uint32_t>(p);};
    struct World {RpWorld* p{};~World(){if(p)RpWorldDestroy(p);}} world;
    RwBBox bounds{{1000,1000,1000},{-1000,-1000,-1000}};
    world.p=RpWorldCreate(&bounds);check(world.p,"effect model world fixture");
    GameWorldObjectState arena{},combat{},cold{};
    arena.words[4]=combat.words[4]=address(world.p);arena.words[12]=combat.words[12]=1u;
    std::array<std::uint32_t,65> arena_group{},combat_group{};
    NativeBaseObjectMemory memory;
    NativeArenaEffectServices arena_services(memory,address(arena_group.data()));
    NativeArenaEffectServices combat_services(memory,address(combat_group.data()));
    NativeGobClumpAsset asset;check(asset.Load(root,"gamedata\\PALWeapon\\WTA\\","WTA"),"effect model original WTA");
    const auto clump=address(asset.Clump());
    combat_services.BindCombatWorld(address(&cold));
    check(!combat_services.AddActiveObject(address(combat_group.data()),clump) && !RpClumpGetWorld(asset.Clump()),
          "cold Combat world declines without substituting scene world");
    combat_services.UnbindArena();combat_services.BindCombatWorld(address(&combat));
    bool rejected{};try{(void)combat_services.AddActiveObject(address(arena_group.data()),clump);}catch(const std::logic_error&){rejected=true;}
    check(rejected && !RpClumpGetWorld(asset.Clump()),"effect model rejects foreign owner before SDK attachment");
    check(combat_services.AddActiveObject(address(combat_group.data()),clump) && RpClumpGetWorld(asset.Clump())==world.p,
          "Combat callback returns actual SDK world addition");
    check(!combat_services.AddActiveObject(address(combat_group.data()),clump),"Combat callback returns false for duplicate world membership");
    arena_services.BindArena(address(&arena));
    check(arena_services.AddActiveObject(address(arena_group.data()),clump),"Arena callback preserves true after declined duplicate addition");
    asset.Unload();
    combat_services.UnbindArena();arena_services.UnbindArena();
    check(!memory.OutstandingAllocations(),"effect world callback probe leaves queue allocator balanced");
    std::cout<<"Native effect model worlds: actual Combat add result, cold/duplicate/foreign rejection, distinct Arena return policy and SDK clump retirement verified\n";
}
}
