#pragma once
#include "pal4/native_combat_main_input.h"
#include "pal4/native_combat_roster_targets.h"
#include "pal4/native_combat_target_selection.h"
#include "pal4/native_combat_player_manager.h"
#include "pal4/native_combat_player_actor_owner.h"
#include "pal4/native_combat_monster_manager.h"
#include "pal4/native_combat_actor_model_owner.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_combat_task_queue.h"
#include "pal4/native_combat_player_action_memory.h"
#include "pal4/native_combat_attack_actions.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace pal4::test {
inline void VerifyNativeCombatMainInput(NativeCombatMainInput& input,NativeCombatRosterTargets& targets,
    NativeCombatTargetSelection& control,NativeCombatPlayerManager& players,NativeCombatMonsterManager& monsters,
    NativeCombatSystemImage& system,NativeScriptEventRouter& router,NativeCombatTaskQueue& queue,
    NativeCombatPlayerActionMemory& memory,NativeCombatAttackActions& actions,
    NativeCombatActorSkillRuntime& source,const NativeCombatActorSkillRuntime::ActionMotionRefresh& motion) {
    const auto check=[](bool b,const char* message){if(!b)throw std::runtime_error(message);};
    const auto word=[](std::uint32_t a)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(a);};
    const auto base=reinterpret_cast<std::uint32_t>(&system);
    const auto before=system.target_selection.words;const auto state=system.State();
    std::vector<std::uint32_t> routes;
    struct Restore {NativeCombatRosterTargets& targets;NativeCombatSystemImage& system;NativeScriptEventRouter& router;
        const std::array<std::uint32_t,21>& saved;std::uint32_t state;std::vector<std::uint32_t>& routes;
        ~Restore(){for(auto h:routes)(void)router.Unsubscribe(h);
            const auto current=system.target_selection.words[16];if(current)(void)targets.SetTargetSelected(current,0u);
            for(unsigned i=10;i<21u;++i)system.target_selection.words[i]=saved[i];system.State()=state;}
    } restore{targets,system,router,before,state,routes};
    check(players.StageCount()==2u && monsters.StageCount()==3u && !before[16],"Main input fixture requires actual first encounter with no prior target");
    system.State()=2u;
    std::array<std::uint32_t,5> event{};unsigned repeats{},substitutes{},confirms{},cancels{},tips{};std::uint32_t expected_target{};
    const auto run=[&](std::uint32_t key){event[4]=key;return input.Handle(reinterpret_cast<std::uint32_t>(event.data()));};
    routes.push_back(router.Subscribe(router.Key("repeat_action"),1u,[&](std::uint32_t p){
        check(word(p)==reinterpret_cast<std::uint32_t>(event.data()),"repeat_action forwards pointer-to-original-input");
        bool rejected{};try{input.Handle(reinterpret_cast<std::uint32_t>(event.data()));}catch(const std::logic_error&){rejected=true;}
        check(rejected,"Main input callback reentry rejected");++repeats;
    }));
    routes.push_back(router.Subscribe(router.Key("re_substitute"),1u,[&](std::uint32_t p){
        const auto actor=word(p)-716u;check(players.Actor(actor).Slot()<3u,"role shortcut emits actual staged Player facet");++substitutes;
    }));
    routes.push_back(router.Subscribe(router.Key("re_select_command_target"),3u,[&](std::uint32_t p){
        check(word(p)==players.StageActorAt(0)+716u && word(p+4u)==expected_target,"Enter copies original four-word target command");
        check(!control.CurrentActor() && queue.Count()==1u && memory.Count()==1u && actions.Count()==2u,
              "production command queues and stores independent actions before stopping selection");
        bool rejected{};try{control.Close();}catch(const std::logic_error&){rejected=true;}check(rejected,"target control remains pinned through confirmation callback");++confirms;
    }));
    routes.push_back(router.Subscribe(router.Key("re_start_ui_control"),1u,[&](std::uint32_t p){check(p==base+40u && control.CurrentActor(),"Esc dispatches existing command before clearing selection");++cancels;}));
    routes.push_back(router.Subscribe(router.Key("hide_monster_tips"),1u,[&](std::uint32_t){++tips;}));
    check(!run(19u) && repeats==1u,"repeat action original handled result remains false");
    for(auto key:{59u,60u,61u})check(!run(key),"role shortcut original handled result remains false");
    check(substitutes==2u,"only two existing Player formation slots emit substitute");
    check(!run(58u) && !run(0u) && substitutes==2u,"unsupported keys do not synthesize controls");
    std::vector<std::uint32_t> ordered;
    for(std::size_t i=0;i<monsters.StageCount();++i)ordered.push_back(monsters.StageActorAt(i));
    std::sort(ordered.begin(),ordered.end(),[&](auto a,auto b){return monsters.ModelOwner(a).Slot()<monsters.ModelOwner(b).Slot();});
    (void)ActorManagerTargetSelectionController::SwitchActiveActor(base,ordered[0],targets);
    for(const auto [key,expected]:std::array<std::pair<unsigned,unsigned>,4>{{{205u,1u},{200u,2u},{203u,1u},{208u,0u}}})
        check(run(key) && control.CurrentActor()==ordered[expected],"direction key follows original sorted Monster cycle");
    {
        auto& health_state=word(ordered[1]+648u);
        struct StateRestore{std::uint32_t& field;std::uint32_t old;~StateRestore(){field=old;}} state_restore{health_state,health_state};
        health_state=3u;reinterpret_cast<std::uint8_t*>(&system)[80u]=1u;
        check(monsters.Skills(ordered[1]).SecondaryMotionState()==3u && run(205u) && control.CurrentActor()==ordered[2],
              "candidate gate skips actual virtual168 health-state3 Monster");
    }
    const auto first=players.StageActorAt(0),second=players.StageActorAt(1);
    (void)ActorManagerTargetSelectionController::SwitchActiveActor(base,first,targets);
    check(word(first+856u) && word(first+864u),"Player selection uses actual pick and OM03 effect handles");
    check(run(205u) && control.CurrentActor()==second && !word(first+856u) && !word(first+864u),"Player target cycle retires previous effects");
    check(run(203u) && control.CurrentActor()==first,"Player target cycle reverses through actual Player stage iterator");
    word(base+40u)=first+716u;word(base+48u)=0u;word(base+52u)=4u;
    check(run(1u) && cancels==1u && tips==1u && !control.CurrentActor() && !word(base+44u) && !word(base+68u) &&
          !word(first+856u) && !word(first+864u),"Esc restores UI control then stops selection and clears real effects");
    check(run(1u) && run(28u) && cancels==1u && confirms==0u,"no-target Enter/Esc preserve original no-dispatch behavior");
    bool failed{};try{input.Handle(0u);}catch(const std::invalid_argument&){failed=true;}check(failed,"null key event rejected");
    check(!targets.ActiveIterators() && !players.ActiveIterators() && !monsters.ActiveIterators(),"all actual input/candidate cursors released");
    std::cout<<"Native combat Main navigation: actual idle repeat/role events, Player/Monster cycles/effects, state3 filter, Esc/no-target Enter, reentry and zero cursors pass\n";
    (void)ActorManagerTargetSelectionController::SwitchActiveActor(base,ordered[0],targets);
    expected_target=ordered[0];
    check(run(28u) && confirms==1u && !word(base+44u) && memory.Get(first) &&
          !queue.Contains(memory.Get(first)) && word(first+720u)==0u,
          "Enter confirms Player attack, retains distinct replay template and clears real action progress");
    queue.ResetForStart();memory.Clear();source.EndAction(motion);
    check(!queue.Count() && !memory.Count() && !actions.Count(),"input module fixture retires its actual queued and replay actions");
    std::cout<<"Native combat Main confirmation: actual selected target delivered through production command consumers\n";
}
}
