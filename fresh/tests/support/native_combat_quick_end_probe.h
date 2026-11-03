#pragma once
#include "pal4/native_combat_quick_end.h"
#include "pal4/native_combat_actor_exit_events.h"
#include "pal4/native_combat_battle_end.h"
#include "pal4/native_combat_ending_window.h"
#include "../../native/ui/cegui_exports_native.h"
#include "pal4/native_dialog_settings_snapshot.h"
#include <vector>
#include "pal4/native_combat_system_update.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_combat_event_handlers.h"
#include "pal4/native_combat_monster_manager.h"
#include "pal4/native_combat_actor_model_owner.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_task_queue.h"
#include "pal4/native_combat_defend_actions.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_skill_system_services.h"
#include "pal4/random_number_generator.h"
#include "pal4/native_pal_game_iv_texture_storage.h"
#include "pal4/native_combat_console.h"
#include "pal4/native_combat_timed_events.h"
#include <bit>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <string>
namespace pal4::test {
//Consumes Monster health/status payloads immediately before the existing real
//EndCombat clear. It does not restore private skill-list memory by byte copying.
inline void VerifyNativeCombatQuickEnd(NativeCombatQuickEnd& quick,NativeCombatSystemUpdate& frame,
    NativeCombatSystemImage& image,NativeCombatEventHandlers& handlers,NativeCombatMonsterManager& monsters,
    NativeCombatTaskQueue& queue,NativeCombatDefendActions& defend,NativeCombatActorExitEvents& death,std::uint32_t player,
    NativeScriptEventRouter& router,INativeSkillSystemServices& skills,NativePalGameIVTextureStorage& game,
    NativeCombatConsole& console,NativeCombatTimedEvents& timers,NativeDialogSettingsSnapshot& settings,NativeEffectSceneSession& effects,NativeCombatBattleEnd& battle,NativeCombatEndingWindow& ending){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto word=[](std::uint32_t a){return *reinterpret_cast<const std::uint32_t*>(a);};
    const auto state=image.State(),elapsed=image.session_storage[29],wins=game.GameDataEntry(0);
    const auto mode=image.Mode();const auto rng=skills.Random();const auto console_count=console.SubscriptionCount();
    std::uint32_t route{},leave_route{},post_route{};const auto latch=handlers.EndCheckSeen();
    struct LeaveRoute{NativeScriptEventRouter& r;std::uint32_t& token;std::uint32_t& post;NativeCombatEventHandlers& h;bool latch;~LeaveRoute(){if(token)(void)r.Unsubscribe(token);if(post)(void)r.Unsubscribe(post);h.SetEndCheckSeen(latch);}} leave_scope{router,leave_route,post_route,handlers,latch};
    struct Restore{NativeCombatSystemImage& image;NativeScriptEventRouter& router;INativeSkillSystemServices& skills;
        NativePalGameIVTextureStorage& game;NativeCombatConsole& console;NativeCombatTaskQueue& queue;
        std::uint32_t state,elapsed,wins,subscriptions;std::uint8_t mode;RandomNumberGenerator rng;std::uint32_t& route;
        ~Restore(){if(route)(void)router.Unsubscribe(route);try{queue.Clear();game.SetGameDataEntry(0,wins);
            if(subscriptions)console.Subscribe();else console.Unsubscribe();}catch(...){}
            image.State()=state;image.Mode()=mode;image.session_storage[29]=elapsed;skills.Random()=rng;}
    } restore{image,router,skills,game,console,queue,state,elapsed,wins,console_count,mode,rng,route};
    std::vector<std::uint32_t> progress_before;
    const auto head=settings.State().Word(136u);
    for(auto n=settings.FirstNode(settings.State(),132u);n!=head;n=settings.NextNode(n,false))progress_before.push_back(settings.NodeValue(n));
    struct RestoreProgress{NativeDialogSettingsSnapshot& s;std::vector<std::uint32_t> before;
        ~RestoreProgress(){s.ClearTree(s.State(),132u);for(auto key:before)s.InsertTreeValue(s.State(),132u,key);}
    } restore_progress{settings,progress_before};
    const auto count=monsters.Count(),events=timers.EventCount(),allocations=timers.OutstandingAllocations();
    const auto reports_before=battle.PreparedReports(),copies_before=battle.CopiedReports(),processed_before=battle.ProcessedReports();
    check(count && !queue.Count() && !defend.Count(),"QuickEnd needs actual populated Monster first roster and idle tasks");
    const auto first_actor=monsters.ActorAt(0u),first_slot=monsters.ModelOwner(first_actor).Slot();const auto stage_before=monsters.StageCount();
    bool outside_rejected{};try{(void)monsters.RetireActorDuringExit(first_actor,effects);}catch(const std::logic_error&){outside_rejected=true;}
    check(outside_rejected && monsters.ModelOwner(first_actor).Slot()==first_slot && monsters.StageCount()==stage_before,
        "source-only nested retirement rejects outside original544 without changing payload/stage");
    for(auto s:{0u,1u,6u,7u}){
        image.State()=s;(void)router.Dispatch("quick_end",0u);
        check(image.State()==s && game.GameDataEntry(0)==wins && timers.EventCount()==events && !queue.Count(),
            "real QuickEnd inactive-state gate ignores null payload and preserves outcome/queue");
    }
    bool foreign{};try{quick.Handle(handlers.ExtendedAddress(),0u);}catch(const std::invalid_argument&){foreign=true;}
    check(foreign,"QuickEnd accepts only actual all-handler owner");
    const auto task=defend.Create(player);check(task && defend.Owns(task),"QuickEnd uses a real GOM Defend task");
    (void)queue.Register(task);check(queue.Count()==1u,"actual task is queued before quick reset");
    unsigned changes{},guards{};
    route=router.Subscribe(router.Key("me_attr_int_ext_changed"),1u,[&](std::uint32_t){
        check(!queue.Count() && !defend.Owns(task),"real task reset/cleanup precedes Monster HP mutation");++changes;
        for(unsigned which=0;which<3u;++which){bool rejected{};try{
            if(which==0u)quick.Close();
            if(which==1u)quick.Handle(handlers.AllAddress(),0u);
            if(which==2u)frame.Advance(.001f,0u);
        }catch(const std::logic_error&){rejected=true;}check(rejected,"quick handler and whole frame pinned during actor callbacks");++guards;}
    });
    bool fail_leave=true;unsigned leaves{},death_guards{};
    leave_route=router.Subscribe(router.Key("me_leave_stage"),1u,[&](std::uint32_t p){
        ++leaves;
        bool rejected{};try{death.Close();}catch(const std::logic_error&){rejected=true;}check(rejected,"native death consumer pinned through actual stage retirement");++death_guards;
        rejected=false;try{death.Handle(handlers.AllAddress(),p);}catch(const std::logic_error&){rejected=true;}check(rejected,"native death consumer rejects nested event");++death_guards;
        rejected=false;try{(void)monsters.RetireActor(word(p)-716u,effects);}catch(const std::logic_error&){rejected=true;}
        check(rejected,"ordinary retirement remains prohibited during actor/manager dispatch");++death_guards;
        if(fail_leave)throw std::runtime_error("intentional native death leave observer failure");
    });
    unsigned stage_guards{};
    post_route=router.Subscribe(router.Key("ae_become_dead"),1u,[&](std::uint32_t p){
        const auto a=word(p)-716u,slot=monsters.ModelOwner(a).Slot();const auto stage=monsters.StageCount();
        const auto cursor=monsters.CreateStageIterator();bool rejected{};
        try{(void)monsters.RetireActorDuringExit(a,effects);}catch(const std::logic_error&){rejected=true;}
        monsters.DeleteStageIterator(cursor);
        check(rejected && monsters.ModelOwner(a).Slot()==slot && monsters.StageCount()==stage,
            "actual544 context still rejects payload retirement under a live stage iterator");++stage_guards;
    });
    std::uint32_t seed{};for(;seed<1000u;++seed){RandomNumberGenerator candidate;candidate.Seed(seed);if(candidate.Next()%1000u<500u)break;}
    check(seed<1000u,"find original RNG direct quick-end bucket fixture");
    const auto prepare=[&]{image.State()=2u;image.Mode()=1u;image.session_storage[29]=std::bit_cast<std::uint32_t>(20.f);skills.Random().Seed(seed);};
    prepare();bool failed{};
    try{frame.Advance(.001f,0u);}catch(const std::runtime_error& e){failed=std::string_view(e.what())=="intentional native death leave observer failure";}
    check(failed && leaves==1u && !monsters.ActiveIterators() && !queue.Count() && !defend.Owns(task),
        "death leave observer exception releases actual first-roster/actor permits before retry");
    fail_leave=false;prepare();bool rewards{};std::string reward_error;
    try{frame.Advance(.001f,0u);}catch(const std::logic_error& e){reward_error=e.what();rewards=reward_error=="Combat ending task sequence services are not bound";}
    if(!rewards)throw std::runtime_error("native death retry observed: "+reward_error);
    const auto visible=detail::Export<bool (__thiscall*)(const void*,bool)>(GetModuleHandleW(L"CEGUIBase.dll"),"?isVisible@Window@CEGUI@@QBE_N_N@Z");
    check(visible(reinterpret_cast<void*>(ending.Root()),false) && router.Contains(ending.Word(56u)) && !ending.TaskCount(),
        "real576720 shows constructed ending frame and reaches native combat_prize subscriber before ending-task guard");
    check(battle.PreparedReports()==reports_before+1u && battle.CopiedReports()==copies_before+1u && battle.ProcessedReports()==processed_before+1u && !battle.PendingReports(),
        "real BattleEnd prepares/deep-copies report and applies actual rewards and releases all reports on downstream ending-UI failure");
    check(rewards && !monsters.ActiveIterators() && !queue.Count() && !defend.Owns(task),
        "actual death consumer continues all Monsters and reaches real victory-report boundary");
    for(std::size_t i=0;i<count;++i){const auto a=monsters.ActorAt(i);
        check(monsters.Skills(a).CurrentStat(7u)==0 && word(a+828u)==4u && monsters.ModelOwner(a).Slot()==6u,
            "actual dead Monsters retain owners but retire original payload/stage slots");
        if(static_cast<std::int32_t>(word(a+964u))>0)
            check(settings.HasMonsterProgress(std::bit_cast<std::int32_t>(word(a+960u))),"real Monster death records progress in existing settings tree132");}

    check(monsters.StageCount()==0u && changes && guards==changes*3u && death_guards==leaves*3u && stage_guards,
        "actual first-roster owner pins coexist with source-only stage retirement and guards");
    check(image.State()==6u && game.GameDataEntry(0)==wins+1u && !console.SubscriptionCount() &&
        timers.EventCount()==events && timers.OutstandingAllocations()==allocations,
        "real death/QuickEnd outcome1 updates ending/counter without fabricating rewards or extra callback");
    image.State()=6u;quick.Handle(handlers.AllAddress(),0u);
    check(monsters.Count()==count && !monsters.ActiveIterators(),"QuickEnd keeps actual actor owners for subsequent original EndCombat");
    std::cout<<"Native QuickEnd/Death57B220: actual575F40 RNG -> GOM queue/HP7/actor544, first-roster source-only388 stage retirement, leave-observer exception retry, all Monster owners="<<count
        <<" stage=0, actual readiness/outcome1/state6/victory counter; real report/rewards complete; real576720 shows ending frame and dispatches prize; stops at ending-task boundary, no fabricated victory success\n";
}
}
