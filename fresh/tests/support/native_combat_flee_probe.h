#pragma once
#include "native_combat_defend_probe.h"
#include "pal4/native_combat_flee_actions.h"
#include "pal4/native_combat_command_actions.h"
#include "pal4/native_combat_player_manager.h"
#include "pal4/native_combat_monster_manager.h"
#include "pal4/native_combat_roster_targets.h"
#include "pal4/native_skill_system_services.h"
#include "pal4/random_number_generator.h"
#include "pal4/combat_flee_sequence_task.h"
#include <set>
namespace pal4::test {
inline void VerifyNativeCombatFlee(NativeCombatFleeActions& flee,NativeCombatCommandActions& command,
    NativeCombatPlayerActionMemory& memory,NativeCombatTaskQueue& queue,NativeCombatPlayerManager& players,
    NativeCombatMonsterManager& monsters,NativeCombatRosterTargets& targets,NativeCombatActionWindow& window,
    NativeCombatActionFrame& frame,NativeCombatMainSession& main,NativeCombatMainWindow& main_window,
    NativeCombatActorSkillRuntime& source,INativeCombatActorTurnServices& turn,NativeCombatEventHandlers& handlers,
    NativeEventHandlerArray& active,NativeScriptEventRouter& router,NativeCombatSystemImage& image,INativeSkillSystemServices& system,
    const std::function<void()>& dismiss_tutorial,const std::function<void(float)>& advance,
    const std::function<std::string(std::uint32_t)>& animation,const NativeCombatFleeActions::MotionRefresh& motion,
    const std::function<void()>& verify_result){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto word=[](std::uint32_t p)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(p);};
    const auto a=source.ActorAddress(),last=active.GetLast(),activity_count=active.Size(),subscriptions=router.SubscriptionCount();
    auto& blocked=*reinterpret_cast<std::uint8_t*>(reinterpret_cast<std::uint32_t>(&image)+501u);
    check(!queue.Count() && !memory.Count() && !flee.Count() && !flee.ChildCount(),"Flee requires idle production owners");
    struct Restore {
        NativeCombatTaskQueue& queue;NativeCombatPlayerActionMemory& memory;NativeCombatActionFrame& frame;NativeCombatMainSession& main;
        NativeCombatEventHandlers& handlers;NativeCombatSystemImage& image;NativeScriptEventRouter& router;
        RandomNumberGenerator& rng;RandomNumberGenerator saved;NativeCombatFleeActions::MotionRefresh motion;
        std::uint32_t state,ready_actor,control_actor;bool started;std::uint8_t mode;std::uint8_t& blocked;std::uint8_t saved_blocked;
        std::vector<std::pair<std::uint32_t,std::uint32_t>> fields;std::vector<std::uint32_t> routes,actors;
        ~Restore(){for(auto token:routes)(void)router.Unsubscribe(token);try{queue.ResetForStart();memory.Clear();frame.SetVisible(false);main.SetVisible(false);}catch(...){}
            for(auto [p,v]:fields)*reinterpret_cast<std::uint32_t*>(p)=v;
            image.State()=state;image.Mode()=mode;blocked=saved_blocked;rng=saved;
            handlers.SetReadyActor(ready_actor);handlers.SetControlActor(control_actor);handlers.SetUiControlStarted(started);
            for(auto actor:actors)try{motion(actor,1u);}catch(...){}
        }
        void Save(std::uint32_t p){fields.emplace_back(p,*reinterpret_cast<std::uint32_t*>(p));}
    } restore{queue,memory,frame,main,handlers,image,router,system.Random(),system.Random(),motion,
        image.State(),handlers.ReadyActor(),handlers.ControlActor(),handlers.UiControlStarted(),image.Mode(),blocked,blocked};
    restore.Save(reinterpret_cast<std::uint32_t>(&image)+504u);
    for(std::size_t i=0;i<players.Count();++i)restore.actors.push_back(players.ActorAt(i));
    for(std::size_t i=0;i<monsters.Count();++i){const auto actor=monsters.ActorAt(i);restore.actors.push_back(actor);restore.Save(actor+1144u);word(actor+1144u)=0u;}
    for(auto actor:restore.actors){for(unsigned o=340u;o<676u;o+=4u)restore.Save(actor+o);
        for(auto o:{720u,724u,828u,832u})restore.Save(actor+o);}
    for(auto o:{60u,88u,0x1e4u})restore.Save(window.Address()+o);
    for(auto o:{268u,272u})restore.Save(main_window.Address()+o);
    main.SetVisible(true);frame.SetVisible(true,main_window.Root());
    unsigned commands{},begins{},failures{},fled{},ends{};bool throw_begin{};
    restore.routes.push_back(router.Subscribe(router.Key("re_select_command"),3u,[&](std::uint32_t p){
        check(word(p)==a+716u && word(p+12u)==3u,"actual Flee UI emits original command3");++commands;
    }));
    restore.routes.push_back(router.Subscribe(router.Key("ae_fleeing"),1u,[&](std::uint32_t p){
        check(word(p)==a+716u && word(a+828u)==5u,"Flee child invokes original actor528 state5 before event");++begins;
        bool rejected{};try{flee.Close();}catch(const std::logic_error&){rejected=true;}check(rejected,"Flee domain pinned during event");
        rejected=false;try{source.Close();}catch(const std::logic_error&){rejected=true;}check(rejected,"Flee registry pinned during event");
        if(throw_begin)throw std::runtime_error("intentional Flee begin observer failure");
    }));
    restore.routes.push_back(router.Subscribe(router.Key("ae_flee_fail"),1u,[&](std::uint32_t p){
        check(word(p)==a+716u && word(a+828u)==2u,"Flee failure invokes actual536 state2 before event");++failures;
    }));
    restore.routes.push_back(router.Subscribe(router.Key("ae_fleed"),1u,[&](std::uint32_t p){
        check(word(p)==a+716u && word(a+828u)==6u,"successful Flee invokes actual532 state6 before outcome consumer");++fled;
    }));
    restore.routes.push_back(router.Subscribe(router.Key("ae_end_action"),3u,[&](std::uint32_t p){if(word(p)==a+716u)++ends;}));
    const auto module=GetModuleHandleW(L"CEGUIBase.dll");
    unsigned animation_failures{},success_animations{},natural_drains{},success_guard{};
    for(unsigned mode=0;mode<5u;++mode){
        for(std::size_t i=0;i<players.Count();++i){const auto actor=players.ActorAt(i);word(actor+828u)=1u;word(actor+636u)=word(actor+648u)=word(actor+660u)=0u;}
        for(std::size_t i=0;i<monsters.Count();++i)word(monsters.ActorAt(i)+368u)=1u;
        word(a+368u)=100000u;image.Mode()=0u;image.State()=3u;blocked=(mode==4u || mode==3u)?0u:1u;
        for(std::size_t i=0;i<monsters.Count();++i)word(monsters.ActorAt(i)+1144u)=mode==3u && i==0u?1u:0u;
        if(mode!=3u){source.SetActionProgress(.75f);source.CanTakeAction(turn);dismiss_tutorial();
            check(window.Word(88u)==a && active.GetLast()==window.Address()+40u,"real Player ready activates Flee input");}
        else image.Mode()=1u;
        auto expected_rng=system.Random();(void)expected_rng.Next();
        if(mode==1u){CeguiMouseEvent event{};event.vtable=reinterpret_cast<std::uint32_t>(detail::Export<const void*>(module,"??_7MouseEventArgs@CEGUI@@6B@"));event.window=window.Button(5u);
            detail::NativeString empty(module,"");detail::Export<void (__thiscall*)(void*,const void*,void*,const void*)>(module,"?fireEvent@EventSet@CEGUI@@UAEXABVString@2@AAVEventArgs@2@0@Z")(
                reinterpret_cast<void*>(event.window),detail::Export<const void*>(module,"?EventClicked@PushButton@CEGUI@@2VString@2@B"),&event,empty.Data());window.RethrowEventError();
        }else if(mode==3u){const std::array request{a+716u,0u,0u,3u};(void)command.Handle(handlers.ExtendedAddress(),reinterpret_cast<std::uint32_t>(request.data()));}
        else{std::array<std::uint32_t,5> event{};event[4]=7u;(void)active.DispatchKeyDown(active.GetLast(),reinterpret_cast<std::uint32_t>(event.data()));}
        check(system.Random().Index()==expected_rng.Index(),"actual Flee chance consumes one real random value; cold replay consumes none");
        for(unsigned i=0;i<RandomNumberGenerator::kStateCount;++i)check(system.Random().StateAt(i)==expected_rng.StateAt(i),"Flee RNG retains full original state transition");
        const auto saved=memory.Get(a);
        check(queue.Count()==1u && memory.Count()==1u && flee.Count()==2u && saved && !queue.Contains(saved) &&
            word(saved)==(mode==3u?QuickFleeSequenceTask::kVtable:FleeSequenceTask::kVtable) && !flee.ActiveIterators() && !targets.ActiveIterators(),
            "Flee owns distinct queue/replay parents with real Quick mode and no leaked selection cursors");
        const auto begin_count=begins;throw_begin=mode==2u;bool expected_failure{},saw_z18{};std::set<std::uint32_t> z17;
        unsigned frames{};
        try{for(;frames<600u && queue.Count();++frames){advance(1.f/60.f);saw_z18|=_stricmp(animation(a).c_str(),"z18")==0;
            if(mode==4u)for(std::size_t i=0;i<players.StageCount();++i){const auto actor=players.StageActorAt(i);if(_stricmp(animation(actor).c_str(),"z17")==0)z17.insert(actor);}
        }}catch(const std::exception& e){
            const auto text=std::string_view(e.what());
            if(mode==2u && text.find("intentional Flee")!=std::string_view::npos)expected_failure=true;
            else throw;
        }
        throw_begin=false;
        std::cout<<"Native Flee observation: mode="<<mode<<" frames="<<frames<<" current_animation="<<animation(a)
            <<" z18="<<saw_z18<<" z17_players="<<z17.size()<<" boundary="<<expected_failure<<'\n';
        if(mode==2u){check(expected_failure && begins==begin_count+1u && word(a+828u)==5u && queue.Count()==1u,"Flee observer failure preserves owned unfinished task");queue.ResetForStart();}
        else if(mode==4u){check(!expected_failure && !queue.Count() && frames>=120u && !z17.empty() && z17.size()==players.StageCount() && word(a+828u)==6u && fled==1u && image.State()==6u,
            "actual successful Flee reaches57B710 combat_over2 and57A920 state6 before natural task retirement");
            success_animations=static_cast<unsigned>(z17.size());verify_result();++success_guard;}
        else{check(!queue.Count() && frames<600u && word(a+828u)==1u,"failed Flee completes actual536/EndAction then natural queue drainage");++natural_drains;
            if(mode!=3u){check(saw_z18,"normal failed Flee waits for actual z18 animation");++animation_failures;}}
        check(flee.Count()==1u && memory.Get(a)==saved && !flee.ChildCount(),"Flee queue cleanup retains independent cold replay template only");
        memory.Clear();check(!flee.Count() && !flee.OwnedAllocations() && !flee.OwnedTexts() && !flee.ActiveIterators(),"Flee child/text/parent/template ownership fully retires");
        std::cout<<"Native Flee route: mode="<<mode<<" frames="<<frames<<" state="<<word(a+828u)<<" z18="<<saw_z18<<" z17="<<z17.size()<<" expected_boundary="<<expected_failure<<'\n';
    }
    check(commands==4u && begins==5u && failures==3u && ends==4u && natural_drains==3u && animation_failures==2u && success_guard==1u,
        "Flee keyboard/DLL/Quick/failure-retry and successful result dispatch remain exact");
    for(auto token:restore.routes)(void)router.Unsubscribe(token);restore.routes.clear();frame.SetVisible(false);main.SetVisible(false);
    check(active.GetLast()==last && active.Size()==activity_count && router.SubscriptionCount()==subscriptions && !players.ActiveIterators() && !monsters.ActiveIterators(),
        "Flee restores frame subscriptions, global input and first/stage cursor ownership");
    std::cout<<"Native Flee: actual numeric6/DLL command3, probability/RNG/blocked501 and opposite roster, z18 failures2 + Quick failure1 natural drains, status observer cleanup/retry, ready z17 Players="
             <<success_animations<<" actual57B710/combat_over2/57A920/count/console unsubscribe/deferred576440; deferred EndSequence executes after scoped actor references unwind\n";
}
}
