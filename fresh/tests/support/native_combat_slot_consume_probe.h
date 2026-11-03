#pragma once
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_actor_owner.h"
#include "pal4/native_combat_player_actor_owner.h"
#include "pal4/native_combat_skill_actor_registry.h"
#include "pal4/native_combat_inventory_events.h"
#include "pal4/native_inventory_property_reader.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_monster_record_catalog.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_skill_record_catalog.h"
#include "pal4/native_skill_system_services.h"
#include "pal4/random_number_generator.h"
#include <array>
#include <bit>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace pal4::test {
struct OriginalSlotConsume {
    unsigned id{},money_id{};float gate{},money_gate{};
    explicit OriginalSlotConsume(const NativeSkillRecordCatalog& catalog){
        for(std::size_t i=0;i<catalog.Size();++i){const auto& row=catalog.At(i);
            const auto g=std::bit_cast<float>(row[5]);
            if(row[4]!=9107u || !row[14] || !std::isfinite(g) || g<=0.f || g>=1.f)continue;
            const auto* next=reinterpret_cast<const unsigned*>(catalog.Address(row[14]));
            const auto m=std::bit_cast<float>(next[5]);
            if(next[4]==9108u && !next[14] && std::isfinite(m) && m>0.f && m<1.f){id=row[0];gate=g;money_id=next[0];money_gate=m;break;}
        }
        if(!id)throw std::runtime_error("original9107 to terminal9108 probability resources");
    }
    unsigned Seed(bool success,bool money=false)const{
        for(unsigned seed=1;seed<10000;++seed){RandomNumberGenerator random;random.Seed(seed);
            if((double(random.Next()%1000u)*double(.001f)<=(money?money_gate:gate))==success)return seed;
        }
        throw std::runtime_error("bounded9107 branch seed");
    }
};
inline void CheckSlotConsumeRng(const RandomNumberGenerator& actual,const RandomNumberGenerator& expected){
    if(actual.Index()!=expected.Index())throw std::runtime_error("loot callbacks consume exact expected random draws");
    for(unsigned i=0;i<RandomNumberGenerator::kStateCount;++i)
        if(actual.StateAt(i)!=expected.StateAt(i))throw std::runtime_error("9107 preserves complete expected RNG state");
}
inline void VerifyNativeMonsterSlotConsume(NativeCombatActorSkillRuntime& skills,
    const NativeSkillRecordCatalog& catalog,INativeSkillSystemServices& system){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto word=[](unsigned a)->unsigned&{return *reinterpret_cast<unsigned*>(a);};
    const auto actor=skills.ActorAddress();const OriginalSlotConsume record(catalog);
    const auto rng=system.Random();
    const std::array<unsigned,8> offsets{544,548,552,208,212,216,708,712};std::array<unsigned,8> saved{};
    for(unsigned i=0;i<offsets.size();++i)saved[i]=word(actor+offsets[i]);
    struct Restore{decltype(word)& w;unsigned a;const decltype(offsets)& offsets;decltype(saved)& saved;INativeSkillSystemServices& s;RandomNumberGenerator rng;
        ~Restore(){for(unsigned i=0;i<offsets.size();++i)w(a+offsets[i])=saved[i];s.Random()=rng;}
    } restore{word,actor,offsets,saved,system,rng};
    for(bool success:{false,true}){
        (void)skills.SetCurrentStatViaActor(0,3012);(void)skills.SetCurrentStatViaActor(1,2);
        (void)skills.SetCurrentStatViaActor(2,0);
        word(actor+208)=3012;word(actor+212)=2;word(actor+216)=0;word(actor+708)=99;word(actor+712)=0;
        system.Random().Seed(record.Seed(success));auto expected=system.Random();(void)expected.Next();(void)expected.Next();
        check(skills.TriggerForTarget(record.id,actor),"actual Monster9107 dispatch");
        check(skills.CurrentStat(0)==(success?0:3012) && skills.CurrentStat(1)==(success?0:2),"Monster9107 consumes both loot slots only on success");
        check(word(actor+208)==(success?0u:3012u) && word(actor+212)==(success?0u:2u) && word(actor+708)==(success?3012u:99u),"Monster9107 clears loot caches and records source result despite RET transfer");
        CheckSlotConsumeRng(system.Random(),expected);
    }
    std::cout<<"Original Skill9107 Monster: ID="<<record.id<<" success/failure, self target, slots/cache/tail and exact RNG pass\n";
}

