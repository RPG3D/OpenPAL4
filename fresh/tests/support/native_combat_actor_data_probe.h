#pragma once
#include "pal4/native_combat_actor_data.h"
#include "pal4/native_combat_player_scene_actors.h"
#include "pal4/native_combat_player_manager.h"
#include "pal4/native_combat_player_model_cache.h"
#include "pal4/native_combat_monster_manager.h"
#include "pal4/native_combat_actor_model_owner.h"
#include "pal4/native_actor_instance_owner.h"
#include "pal4/native_animated_actor_asset.h"
#include "pal4/native_combat_task_queue.h"
#include <cmath>
#include <limits>
#include <rwcore.h>
#include <rphanim.h>
#include <rtanim.h>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace pal4::test {
inline void VerifyNativeCombatActorData(NativeCombatPlayerSceneActors& scene,
    NativeCombatPlayerManager& players,NativeCombatMonsterManager& monsters,
    const std::vector<std::unique_ptr<NativeCombatPlayerModelCache>>& models,
    NativeCombatTaskQueue& queue) {
    auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    const auto player_count=players.Count(),monster_count=monsters.Count();
    const auto player_stage=players.StageCount(),monster_stage=monsters.StageCount();
    const auto domains=queue.DomainCount();
    check(player_stage && monster_stage && !queue.Count(),"actor data probe requires actual staged actors and an idle queue");
    std::vector<RpHAnimHierarchy*> hierarchies;
    for(const auto& model:models)hierarchies.push_back(model->Current().Asset()->Hierarchy());
    for(std::size_t i=0;i<monster_stage;++i)
        hierarchies.push_back(monsters.ModelOwner(monsters.StageActorAt(i)).Model()->Asset()->Hierarchy());
    std::vector<float> before;
    for(auto* hierarchy:hierarchies){
        check(hierarchy && hierarchy->currentAnim,"actual staged actor has an SDK animation interpolator");
        before.push_back(hierarchy->currentAnim->currentTime);
    }
    for(float invalid:{-1.0f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}) {
        bool rejected=false;
        try{UpdateNativeCombatActorData(invalid,scene,monsters,queue);}
        catch(const std::invalid_argument&){rejected=true;}
        check(rejected,"invalid actor delta rejected before touching actors");
    }
    for(std::size_t i=0;i<hierarchies.size();++i)
        check(hierarchies[i]->currentAnim->currentTime==before[i],"invalid delta leaves actual SDK clocks unchanged");
    UpdateNativeCombatActorData(1.0f/60.0f,scene,monsters,queue);
    for(std::size_t i=0;i<hierarchies.size();++i)
        check(std::isfinite(hierarchies[i]->currentAnim->currentTime) &&
              hierarchies[i]->currentAnim->currentTime!=before[i],"every staged SDK animation advances through actor data composition");
    for(unsigned frame=1;frame<120;++frame)UpdateNativeCombatActorData(1.0f/60.0f,scene,monsters,queue);
    for(auto* hierarchy:hierarchies)check(std::isfinite(hierarchy->currentAnim->currentTime),"SDK clock finite after continuous actor updates");
    check(players.Count()==player_count && monsters.Count()==monster_count &&
          players.StageCount()==player_stage && monsters.StageCount()==monster_stage &&
          !players.ActiveIterators() && !monsters.ActiveIterators() &&
          !queue.Count() && queue.DomainCount()==domains,"actor update preserves actual rosters, cursors and task bindings");
    std::cout<<"Native combat actor data: frames=120 Players="<<player_stage<<" Monsters="<<monster_stage
             <<" SDK_clocks_advanced="<<hierarchies.size()<<" invalid_dt=3 cursors=0 queued_tasks=0\n";
}
}
