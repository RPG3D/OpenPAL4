#pragma once
#include "pal4/native_combat_player_manager.h"
#include "pal4/native_combat_player_actor_owner.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_skill_system_services.h"
#include "pal4/native_player_manager_session.h"
#include <array>
#include <bit>
#include <cstring>
#include <iostream>
namespace pal4::test {
inline void VerifyNativeCombatPlayerManager(NativePlayerManagerSession& source,INativeSkillSystemServices& system) {
    const auto check=[](bool ok,const char* m){if(!ok)throw std::runtime_error(m);};
    const auto reject=[&](auto&& call){bool rejected=false;try{call();}catch(const std::logic_error&){rejected=true;}check(rejected,"Player manager guard");};
    const auto word=[](std::uint32_t a){std::uint32_t v;std::memcpy(&v,reinterpret_cast<const void*>(a),4u);return v;};
    const auto write=[](std::uint32_t a,std::uint32_t v){std::memcpy(reinterpret_cast<void*>(a),&v,4u);};
    const auto player=[&](unsigned i){return source.Address()+i*PlayerManagerObject::kElementBytes;};
    struct Restore {
        std::array<std::array<std::uint32_t,4>,4> values;
        ~Restore(){for(const auto& v:values){std::memcpy(reinterpret_cast<void*>(v[0]+0xb08u),&v[1],4u);std::memcpy(reinterpret_cast<void*>(v[0]+0x8f0u),&v[2],4u);std::memcpy(reinterpret_cast<void*>(v[0]+0x7a0u),&v[3],4u);}}
    } restore;
    for(unsigned i=0;i<4u;++i)restore.values[i]={player(i),word(player(i)+0xb08u),word(player(i)+0x8f0u),word(player(i)+0x7a0u)};
    NativeScriptEventRouter router;
    unsigned observed{};bool throw_observer{};NativeCombatPlayerManager* current{};
    const auto subscription=router.Subscribe(router.Key("re_new_added"),1u,[&](std::uint32_t payload){
        const auto actor=word(payload)-716u;
        check(current && current->ActorAt(current->Count()-1u)==actor,"Player insertion precedes actual re_new_added");
        auto& owner=current->Actor(actor);
        check(owner.Name()==owner.BaseName() && word(actor+332u)==owner.Slot(),"Player name and mirrored property124 precede event");
        reject([&]{current->Close();});reject([&]{current->AddPlayer(0u);});reject([&]{current->CreateActiveParty();});
        reject([&]{owner.Destroy();});reject([&]{owner.SetName("recursive");});reject([&]{owner.SetSlot(9u);});
        if(current->Count()>1u)reject([&]{current->Actor(current->ActorAt(0u)).Destroy();});
        ++observed;if(throw_observer)throw std::logic_error("intentional Player new-actor observer failure");
    });
    struct Observer {NativeScriptEventRouter& r;std::uint32_t t;~Observer(){(void)r.Unsubscribe(t);}} observer{router,subscription};
    // Actual current NewGame membership is exercised separately by the
    // persistent application's prerequisite. Here cover all queue branches.
    for(unsigned i=0;i<4u;++i)write(player(i)+0xb08u,0u);
    {
        NativeCombatPlayerManager empty(source,router,system);current=&empty;
        check(empty.SubscriptionCount()==3u,"Player manager actual three subscriptions");
        empty.CreateActiveParty();check(!empty.Count() && !empty.StageCount(),"empty party is empty without fabricated actors");empty.Close();
    }
    {
        write(player(2u)+0xb08u,1u);write(player(2u)+0x8f0u,7u);
        NativeCombatPlayerManager single(source,router,system);current=&single;single.CreateActiveParty();
        check(single.Count()==1u && single.Actor(single.ActorAt(0u)).Slot()==1u &&
            word(single.ActorAt(0u)+1192u)==word(player(2u)+2632u),"single party overrides source priority with original argument1");
        single.Close();check(!single.SubscriptionCount(),"Player subscriptions retire");
    }
    {
        for(unsigned i=0;i<4u;++i)write(player(i)+0xb08u,i<3u?1u:0u);
        write(player(0u)+0x8f0u,4u);write(player(1u)+0x8f0u,0xffffffffu);write(player(2u)+0x8f0u,2u);
        NativeCombatPlayerManager multi(source,router,system);current=&multi;multi.CreateActiveParty();
        const std::array<unsigned,3> order{1u,2u,0u};check(multi.Count()==3u,"three active source slots");
        for(unsigned i=0;i<3u;++i) {
            const auto a=multi.ActorAt(i),s=player(order[i]);
            check(word(a+1192u)==word(s+2632u) && multi.Actor(a).Slot()==word(s+0x8f0u),"signed4419D0 order and argument6 source state copied");
        }
        reject([&]{multi.CreateActiveParty();});
        reject([&]{multi.Actor(multi.ActorAt(0u)).Destroy();});
        const auto emit=[&](const char* event,std::uint32_t a){const std::array payload{a+716u};(void)router.Dispatch(event,reinterpret_cast<std::uint32_t>(payload.data()));};
        const auto first=multi.ActorAt(0u),second=multi.ActorAt(1u);
        emit("re_join_stage",first);emit("re_join_stage",second);emit("re_join_stage",first);
        check(multi.StageCount()==3u && multi.StageActorAt(2u)==first,"actual join callback preserves duplicate append");
        const auto cursor=multi.CreateStageIterator();multi.NextStageIterator(cursor);const auto clone=multi.CloneStageIterator(cursor);
        check(multi.StageIteratorActor(clone)==second,"Player cursor clone preserves current actor");
        reject([&]{emit("re_leave_stage",first);});reject([&]{multi.Close();});reject([&]{multi.AddPlayer(3u);});
        multi.DeleteStageIterator(cursor);multi.DeleteStageIterator(clone);
        reject([&]{(void)multi.StageIteratorActor(cursor);});
        emit("re_leave_stage",first);check(multi.StageCount()==2u && multi.StageActorAt(0u)==second && multi.StageActorAt(1u)==first,"leave callback removes first duplicate only");
        const auto before=multi.Count();throw_observer=true;reject([&]{multi.AddPlayer(3u,2u);});throw_observer=false;
        check(multi.Count()==before+1u && multi.Actor(multi.ActorAt(before)).Slot()==2u,"throwing event retains inserted Player and preceding state");
        const auto bound=multi.ActorAt(1u);write(bound+808u,1u);reject([&]{multi.Close();});write(bound+808u,0u);
        check(multi.Count()==4u && multi.SubscriptionCount()==3u,"Close preflights every actor before changing manager");
        (void)router.Dispatch("combat_over",0u);check(multi.Count()==4u,"original combat_over callback is RET without fake cleanup");
        multi.Close();multi.Close();check(!multi.Count() && !multi.StageCount() && !multi.ActiveIterators(),"native Player manager teardown");
    }
    current=nullptr;check(observed==5u,"actual Player creation event count");
    std::cout<<"Native53B0D0 Player manager: shared creation/name/mirrored slot/event order, active empty/single/multi queue, signed sort, 5 new events, real stage subscriptions/duplicate removal/cursor guards, throw recovery and native teardown pass\n";
}
}
