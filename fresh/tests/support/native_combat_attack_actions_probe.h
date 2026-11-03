#include "pal4/native_game_vector_math.h"
#pragma once
#include "pal4/native_combat_attack_actions.h"
#include "pal4/native_combat_action_children.h"
#include "pal4/native_combat_protection_selection.h"
#include "pal4/native_combat_actor_model_owner.h"
#include "pal4/native_combat_formula_catalog.h"
#include "pal4/native_combat_player_manager.h"
#include "pal4/native_combat_monster_manager.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_scene_actors.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_skill_system_services.h"
#include "pal4/native_script_event_router.h"
#include "pal4/game_resource_manager.h"
#include "pal4/native_actor_instance_owner.h"
#include "pal4/native_animated_actor_asset.h"
#include "pal4/random_number_generator.h"
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace pal4::test {
// Actual mixed managers/models with a scoped router. Full production console,
// script-protection consumers and downstream StartAttack are separate gates.
inline void VerifyNativeCombatAttackActions(NativeCombatPlayerManager& players,
    NativeCombatMonsterManager& monsters,std::span<NativeCombatActorSkillRuntime* const> skills,
    INativeCombatSceneActors& models,NativeCombatSystemImage& image,
    const NativeCombatFormulaCatalog& formulas,INativeSkillSystemServices& system,NativeScriptEventRouter& router){
    const auto check=[](bool b,const char* m){if(!b)throw std::runtime_error(m);};
    const auto reject=[&](auto&& f){bool failed{};try{f();}catch(const std::logic_error&){failed=true;}check(failed,"CheckProtect operation must reject");};
    const auto word=[](std::uint32_t a)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(a);};
    check(skills.size()==2u && players.StageCount()==2u,"CheckProtect actual two staged Players");
    const auto target=skills[0]->ActorAddress(),candidate=skills[1]->ActorAddress();
    const auto source=monsters.CreateMonster(2579,0u);
    auto& source_skills=monsters.Skills(source);(void)source_skills.RebuildStats();
    (void)monsters.ModelOwner(source).RefreshResource();
    const auto source_child=monsters.SceneChild(source),target_child=models.SceneChild(target),candidate_child=models.SceneChild(candidate);
    const auto position=[](std::uint32_t child){return *reinterpret_cast<const std::array<float,3>*>(child+172u);};
    struct Restore{NativeCombatSystemImage& image;INativeSkillSystemServices& system;RandomNumberGenerator rng;std::uint8_t mode;
        std::array<std::uint32_t*,6> fields;std::array<std::uint32_t,6> values;
        INativeCombatSceneActors& models;std::uint32_t candidate;std::array<float,3> p;
        ~Restore(){image.Mode()=mode;system.Random()=rng;for(unsigned i=0;i<fields.size();++i)*fields[i]=values[i];
            try{models.PositionSceneModel(candidate,{p[0],p[1],p[2]});}catch(...){}}}
        restore{image,system,system.Random(),image.Mode(),{&word(candidate+636u),&word(candidate+648u),&word(target+648u),&word(source+1148u),&word(source+828u),&word(source+1152u)},
        {word(candidate+636u),word(candidate+648u),word(target+648u),word(source+1148u),word(source+828u),word(source+1152u)},models,candidate,position(candidate_child)};
    image.Mode()=0u;word(candidate+636u)=0u;word(candidate+648u)=0u;word(target+648u)=1u;word(source+1148u)=0u;word(source+1152u)=0u;
    NativeCombatProtectionSelection selection(players,monsters,skills,formulas,system);
    NativeCombatActionChildren children([&](std::uint32_t a,const std::array<float,3>& p){
        if(a==source){monsters.PositionSceneModel(a,{p[0],p[1],p[2]});return;}
        check(a==candidate,"original position child moves selected protector");models.PositionSceneModel(a,{p[0],p[1],p[2]});
    },[&](std::uint32_t a,std::uint32_t protected_actor,std::uint32_t other){
        check(a==candidate && protected_actor==source && other==target,"original response child preserves three distinct actors");
        (void)skills[1]->ProtectAction(protected_actor,other);
    });
    unsigned faces{},runs{};
    children.BindApproach({
        [&](std::uint32_t a){check(a==source,"approach queries actual source model");return position(source_child);},
        [&](std::uint32_t a,const std::array<float,3>& v){check(a==source,"FaceTask rotates actual source");monsters.ModelOwner(a).RotateSceneModel({v[0],v[1],v[2]});++faces;},
        [&](std::uint32_t a){
            check(a==source,"RunTask animates actual source");monsters.ModelOwner(a).PlayAction("z02",1u,system);
            check(_stricmp(monsters.ModelOwner(a).Model()->Asset()->CurrentAction().c_str(),"z02")==0,"actual source model selected z02 clip");++runs;
        },
        [](std::array<float,3>& v){RwV3d value{v[0],v[1],v[2]};(void)RwV3dNormalize(&value,&value);v={value.x,value.y,value.z};},
        [](const std::array<float,3>& v){return pal4::NativeGameApproximateMagnitude(v);}
    });
    NativeCombatAttackActions actions(players,monsters,skills,models,selection,children,image);
    struct Routes{NativeScriptEventRouter& router;std::vector<std::uint32_t> handles;~Routes(){for(auto h:handles)(void)router.Unsubscribe(h);}} routes{router};
    unsigned starts{},responses{};bool fail_start{};
    routes.handles.push_back(router.Subscribe(router.Key("ae_start_action"),1u,[&](std::uint32_t p){
        if(word(p)!=source+716u)return;++starts;
        reject([&]{actions.Close();});reject([&]{(void)actions.Create(source,target,1u);});
        if(fail_start)throw std::logic_error("intentional CheckProtect start observer failure");
    }));
    routes.handles.push_back(router.Subscribe(router.Key("ae_protect_action"),1u,[&](std::uint32_t p){
        check(word(p)==source+716u && word(p+4u)==target && word(p+8u)==candidate,"actual CheckProtect event record");++responses;
        reject([&]{actions.Close();});reject([&]{children.Close();});
    }));
    std::array<std::uint32_t,2> seeds{};std::array<bool,2> found{};
    for(std::uint32_t seed=0;seed<256u && (!found[0] || !found[1]);++seed){
        system.Random().Seed(seed);const auto chosen=selection.Select(source,target);const auto index=chosen?1u:0u;
        if(!found[index]){found[index]=true;seeds[index]=seed;}
    }
    check(found[0] && found[1],"actual probability supports both parent branches");
    const auto source_pos=position(source_child),target_pos=position(target_child);
    const auto bounds=*reinterpret_cast<const std::array<float,6>*>(source_child+524u);
    std::array<float,3> expected{};float length{};for(unsigned i=0;i<3u;++i)length+=(source_pos[i]-target_pos[i])*(source_pos[i]-target_pos[i]);length=std::sqrt(length);
    check(length>0.f,"actual protection geometry has distinct positions");
    for(unsigned i=0;i<3u;++i)expected[i]=target_pos[i]+(source_pos[i]-target_pos[i])/length*((bounds[0]-bounds[3])*0.5f+50.f);
    for(unsigned branch=0;branch<2u;++branch){
        system.Random().Seed(seeds[branch]);const auto a=actions.Create(source,target,7u);
        check(actions.Owns(a) && actions.Identity(a)==source && actions.Processed(a) && actions.Enabled(a),"real factory/identity/processed flag");
        reject([&]{(void)actions.ResultEvent(a);});actions.Prepare(a);reject([&]{actions.Prepare(a);});
        check(word(source+828u)==2u && word(a+52u)==(branch?candidate:0u) && children.Count()==(branch?2u:0u),"real start precedes selection and original child recipe");
        actions.BeginWaiting(a);for(unsigned tick=0;tick<16u && !actions.Ready(a);++tick)actions.Update(a,0.1f);
        check(actions.Ready(a) && !children.Count(),"real protection GOM drains");
        if(branch){const auto moved=position(candidate_child);for(unsigned i=0;i<3u;++i)check(std::abs(moved[i]-expected[i])<0.002f,"original bounds/SDK guard position");}
        const auto next=actions.ResultEvent(a);
        check(next && actions.Owns(next) && word(next)==0x845924u && word(next+44u)==source && word(next+48u)==(branch?candidate:target) &&
            *reinterpret_cast<const std::uint8_t*>(next+52u)==(branch?0u:1u) && *reinterpret_cast<const std::uint8_t*>(next+53u)==(branch?1u:0u) &&
            *reinterpret_cast<const std::uint8_t*>(next+54u)==7u,"actual StartAttack successor branch/flags");
        actions.Prepare(next);check(children.Count()==2u,"StartAttack schedules actual Face then Run children");
        actions.BeginWaiting(next);for(unsigned tick=0;tick<2000u && !actions.Ready(next);++tick)actions.Update(next,0.01f);
        check(actions.Ready(next) && position(source_child)!=source_pos && faces==branch+1u && runs==branch+1u,"actual StartAttack turns source, plays z02 and advances model");
        const auto physical=actions.ResultEvent(next);
        check(word(physical)==0x845948u && word(physical+44u)==source && word(physical+48u)==(branch?candidate:target) &&
            *reinterpret_cast<const std::uint8_t*>(physical+54u)==7u,"actual PhysicalAttack successor preserves identities/mode");
        reject([&]{actions.Prepare(physical);});reject([&]{(void)actions.ResultEvent(a);});
        actions.Cancel(a);actions.Cancel(next);actions.Cancel(physical);
        monsters.PositionSceneModel(source,{source_pos[0],source_pos[1],source_pos[2]});
        check(!actions.Count() && !actions.OwnedAllocations() && !actions.OwnedTexts() && !children.OwnedAllocations(),"parent/successor/container/host names released");
    }
    check(starts==4u && responses==1u,"protection and approach dispatch expected actual events");
    // Actual Monster420/424 nonzero: fixed target position, Face only.
    word(source+1148u)=1u;word(source+1152u)=1u;
    const auto face_parent=actions.Create(source,target,5u);actions.Prepare(face_parent);actions.BeginWaiting(face_parent);
    for(unsigned tick=0;tick<8u && !actions.Ready(face_parent);++tick)actions.Update(face_parent,0.01f);
    const auto face_start=actions.ResultEvent(face_parent);actions.Prepare(face_start);
    check(children.Count()==1u,"actual source424 prevents Run after Face creation");actions.BeginWaiting(face_start);
    for(unsigned tick=0;tick<12u && !actions.Ready(face_start);++tick)actions.Update(face_start,0.01f);
    check(actions.Ready(face_start) && faces==3u && runs==2u && position(source_child)==source_pos,
        "blocked/fixed-target branch faces without moving or playing run");
    const auto face_physical=actions.ResultEvent(face_start);actions.Cancel(face_parent);actions.Cancel(face_start);actions.Cancel(face_physical);
    word(source+1148u)=0u;word(source+1152u)=0u;
    const auto failed=actions.Create(source,target,1u);fail_start=true;reject([&]{actions.Prepare(failed);});fail_start=false;
    reject([&]{actions.BeginWaiting(failed);});actions.Cancel(failed);
    check(!children.Count() && !children.OwnedAllocations() && !actions.Count(),"failed StartAction can cancel without child leak");
    check(actions.Create(0u,target,1u)==0u,"null source factory gate");reject([&]{(void)actions.Create(0x1000u,target,1u);});
    image.Mode()=1u;reject([&]{(void)actions.Create(source,target,1u);});image.Mode()=0u;
    actions.Close();children.Close();reject([&]{(void)actions.Create(source,target,1u);});
    std::cout<<"Native CheckProtect parent: actual mixed actors, start/select/SDK bounds position/response, both StartAttack successors, failure/retirement guards and zero allocations pass\n";
    std::cout<<"Native StartAttack: actual Face/Run, SDK geometry, z02 and model movement, fixed-target/Face-only branch, PhysicalAttack construction and zero allocations pass\n";
}
}
