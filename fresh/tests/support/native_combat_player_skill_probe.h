#pragma once
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_skill_actor_registry.h"
#include "pal4/native_combat_player_actor_owner.h"
#include "pal4/native_combat_actor_owner.h"
#include "pal4/native_monster_record_catalog.h"
#include "pal4/native_skill_record_catalog.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_script_event_router.h"
#include "native_combat_damage_probe.h"
#include "native_combat_current_delta_probe.h"
#include "native_combat_attack_notifications_probe.h"
#include "native_combat_attack_gates_probe.h"
#include "native_combat_money_transfer_probe.h"
#include "native_combat_status_chain_probe.h"
#include "native_combat_multi_effect_probe.h"
#include "native_combat_random_response_probe.h"
#include "native_combat_conditional_actions_probe.h"
#include <algorithm>
#include <functional>
#include <iostream>
#include <set>
#include <vector>
namespace pal4::test {
inline void VerifyNativeCombatPlayerSkills(NativePlayerManagerSession& source,NativeInventorySession& inventory,
    NativeSkillRecordCatalog& catalog,const NativeMonsterRecordCatalog& monsters,
    INativeSkillSystemServices& system,const NativeCombatFormulaCatalog& formulas){
    const auto check=[](bool b,const char* m){if(!b)throw std::runtime_error(m);};
    const auto reject=[&](auto&& f){bool denied=false;try{f();}catch(const std::logic_error&){denied=true;}check(denied,"Player skill guard");};
    const auto word=[](std::uint32_t a)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(a);};
    NativeScriptEventRouter router;
    NativeCombatSkillActorRegistry registry(router,catalog,system);
    NativeCombatDamageCalculator damage(registry,formulas);
    NativeCombatActorOwner monster;const auto m=monster.Construct(monsters,2579);
    NativeCombatActorSkillRuntime target(monster,registry);(void)target.RebuildStats();
    std::uint32_t scalar{},trigger{};std::vector<std::uint32_t> chain;
    for(std::size_t i=0;i<catalog.Size() && !scalar;++i){
        const auto& root=catalog.At(i);
        if(root[4]!=9109u && root[4]!=9112u && root[4]!=9113u && root[4]!=9105u)continue;
        std::set<std::uint32_t> seen;std::vector<std::uint32_t> candidate;
        auto p=catalog.Address(root[0]);bool supported=true;
        while(p){
            if(!seen.insert(p).second){supported=false;break;}
            const auto f=word(p+16u);
            if(f!=9105u && f!=9109u && f!=9112u && f!=9113u && f!=9101u && f!=9102u && f!=9103u && f!=9104u && f!=9120u && f!=9121u){supported=false;break;}
            candidate.push_back(p);const auto next=word(p+56u);p=next?catalog.Address(next):0u;
        }
        if(supported){scalar=root[0];trigger=root[2];chain=std::move(candidate);}
    }
    check(scalar && !chain.empty(),"original Player scalar Skill chain exists");
    for(unsigned index=0;index<4u;++index){
        NativeCombatPlayerActorOwner player;const auto a=player.Construct(source,index);
        const auto allocations=player.OwnedAllocations(),arrays=player.OwnedArrays();
        NativeCombatActorSkillRuntime skills(player,registry,inventory);
        reject([&]{NativeCombatActorSkillRuntime duplicate(player,registry,inventory);});
        reject([&]{player.Destroy();});
        const auto hp=skills.CurrentStat(7u),mp=skills.CurrentStat(9u),sp=skills.CurrentStat(8u);
        (void)skills.RebuildStats();
        const auto max_hp=skills.StatParameter(0u),max_mp=skills.StatParameter(2u),speed=skills.StatParameter(5u);
        check(skills.CurrentStat(7u)==std::clamp(hp,0,std::max(max_hp,0)) &&
              skills.CurrentStat(9u)==std::clamp(mp,0,std::max(max_mp,0)) &&
              skills.CurrentStat(8u)==std::clamp(sp,0,100),"Player538BC0 retains clamped HP/MP/SP");
        (void)skills.RebuildStats();
        check(skills.StatParameter(0u)==max_hp && skills.StatParameter(2u)==max_mp && skills.StatParameter(5u)==speed &&
              player.OwnedAllocations()==allocations,"Player repeated rebuild replaces property and never doubles equipment");
        VerifyNativeCombatDamage(damage,skills,target,formulas,system,"Player/Monster");
        VerifyNativeCombatStatusChain(player,skills,system,router,catalog);
        VerifyNativeCombatCurrentDelta(damage,skills,m,system,router,"Player");
        VerifyNativeCombatAttackNotifications(skills,target,router,"Player");
        VerifyNativeCombatAttackNotifications(target,skills,router,"Monster");
        VerifyNativeCombatAttackGates(damage,skills,target,formulas,system,"Player");
        if(index==0u)VerifyNativeCombatMoneyTransfer(skills,target,inventory,catalog,router,system);
        if(index==0u)VerifyNativeCombatMultiEffect(skills,target,registry,monsters,catalog,router,system);
        if(index==0u)VerifyNativeCombatRandomResponse(skills,target,catalog,router,system);
        if(index==0u)VerifyNativeCombatConditionalActions(skills,target,catalog,system);
        unsigned player_added{},wrong_added{};
        const auto joined=router.Subscribe(router.Key("re_new_added"),1u,[&](std::uint32_t p){check(word(p)==a+716u,"Player new actor facet");++player_added;});
        const auto wrong=router.Subscribe(router.Key("me_new_added"),1u,[&](std::uint32_t){++wrong_added;});
        skills.NotifyNewActor();check(player_added==1u && !wrong_added,"Player uses re new actor event family");
        (void)router.Unsubscribe(joined);(void)router.Unsubscribe(wrong);
        skills.BindActionProgress(formulas);skills.PrepareActionProgress();
        player.SetSlot(0u);skills.SetActionProgress(0.125f);check(skills.ActionProgress()==0.125f,"Player real skill facade progress binding");
        std::function<void()> hook;unsigned added{},removed{},calls{},player_changes{},monster_changes{};
        std::vector<std::uint32_t> tokens;
        for(const auto name:{"ae_add_skill","ae_remove_skill","ae_call_fun_id","re_attr_float_ext_changed","me_attr_float_ext_changed"})
            tokens.push_back(router.Subscribe(router.Key(name),1u,[&,name](std::uint32_t p){
                const auto facet=word(p);check(facet==a+716u || facet==m+716u,"shared Player/Monster skill event receiver");
                if(std::string_view(name)=="ae_add_skill")++added;
                if(std::string_view(name)=="ae_remove_skill")++removed;
                if(std::string_view(name)=="ae_call_fun_id")++calls;
                if(std::string_view(name)=="re_attr_float_ext_changed")++player_changes;
                if(std::string_view(name)=="me_attr_float_ext_changed")++monster_changes;
                reject([&]{player.Destroy();});reject([&]{skills.Close();});reject([&]{target.Close();});
                if(hook)hook();
            }));
        check(skills.AddRecord(7605u,a),"Player original permanent Skill insertion");
        check(skills.AddRecord(scalar,a),"Player original scalar chain insertion");
        skills.Process(trigger);
        check(calls>0u && added==2u,"Player real SkillSystem callback chain and add notification");
        // Force the target fields to differ, so routing/event-family assertions
        // remain observable even when this original effect was already active.
        for(unsigned i=0;i<5u;++i)word(m+584u+4u*i)=0x3f7fffffu;
        skills.Process(trigger,m);
        check(monster_changes>0u,"Player context dispatches scalar effects into registered Monster");
        check(player_changes>0u,"Player own scalar effects dispatch re events");
        check(skills.OwnedBuffers()>0u && registry.Count()==2u,"Player owns actual skill buffer in shared registry");
        bool failed=false;hook=[] {throw std::runtime_error("intentional Player skill observer failure");};
        try{skills.Process(trigger);}catch(const std::runtime_error&){failed=true;}
        check(failed,"Player skill callback exception exercised");hook={};skills.Process(trigger);
        skills.Remove(scalar);skills.Remove(7605u);check(removed>=2u,"Player original removal and stat refresh");
        for(auto token:tokens)(void)router.Unsubscribe(token);
        skills.Close();check(registry.Count()==1u && !skills.OwnedBuffers(),"Player lists/registry retired");
        reject([&]{(void)damage.Calculate(a,m);});
        reject([&]{(void)skills.AddCurrentStat(7u,-1);});
        reject([&]{skills.NotifyPhysicalAttack(m);});
        reject([&]{(void)skills.AllowsAttack();});reject([&]{(void)skills.AllowsDamage();});
        reject([&]{(void)skills.BlocksReflectedDamage();});reject([&]{(void)damage.CheckHit(a,m);});
        check(player.OwnedAllocations()==allocations && player.OwnedArrays()==arrays,"Player native allocator remains balanced");
        std::cout<<"Native Player skills probe: role="<<index<<" maxHP="<<max_hp<<" maxMP="<<max_mp
                 <<" speed="<<speed<<" scalar="<<scalar<<" chain="<<chain.size()<<'\n';
        player.Destroy();
    }
    target.Close();monster.Destroy();check(!registry.Count(),"mixed registry fully retired");
    std::cout<<"Native Player skills: actual4 equipment/stat rebuilds, original Skill chains, mixed Monster routing and ownership pass\n";
}
}
