#pragma once
#include "pal4/native_combat_player_equipment.h"
#include "pal4/native_combat_player_actor_owner.h"
#include "pal4/native_actor_instance_owner.h"
#include "pal4/native_effect_scene_session.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_inventory_session.h"
#include "native_combat_player_model_cache_probe.h"
#include <rpworld.h>
#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeCombatPlayerEquipment(NativePlayerManagerSession& source,
    const NativeGameDatabase& database,NativePkFileManagerRuntime& packages,
    const std::filesystem::path& root,std::uint32_t game,INativeSkillSystemServices& skills,
    NativeEffectSceneSession& effects) {
    const auto check=[](bool ok,const char* m){if(!ok)throw std::runtime_error(m);};
    const auto address=[](const void* p){return reinterpret_cast<std::uint32_t>(p);};
    const auto word=[](std::uint32_t a){std::uint32_t v;std::memcpy(&v,reinterpret_cast<const void*>(a),4u);return v;};
    const auto reject=[&](auto&& f){bool denied=false;try{f();}catch(const std::logic_error&){denied=true;}check(denied,"Player equipment expected rejection");};
    const auto snapshot=source.State();
    // A separate inventory exercises real Equip records without changing the
    // game's persistent inventory or source Player equipment sequence.
    NativeInventorySession inventory;inventory.BindDatabase(database);(void)inventory.Start();
    struct World {RpWorld* p{};~World(){if(p)RpWorldDestroy(p);}} world;
    RwBBox bounds{{1000,1000,1000},{-1000,-1000,-1000}};
    world.p=RpWorldCreate(&bounds);check(world.p,"Player equipment isolated SDK world");
    std::array<std::uint32_t,100> manager{};
    std::array<std::uint32_t,130> combat{};
    combat[33]=address(world.p);combat[41]=1u;
    NativeScriptEventRouter router;
    effects.BindCombatEffects(address(manager.data()),address(combat.data()),router);
    struct Retire {NativeEffectSceneSession& e;bool active=true;~Retire(){if(active)try{e.ClearCombatEffects();e.UnbindCombatEffects();}catch(...){}}} retire{effects};
    for(unsigned i=0;i<4u;++i) {
        inventory.Reset();
        NativeCombatPlayerActorOwner actor;const auto a=actor.Construct(source,i);
        check(!inventory.FindInventoryEntry(inventory.Address()+124u,word(a+896u),7u),
              "each Player default branch starts with isolated empty equipment");
        NativeActorInstanceOwner model;(void)model.Construct(game);
        const std::string name(actor.RequestedModel());check(!name.empty(),"actual Player requested model copied");
        model.Load(root,name);(void)model.InitializeFrame();
        model.BindHandResources(root,packages,effects,skills,[]{return false;});
        reject([&]{(void)NativeCombatPlayerEquipment::Refresh(actor,model,inventory,120300);});
        (void)model.BindParent(a);
        struct Return {NativeActorInstanceOwner& m;~Return(){try{m.ReleaseToPool();}catch(...){}}} return_model{model};
        const auto role=word(a+1192u);check(role==i,"actual Player role identity differs from formation slot");
        (void)NativeCombatPlayerEquipment::Refresh(actor,model,inventory,120300);
        const auto left=word(model.Address()+624u+20u),right=word(model.Address()+1088u+20u);
        check((i==3u?right:left)!=0u && (i!=1u || right),"actual Player equipment loads expected hands");
        const auto saved_left=left,saved_right=right;
        (void)NativeCombatPlayerEquipment::Refresh(actor,model,inventory,120300);
        check(word(model.Address()+644u)==saved_left && word(model.Address()+1108u)==saved_right,
              "same Player equipment refresh reuses actual SDK hand clumps");
        if(i==0u) {
            const auto sword=inventory.ProcessEquipment(inventory.Address()+124u,3501u);
            check(sword!=0u,"Player equipment original Equip3501");
            const auto sequence=inventory.ReadDword(sword+400u);
            std::memcpy(reinterpret_cast<void*>(a+896u),&sequence,4u);
            (void)NativeCombatPlayerEquipment::Refresh(actor,model,inventory,120300);
            check(std::string_view(reinterpret_cast<const char*>(word(model.Address()+628u)))=="WTA" &&
                  std::string_view(reinterpret_cast<const char*>(word(model.Address()+640u)))=="WTA1",
                  "actual Player equipment selects original record model and texture");
            std::cout<<"Native Player equipment original inventory: Equip3501 model=WTA texture=WTA1\n";
        }
        check(!word(a+808u) && !word(a+1296u) && !word(a+1300u),
              "equipment preparation does not publish cache/stage entry");
        model.ReleaseToPool();
        reject([&]{(void)NativeCombatPlayerEquipment::Refresh(actor,model,inventory,120300);});
        model.Destroy();actor.ValidateDetached();actor.Destroy();
        check(!effects.CombatXmlEffectCount() && !effects.CombatActiveCount(),"Player equipment drains isolated Combat effects");
        std::cout<<"Native Player equipment probe: role="<<i<<" model="<<name<<" hands="<<(left?1:0)+(right?1:0)<<" retired=1\n";
    }
    VerifyNativeCombatPlayerModelCache(source,inventory,packages,root,game,skills,effects,router);
    effects.ClearCombatEffects();effects.UnbindCombatEffects();retire.active=false;
    inventory.Close();check(!inventory.OutstandingAllocations(),"isolated Player equipment inventory retired");
    check(!std::memcmp(&snapshot,&source.State(),sizeof(snapshot)),"equipment probe preserves source Player session");
}
}
