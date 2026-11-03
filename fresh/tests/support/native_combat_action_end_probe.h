#pragma once
#include "pal4/native_combat_action_end.h"
#include "pal4/native_combat_player_manager.h"
#include "pal4/native_combat_monster_manager.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_player_motion.h"
#include "pal4/native_combat_event_handlers.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_script_event_router.h"
#include <array>
#include <vector>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeCombatActionEnd(NativeCombatActionEnd& end,NativeCombatPlayerManager& players,
    NativeCombatMonsterManager& monsters,std::span<NativeCombatActorSkillRuntime* const> player_skills,
    NativeCombatEventHandlers& handlers,NativeCombatSystemImage& system,NativeScriptEventRouter& router,
    NativeCombatPlayerMotion& player_motion){
    const auto check=[](bool b,const char* why){if(!b)throw std::runtime_error(why);};
    const auto reject=[&](auto&& f,const char* why){bool denied{};try{f();}catch(const std::logic_error&){denied=true;}check(denied,why);};
    const auto word=[](std::uint32_t a)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(a);};
    check(players.Count()==2u && monsters.Count()==3u && player_skills.size()==2u,"actual end-check rosters available");
    struct Saved {NativeCombatActorSkillRuntime* skills;std::uint32_t facet,health,action;float progress;};
    struct Restore {
        std::vector<Saved> actors;NativeCombatSystemImage& system;NativeCombatEventHandlers& handlers;
        std::uint32_t state;bool seen;
        ~Restore(){for(const auto& r:actors){const auto a=r.skills->ActorAddress();
            *reinterpret_cast<std::uint32_t*>(a+828u)=r.facet;*reinterpret_cast<std::uint32_t*>(a+648u)=r.health;
            *reinterpret_cast<std::uint32_t*>(a+660u)=r.action;try{r.skills->SetActionProgress(r.progress);}catch(...){}}
            system.State()=state;handlers.SetEndCheckSeen(seen);}
    } restore{{},system,handlers,system.State(),handlers.EndCheckSeen()};
    for(auto* skills:player_skills)restore.actors.push_back({skills,word(skills->ActorAddress()+828u),word(skills->ActorAddress()+648u),word(skills->ActorAddress()+660u),skills->ActionProgress()});
    for(std::size_t i=0;i<monsters.Count();++i){auto& skills=monsters.Skills(monsters.ActorAt(i));const auto a=skills.ActorAddress();
        restore.actors.push_back({&skills,word(a+828u),word(a+648u),word(a+660u),skills.ActionProgress()});}
    const auto set_all=[&](std::uint32_t facet,std::uint32_t health){for(const auto& r:restore.actors){const auto a=r.skills->ActorAddress();word(a+828u)=facet;word(a+648u)=health;word(a+660u)=0u;}};
    const auto balanced=[&]{return !end.ActiveIterators() && !players.ActiveIterators() && !monsters.ActiveIterators();};
    auto& player=*player_skills.front();auto& monster=monsters.Skills(monsters.ActorAt(0));
    set_all(1u,0u);handlers.SetEndCheckSeen(false);system.State()=5u;
    check(end.CheckOutcome()==3u && balanced(),"actual full rosters are ongoing");
    unsigned progress_calls{},end_calls{};bool formal{};
    struct Routes {NativeScriptEventRouter& router;std::vector<std::uint32_t> handles;~Routes(){for(auto h:handles)(void)router.Unsubscribe(h);}} routes{router};
    for(const auto name:{"re_moved_at_action_sequence","me_moved_at_action_sequence"})
        routes.handles.push_back(router.Subscribe(router.Key(name),3u,[&](std::uint32_t){
            if(!formal)return;++progress_calls;
            reject([&]{end.Close();},"formal action-end consumer cannot retire in progress callback");
            reject([&]{end.CheckOutcome();},"formal action-end reentry rejected");
            reject([&]{handlers.Close();},"original handler owner cannot retire during end dispatch");
        }));
    routes.handles.push_back(router.Subscribe(router.Key("ae_end_action"),3u,[&](std::uint32_t){++end_calls;}));
    for(auto* skills:{&player,&monster}){
        const auto a=skills->ActorAddress();skills->SetActionProgress(12.f);word(a+660u)=1u;
        skills->StartAction();formal=true;
        skills->EndAction([&](std::uint32_t actor,std::uint32_t flag){
            if(actor==player.ActorAddress())player_motion.Update(actor,flag);else monsters.UpdateMonsterMotion(actor,flag);
        });formal=false;
        check(skills->ActionProgress()==32.f && !handlers.EndCheckSeen() && balanced(),
            "formal Player/Monster lifecycle routes through actual57B5E0 and ongoing result");
    }
    check(end_calls==2u && progress_calls==2u,"formal lifecycle emits once and real end consumer updates progress once");
    // Winning/losing publication uses explicit outcome-consumer fixtures so the
    // still-incomplete formal outcome/Scene pipeline is not reported complete.
    NativeScriptEventRouter fixture_router;NativeCombatSystemImage fixture_system;
    std::vector<std::uint32_t> outcomes;bool inspect{},fail_publish{};NativeCombatActionEnd* running{};
    NativeCombatEventHandlers fixture_handlers(fixture_router,fixture_system.ActorHandlersSlot(),fixture_system.ExtendedHandlersSlot(),
        [&](std::uint32_t,std::uint32_t entry,std::uint32_t payload){
            check(entry==0x57a920u || entry==0x57c6b0u,"fixture only consumes original outcome routes");
            if(inspect)reject([&]{running->Close();},"outcome callback cannot retire consumer");
            if(fail_publish)throw std::logic_error("intentional outcome consumer failure");
            outcomes.push_back(word(payload));
        });
    NativeCombatActionEnd fixture(players,monsters,player_skills,fixture_handlers,fixture_system,fixture_router);running=&fixture;
    fixture_system.State()=5u;set_all(1u,0u);
    const std::uint32_t facet=player.ActorAddress()+716u;
    const auto invoke=[&]{return fixture.Handle(fixture_handlers.AllAddress(),reinterpret_cast<std::uint32_t>(&facet));};
    word(player.ActorAddress()+660u)=1u;player.SetActionProgress(12.f);fixture_system.State()=1u;
    check(invoke()==3u && player.ActionProgress()==12.f,"inactive gate skips opposite scan/progress while preserving roster outcome check");
    fixture_system.State()=5u;
    for(std::size_t i=0;i<monsters.Count();++i){const auto a=monsters.ActorAt(i);word(a+828u)=0u;word(a+648u)=3u;}
    check(invoke()==3u && player.ActionProgress()==12.f,"opposite health3 skipped while state-zero keeps roster present");
    for(std::size_t i=0;i<monsters.Count();++i)word(monsters.ActorAt(i)+828u)=4u;
    check(fixture.CheckOutcome()==1u && invoke()==1u && fixture_handlers.EndCheckSeen() && outcomes.empty(),"first winning check sets actual owner136 without publishing");
    inspect=true;(void)invoke();inspect=false;check(outcomes==std::vector<std::uint32_t>{1u,1u},"second winning check publishes one-word result1 to both original outcome routes");
    set_all(1u,0u);check(invoke()==3u && fixture_handlers.EndCheckSeen(),"ongoing result retains original sticky owner flag");
    for(std::size_t i=0;i<players.Count();++i)word(players.ActorAt(i)+828u)=4u;
    fixture_handlers.SetEndCheckSeen(false);
    check(fixture.CheckOutcome()==0u && invoke()==0u && fixture_handlers.EndCheckSeen(),"first losing check sets flag");
    fail_publish=true;reject([&]{invoke();},"outcome failure propagates");fail_publish=false;
    (void)invoke();check(outcomes==std::vector<std::uint32_t>{1u,1u,0u,0u},"result0 uses an actual payload pointer and failure releases guard");
    set_all(1u,0u);
    NativeCombatActionEnd missing(players,monsters,{},fixture_handlers,fixture_system,fixture_router);
    reject([&]{missing.CheckOutcome();},"missing Player services reject during full-roster scan");
    check(!missing.ActiveIterators() && balanced(),"query failure releases actual first-list cursor");missing.Close();
    reject([&]{fixture.Handle(0u,reinterpret_cast<std::uint32_t>(&facet));},"foreign callback owner rejected");
    reject([&]{fixture.Handle(fixture_handlers.AllAddress(),0u);},"null facet rejected");
    const std::uint32_t foreign=0x1000u;
    reject([&]{fixture.Handle(fixture_handlers.AllAddress(),reinterpret_cast<std::uint32_t>(&foreign));},"foreign actor rejected");
    check(!fixture.ActiveIterators() && balanced(),"all outcome/invalid branches release cursors");fixture.Close();
    reject([&]{invoke();},"retired end consumer rejects calls");
    std::cout<<"Native combat action end: formal Player/Monster lifecycle, actual full-roster queries/progress, outcome flag/payload fixtures, gates and retirement/failure cleanup pass\n";
}
}
