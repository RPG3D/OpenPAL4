#pragma once
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_skill_system_owner.h"
#include "pal4/native_player_combat_events.h"
#include "pal4/native_combat_player_actor_owner.h"
#include "pal4/native_combat_skill_actor_registry.h"
#include "pal4/native_skill_record_catalog.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_combat_actor_skill_events.h"
#include "native_combat_parameter_skill_probe.h"
#include <array>
#include <bit>
#include <iostream>
#include <stdexcept>

namespace pal4::test {
// Exercise real event40 callbacks and actor-owned links, for both concrete
// Combat Player and Monster owners. No raw status/ownership writes are used.
template<class Owner>
void VerifyNativeCombatStatusChain(Owner& owner,NativeCombatActorSkillRuntime& skills,
    INativeSkillSystemServices& system,NativeScriptEventRouter& router,NativeSkillRecordCatalog& catalog) {
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto word=[](std::uint32_t a){return *reinterpret_cast<const std::uint32_t*>(a);};
    const auto a=owner.Address(),base=word(a+800u);
    const auto allocations=owner.OwnedAllocations();
    const auto hp=skills.CurrentStat(7u);
    const auto attack=skills.AllowsAttack(),damage=skills.AllowsDamage();
    const auto command=skills.CommandAvailability(0u);
    std::array<bool,5> original_commands{};
    for(unsigned mode=0;mode<original_commands.size();++mode)
        original_commands[mode]=skills.CommandAvailability(mode);
    const auto event=system.PropertyEvents().Lookup(40u);
    check(word(a+636u)==0u && skills.AllowsAction(),"status fixture starts with unwrapped action availability");
    const auto set=[&](float value){skills.SetCombatEvent(event,std::bit_cast<std::uint32_t>(value));};
    set(1.f);
    auto root=word(a+800u);
    check(word(a+636u)==1u && word(root)==0x847730u && word(root+8u)==base &&
          *reinterpret_cast<const unsigned char*>(root+4u)==1u &&
          *reinterpret_cast<const unsigned char*>(a+796u)==1u,
          "event40 installs owned EAST_B1 wrapper over original base");
    check(owner.OwnedAllocations()==allocations+1u && !skills.AllowsAction(),
          "EAST_B1 allocates one node and blocks action");
    check(skills.CurrentStat(7u)==hp && skills.AllowsAttack()==attack &&
          skills.AllowsDamage()==damage && skills.CommandAvailability(0u)==command,
          "wrapped numeric and inherited predicates preserve original behavior");
    skills.Defend();skills.NotifyPhysicalAttack(a);skills.NotifyCriticalAttack(a);
    NativeCombatActorSkillEvents notifications(owner,router,catalog);
    for(const auto slot:{308u,312u,316u,320u})
        notifications.InvokeVirtual(a,slot,std::array{a,catalog.Address(7648u),1u});
    check(word(a+800u)==root && !skills.AllowsAction(),"lifecycle/attack forwarding preserves active status");
    set(1.f);
    root=word(a+800u);
    check(word(root)==0x847730u && word(root+8u)==base && owner.OwnedAllocations()==allocations+1u,
          "reapplication deduplicates status without leaking retired wrapper");
    set(0.f);
    check(word(a+636u)==0u && word(a+800u)==base && owner.OwnedAllocations()==allocations && skills.AllowsAction(),
          "clearing event40 restores original base ownership and action availability");
    set(0.f);
    check(word(a+800u)==base && owner.OwnedAllocations()==allocations,"repeated clear is balanced");
    set(4.f);
    root=word(a+800u);
    check(word(a+636u)==4u && word(root)==0x847310u && word(root+8u)==base &&
          owner.OwnedAllocations()==allocations+1u && !skills.AllowsAction(),
          "event40 variant4 installs its original owned action-blocking wrapper");
    check(skills.CurrentStat(7u)==hp && skills.AllowsAttack()==attack &&
          skills.AllowsDamage()==damage && skills.CommandAvailability(0u)==command,
          "variant4 preserves inherited predicates and numeric access");
    for(const auto slot:{308u,312u,316u,320u})
        notifications.InvokeVirtual(a,slot,std::array{a,catalog.Address(7315u),1u});
    set(1.f);
    check(word(word(a+800u))==0x847730u && word(word(a+800u)+8u)==base &&
          owner.OwnedAllocations()==allocations+1u,"variant1 replaces variant4 by original property ID1");
    set(4.f);set(4.f);
    check(word(word(a+800u))==0x847310u && word(word(a+800u)+8u)==base &&
          owner.OwnedAllocations()==allocations+1u,"variant4 replacement and reapplication retain one owned wrapper");
    set(0.f);
    check(word(a+800u)==base && owner.OwnedAllocations()==allocations && skills.AllowsAction(),
          "variant4 clear restores exact base ownership and action availability");
    set(2.f);root=word(a+800u);
    check(word(a+636u)==2u && word(root)==0x8475d0u && word(root+8u)==base &&
          owner.OwnedAllocations()==allocations+1u && skills.AllowsAction(),
          "EAST_B2 retains original owned terminal and general action availability");
    check(!skills.CommandAvailability(3u) && !skills.CommandAvailability(4u) &&
          skills.CommandAvailability(0u)==command && skills.AllowsAttack()==attack && skills.AllowsDamage()==damage,
          "EAST_B2 only blocks command kinds3/4 and forwards inherited predicates");
    set(0.f);check(word(a+800u)==base && owner.OwnedAllocations()==allocations && skills.AllowsAction(),
          "EAST_B2 clear restores terminal and owned allocation count");
    std::cout<<"Native Combat EAST_B2: original vtable, shared56D9B0 command3/4 gate, inherited actions and owned cleanup pass\n";
    set(3.f);root=word(a+800u);
    check(word(a+636u)==3u && word(root)==0x847470u && word(root+8u)==base &&
          owner.OwnedAllocations()==allocations+1u && skills.AllowsAction(),
          "EAST_B3 retains original owned terminal and general action availability");
    for(unsigned mode=0;mode<original_commands.size();++mode)
        check(skills.CommandAvailability(mode)==((mode==2u || mode==4u)?false:original_commands[mode]),
              "EAST_B3 blocks command kinds2/4 and forwards the other modes");
    check(skills.AllowsAttack()==attack && skills.AllowsDamage()==damage,
          "EAST_B3 retains inherited attack and damage predicates");
    set(2.f);set(3.f);set(3.f);
    check(word(word(a+800u))==0x847470u && word(word(a+800u)+8u)==base &&
          owner.OwnedAllocations()==allocations+1u,
          "EAST_B3 replacement and reapplication retain one owned wrapper");
    set(0.f);
    check(word(a+800u)==base && owner.OwnedAllocations()==allocations && skills.AllowsAction(),
          "EAST_B3 clear restores terminal ownership and action availability");
    std::cout<<"Native Combat EAST_B3: original vtable, shared56D9E0 command2/4 gate, replacement and owned cleanup pass\n";
    std::cout<<"Native Combat EAST_B1: event40, owned wrapper, action blocking, inherited dispatch, reapply and clear pass\n";
    std::cout<<"Native Combat EAST_B4: event40, original vtable, notification forwarding, cross-variant deduplication and clear pass\n";
}
template<class Owner>
void VerifyOriginalTimedCombatStatus(Owner& owner,NativeCombatActorSkillRuntime& skills,
    const NativeSkillRecordCatalog& catalog) {
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto word=[](std::uint32_t a){return *reinterpret_cast<const std::uint32_t*>(a);};
    const auto a=owner.Address();
    const auto count=[&]{unsigned found{};for(auto p=word(a+680u);p!=word(a+684u);p+=12u)if(word(p))++found;return found;};
    check(count()==0u && skills.AllowsAction(),"original7648 fixture starts without timed status");
    check(skills.AddRecord(7648u,a) && count()==1u && word(a+636u)==1u && !skills.AllowsAction(),
          "original7648 addition executes real9100 and wrapped skill notifications");
    check(skills.AddRecord(7648u,a) && count()==1u && word(a+636u)==1u && !skills.AllowsAction(),
          "original7648 mutex40 replacement retains exactly one live status");
    skills.Advance(2u);
    check(count()==1u && word(a+636u)==1u && !skills.AllowsAction(),"original7648 remains active before three-turn limit");
    skills.Advance(1u);
    check(count()==0u && word(a+636u)==0u && skills.AllowsAction(),
          "original7648 expiry removes status through real notifications and stat rebuild");
    skills.Advance(0u);
    check(word(a+680u)==word(a+684u),"expired7648 list compacts without a live entry");
    std::cout<<"Native Combat original7648: real9100, four notification slots, mutex replacement, three-turn expiry and restored action pass\n";
    const auto record=catalog.Address(7315u);
    check(record!=0u,"original7315 resource is present");
    const auto duration=word(record+12u);
    check(duration>0u && duration<100u,"original7315 has a bounded natural turn duration");
    check(skills.AddRecord(7315u,a) && count()==1u && word(a+636u)==4u && !skills.AllowsAction(),
          "original7315 applies real EAST_B4 through skill callbacks");
    skills.Advance(duration-1u);
    check(count()==1u && word(a+636u)==4u && !skills.AllowsAction(),
          "original7315 remains active until its resource turn limit");
    skills.Advance(1u);
    check(count()==0u && word(a+636u)==0u && skills.AllowsAction(),
          "original7315 expires through real removal notifications and stat rebuild");
    std::cout<<"Native Combat original7315: EAST_B4 actual resource duration="<<duration<<", blocked action and natural expiry pass\n";
    const auto ban=catalog.Address(7651u);
    check(ban!=0u,"original7651 resource is present");
    const auto ban_duration=word(ban+12u);
    check(ban_duration>0u && ban_duration<100u,"original7651 has a bounded natural turn duration");
    check(skills.AddRecord(7651u,a) && count()==1u && word(a+636u)==3u &&
          word(word(a+800u))==0x847470u && skills.AllowsAction() && !skills.CommandAvailability(2u),
          "original7651 applies real EAST_B3 through skill callbacks");
    skills.Advance(ban_duration-1u);
    check(count()==1u && word(a+636u)==3u,"original7651 remains active until its resource turn limit");
    skills.Advance(1u);
    check(count()==0u && word(a+636u)==0u && skills.AllowsAction(),
          "original7651 expires through real removal notifications and stat rebuild");
    std::cout<<"Native Combat original7651: EAST_B3 actual resource duration="<<ban_duration<<", blocked command and natural expiry pass\n";
}
inline void VerifyLoadedCombatPlayerPropertySkills(NativePlayerManagerSession& source,
    NativeInventorySession& inventory,const NativeGameDatabase& database,INativeSkillSystemServices& system) {
    NativeSkillRecordCatalog catalog(database);NativeScriptEventRouter router;
    NativeCombatSkillActorRegistry registry(router,catalog,system);
    for(unsigned index=0;index<4u;++index){
        NativeCombatPlayerActorOwner player;(void)player.Construct(source,index);
        NativeCombatActorSkillRuntime skills(player,registry,inventory);
        VerifyNativeCombatStatusChain(player,skills,system,router,catalog);
        VerifyOriginalTimedCombatStatus(player,skills,catalog);
        VerifyOriginalCombatHaste(player,skills,system,router,catalog);
        skills.SetCombatEvent(system.PropertyEvents().Lookup(40u),std::bit_cast<unsigned>(4.f));
        skills.Close();player.Destroy();
        if(player.OwnedAllocations())throw std::runtime_error("wrapped Player destruction leaks");
    }
    if(registry.Count())throw std::runtime_error("status probe retains registered Player");
    std::cout<<"Native Combat property skills: four actual loaded Players, EAST_B1/2/3/4, CA_SPEED/haste and wrapped destruction pass\n";
}
}