inline void VerifyNativePlayerSlotConsumeOwners(NativePlayerManagerSession& source,const NativeGameDatabase& database,
    NativeSkillRecordCatalog& catalog,const NativeMonsterRecordCatalog& monsters,INativeSkillSystemServices& system){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto word=[](unsigned a)->unsigned&{return *reinterpret_cast<unsigned*>(a);};
    // Real item acquisition also updates global discovery history. An isolated
    // inventory preserves that production behavior without changing pal4.set.
    std::vector<unsigned> discoveries;
    NativeInventorySession inventory;check(inventory.Start()!=0u,"9107 isolated real inventory owner");inventory.Reset();inventory.BindDatabase(database);
    inventory.BindItemObserver([&](unsigned id){discoveries.push_back(id);});
    NativeScriptEventRouter router;NativeCombatSkillActorRegistry registry(router,catalog,system);
    NativeCombatActorOwner monster;(void)monster.Construct(monsters,2579);
    NativeCombatActorSkillRuntime target(monster,registry);(void)target.RebuildStats();
    NativeCombatPlayerActorOwner owner;(void)owner.Construct(source,0u);
    NativeCombatActorSkillRuntime player(owner,registry,inventory);(void)player.RebuildStats();
    const auto a=player.ActorAddress(),b=target.ActorAddress();const OriginalSlotConsume record(catalog);
    constexpr unsigned item=3012;check(inventory.PropertyAddress(item)!=0u,"9107 original Property fixture");
    const auto quantity=[&]{const auto entry=NativeInventoryPropertyReader(inventory).FindId(item);return entry?int(entry->count):0;};
    const auto initial=quantity();const auto rng=system.Random();
    const auto money=[&]{return *reinterpret_cast<const int*>(inventory.Address()+0x134u);};const auto initial_money=money();
    struct Restore{NativeInventorySession& inventory;decltype(quantity)& quantity;int initial;decltype(money)& money;int initial_money;INativeSkillSystemServices& system;RandomNumberGenerator rng;
        ~Restore(){(void)inventory.AddItem(item,initial-quantity());(void)inventory.UpdateCounter(initial_money-money());system.Random()=rng;}
    } restore{inventory,quantity,initial,money,initial_money,system,rng};
    (void)inventory.AddItem(item,3-initial);
    const auto discoveries_before=discoveries.size();
    struct Event{std::string name;std::array<unsigned,2> values;};std::vector<Event> events;
    const auto added=router.Subscribe(router.Key("add_property"),1u,[&](unsigned payload){
        const auto* p=reinterpret_cast<const unsigned*>(payload);
        check(!target.CurrentStat(0) && !target.CurrentStat(1) && !word(b+208) && !word(b+212),"9107 clears target before inventory notification");
        events.push_back({"add_property",{p[0],p[1]}});(void)NativeCombatInventoryEvents::Handle(0x57b0c0u,payload,inventory);
    });
    const auto picked=router.Subscribe(router.Key("add_picked_property"),1u,[&](unsigned payload){
        const auto* p=reinterpret_cast<const unsigned*>(payload);events.push_back({"add_picked_property",{p[0],p[1]}});
    });
    const auto failed=router.Subscribe(router.Key("pick_up_fail"),1u,[&](unsigned payload){
        check(*reinterpret_cast<const unsigned char*>(payload)==static_cast<unsigned char>(a>>24u),"9107 original failure byte");events.push_back({"pick_up_fail",{0,0}});
    });
    const auto cash=router.Subscribe(router.Key("add_money"),1u,[&](unsigned payload){
        events.push_back({"add_money",{word(payload),0}});(void)NativeCombatInventoryEvents::Handle(0x57b0a0u,payload,inventory);
    });
    const auto cash_picked=router.Subscribe(router.Key("add_picked_money"),1u,[&](unsigned payload){events.push_back({"add_picked_money",{word(payload),0}});});
    struct Unsubscribe{NativeScriptEventRouter& router;std::array<unsigned,5> tokens;~Unsubscribe(){for(auto token:tokens)(void)router.Unsubscribe(token);}} unsubscribe{router,{added,picked,failed,cash,cash_picked}};
    const auto setup=[&](int id,int count,unsigned guard){
        (void)target.SetCurrentStatViaActor(0,id);(void)target.SetCurrentStatViaActor(1,count);
        (void)target.SetCurrentStatViaActor(2,0);
        word(b+208)=unsigned(id);word(b+212)=unsigned(count);word(b+216)=0;word(a+708)=99;word(a+712)=guard;events.clear();
    };
    const auto invoke=[&](bool success){system.Random().Seed(record.Seed(success));auto expected=system.Random();(void)expected.Next();(void)expected.Next();
        check(player.TriggerForTarget(record.id,b),"actual Player9107 mixed dispatch");CheckSlotConsumeRng(system.Random(),expected);};
    setup(item,2,0);invoke(true);
    check(quantity()==5 && events.size()==2 && events[0].name=="add_property" && events[1].name=="add_picked_property" &&
        events[0].values==std::array<unsigned,2>{item,2} && events[1].values==events[0].values && word(a+708)==item,"9107 real inventory credit, exact payload/order and source result");
    check(discoveries.size()==discoveries_before+1u && discoveries.back()==item,"9107 real acquisition invokes discovery observer exactly once");
    setup(item,2,0);invoke(false);
    check(quantity()==5 && events.size()==1 && events[0].name=="pick_up_fail" && target.CurrentStat(0)==int(item) && word(b+208)==item,"9107 failed roll retains target and emits failure");
    setup(item,2,99);invoke(false);
    check(events.empty() && quantity()==5 && word(a+712)==99,"9107 source guard suppresses duplicate failure");
    setup(0,2,0);invoke(true);
    check(events.size()==1 && events[0].name=="pick_up_fail" && !target.CurrentStat(1) && !word(b+212) && quantity()==5,"9107 empty item still clears count/cache and emits failure");
    const auto invoke_money=[&](bool success){system.Random().Seed(record.Seed(success,true));auto expected=system.Random();(void)expected.Next();
        check(player.TriggerForTarget(record.money_id,b),"actual terminal9108 dispatch");CheckSlotConsumeRng(system.Random(),expected);};
    setup(item,2,0);(void)target.SetCurrentStatViaActor(2,17);word(b+216)=17;invoke_money(true);
    std::cout<<"9108 money diagnostic: initial="<<initial_money<<" actual="<<money()<<" slot="<<target.CurrentStat(2)<<" cache="<<word(b+216)<<" result="<<word(a+712)<<" events=";
    for(const auto& event:events)std::cout<<event.name<<':'<<event.values[0]<<','<<event.values[1]<<';';std::cout<<'\n';
    check(money()==initial_money+17 && !target.CurrentStat(2) && !word(b+216) && word(a+712)==17 && events.size()==2 &&
        events[0].name=="add_money" && events[1].name=="add_picked_money" && events[0].values[0]==17 && events[1].values==events[0].values,"9108 positive money transfers after target clear with both original events");
    setup(item,2,0);word(a+708)=0;(void)target.SetCurrentStatViaActor(2,17);word(b+216)=17;invoke_money(false);
    check(events.size()==1 && events[0].name=="pick_up_fail" && target.CurrentStat(2)==17 && word(b+216)==17 && money()==initial_money+17,"9108 failed roll preserves money slots/cache");
    setup(item,2,0);(void)target.SetCurrentStatViaActor(2,17);word(b+216)=17;invoke_money(false);
    check(events.empty() && target.CurrentStat(2)==17,"9108 item-result guard suppresses duplicate failure");
    for(int value:{0,-3}){setup(item,2,0);word(a+708)=0;(void)target.SetCurrentStatViaActor(2,value);word(b+216)=unsigned(value);invoke_money(true);
        check(events.size()==1 && events[0].name=="pick_up_fail" && !target.CurrentStat(2) && !word(b+216) && money()==initial_money+17,"9108 clears nonpositive money without credit");}
    player.Close();target.Close();owner.Destroy();monster.Destroy();check(!registry.Count(),"9107 fixture retires both actors");
    std::cout<<"Original Skill9107 Player: ID="<<record.id<<" real inventory, notification order, failed roll/guard/empty loot and exact RNG pass\n";
}
}
