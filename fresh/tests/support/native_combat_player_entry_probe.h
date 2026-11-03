#pragma once
#include "pal4/native_combat_actor_entry.h"
#include "pal4/native_combat_actor_effects.h"
#include "pal4/native_combat_player_motion.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_skill_actor_registry.h"
#include "pal4/native_combat_player_manager.h"
#include "pal4/native_combat_player_model_cache.h"
#include "pal4/native_combat_player_actor_owner.h"
#include "pal4/native_actor_instance_owner.h"
#include "pal4/native_animated_actor_asset.h"
#include "pal4/native_player_collection_values.h"
#include "pal4/native_effect_scene_session.h"
#include "pal4/native_effect_handle_content_runtime.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_pk_file_manager_runtime.h"
#include "pal4/effect_handle.h"
#include <algorithm>
#include <iostream>
#include <vector>
namespace pal4::test {
inline void VerifyNativeCombatPlayerEntry(NativeCombatPlayerManager& players,
    NativeCombatPlayerModelCache& one,NativeCombatPlayerModelCache& two,
    const NativeGameDatabase& database,NativeInventorySession& inventory,
    NativeSkillRecordCatalog& records,INativeSkillSystemServices& system,
    NativeScriptEventRouter& router,NativeEffectSceneSession& effects,NativePkFileManagerRuntime& packages) {
    const auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    const auto word=[](std::uint32_t a)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(a);};
    const auto first=one.ActorAddress(),second=two.ActorAddress();
    NativeCombatSkillActorRegistry registry(router,records,system);
    NativeCombatActorSkillRuntime first_skills(players.Actor(first),registry,inventory),
        second_skills(players.Actor(second),registry,inventory);
    NativePlayerCollectionCatalog passives(database);
    NativeCombatPlayerMotion motion(players,system,router,[&](std::uint32_t a)->NativeCombatPlayerMotionBinding {
        if(a==first)return {one,first_skills};
        if(a==second)return {two,second_skills};
        throw std::invalid_argument("Player entry probe foreign motion actor");
    });
    bool initializing=true;
    std::vector<std::string> trace;
    std::vector<std::uint32_t> subscriptions;
    for(const auto name:{"re_join_stage","re_join_scene","ae_become_alive","ae_attach_effect"}) {
        subscriptions.push_back(router.Subscribe(router.Key(name),1u,[&,name](std::uint32_t payload){
            const auto a=word(payload)-716u;
            if(a!=first && a!=second)return;
            check(word(a+828u)==1u && !word(a+832u) && !*reinterpret_cast<const std::uint8_t*>(a+892u),
                  "Player full initialization resets state before stage/effect events");
            if(initializing) {
            auto& cache=a==first?one:two;
            auto& skills=a==first?first_skills:second_skills;
            bool denied=false;
            try{InitializeNativeCombatActor(players.Actor(a),cache,skills,passives,effects,router,motion,0u);}
            catch(const std::logic_error&){denied=true;}
            check(denied,"Player entry callback cannot recursively reinitialize actor");
            denied=false;try{cache.Close();}catch(const std::logic_error&){denied=true;}
            check(denied,"Player entry callback cannot retire cache");
            }
            trace.push_back(std::to_string(a)+":"+name);
        }));
    }
    struct Unsubscribe {NativeScriptEventRouter& router;std::vector<std::uint32_t>& ids;
        ~Unsubscribe(){for(auto id:ids)(void)router.Unsubscribe(id);}} unsub{router,subscriptions};
    // A mismatched borrowed cache must reject before even the first state write.
    word(first+832u)=77u;bool rejected=false;
    try{InitializeNativeCombatActor(players.Actor(first),two,first_skills,passives,effects,router,motion,0u);}
    catch(const std::invalid_argument&){rejected=true;}
    check(rejected && word(first+832u)==77u && !players.StageCount(),"Player entry preflights cache identity");
    for(auto* cache:{&one,&two}) {
        const auto a=cache->ActorAddress();
        auto& skills=a==first?first_skills:second_skills;
        word(a+832u)=77u;*reinterpret_cast<std::uint8_t*>(a+892u)=1u;
        InitializeNativeCombatActor(players.Actor(a),*cache,skills,passives,effects,router,motion,a==first?0u:1u);
        check(cache->Current().Address()==word(a+808u) && skills.ReadyForAction() &&
              _stricmp(cache->Current().Asset()->CurrentAction().c_str(),"z01")==0,
              "Player full52CFD0 prepares real model, stats, motion and alive state");
        const auto stage=std::to_string(a)+":re_join_stage",scene=std::to_string(a)+":re_join_scene";
        check(std::find(trace.begin(),trace.end(),stage)<std::find(trace.begin(),trace.end(),scene),
              "Player stage event precedes real world scene event");
        const auto alive=std::to_string(a)+":ae_become_alive",attached=std::to_string(a)+":ae_attach_effect";
        check(std::find(trace.begin(),trace.end(),scene)<std::find(trace.begin(),trace.end(),alive) &&
              std::find(trace.begin(),trace.end(),alive)!=trace.end(),"Player scene precedes alive event");
        if(EffectHandle::DialogNeedsUpdate({word(a+868u),word(a+872u)},effects.ContentHandles())) {
            check(std::find(trace.begin(),trace.end(),alive)<std::find(trace.begin(),trace.end(),attached) &&
                  std::find(trace.begin(),trace.end(),attached)!=trace.end(),"Player alive precedes bottom attachment");
        } else {
            check(packages.AddEffectScriptAndProcess()!=0u,"Player bottom resource package mounts");
            struct Scope {NativePkFileManagerRuntime& p;~Scope(){(void)p.CleanupAllScripts();}} scope{packages};
            const auto path="gamedata\\effect\\script\\"+std::string(cache->Current().ModelName())+".eft";
            const auto file=packages.OpenEffectScriptPath(path.c_str());
            if(file)(void)packages.CloseEffectScriptMember(file);
            check(!file && word(a+868u)==0xffffffffu && !word(a+872u),
                  "Player absent original model EFT preserves invalid bottom handle");
            std::cout<<"Native Player bottom resource absent: "<<path<<" original invalid handle retained\n";
        }
        check(word(a+876u)==0xffffffffu && !word(a+880u) && word(a+884u)==0xffffffffu && !word(a+888u),
              "ordinary Player model id1196 creates no special Monster hand effects");
    }
    initializing=false;
    check(players.StageCount()==2u,"full Player initialization enters two actual stage members");
    // Exercise the newly shared Player52E2E0 boundary with an actual original
    // resource. hook_area must use fixed448/8449E4, not per-slot444 or Monster.
    NativeCombatActorEffects attachment(one,effects,router);
    const auto created=attachment.Create("foot_2","hook_area");
    EffectSystemHandleWords handle{created.id,created.owner};
    check(EffectHandle::DialogNeedsUpdate(handle,effects.ContentHandles()),"Player effect uses live Combat group");
    const auto node=effects.ContentHandles().FindElement(handle.system+12u,handle);
    const auto state=effects.ContentHandles().ReadWord(node+16u);
    const auto* floats=reinterpret_cast<const float*>(state);
    check(floats[44]==8.0f && floats[45]==0.0f && floats[46]==156.0f,
          "Player fixed448 effect position is actual8449E4");
    effects.ReleaseCombatEffect(handle,true);
    // Full initialization's actual bottom handles are retired by the existing
    //52DE40 tests later in this world fixture, before model/cache destruction.
    std::cout<<"Native52CFD0 Player entry: actual2 M01 actors, state/slot, passive/stat cleanup, stage before scene, z01/alive, handle reset, identity guard and shared52E2E0 fixed448 effect pass\n";
}
}
