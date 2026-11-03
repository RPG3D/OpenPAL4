#include "native_combat_player_input_policy.h"
#include "pal4/native_combat_action_controller.h"
#include "pal4/native_combat_action_window.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/native_magic_window_records.h"
#include "pal4/native_magic_window_layout.h"
#include "pal4/native_cegui_window_pointer_events.h"
#include "pal4/native_combat_target_selection.h"
#include "pal4/native_combat_monster_manager.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_inventory_property_reader.h"
#include "pal4/native_skill_record_catalog.h"
#include "../../native/ui/cegui_exports_native.h"
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <bit>
namespace pal4::test {
void ReportNativeCombatPlayerInitialStats(std::span<NativeCombatActorSkillRuntime* const> players){
    for(auto* s:players)std::cout<<"Native natural Player entry: actor="<<s->ActorAddress()<<" HP="<<s->CurrentStat(7u)<<"/"<<s->StatParameter(0u)
        <<" MP="<<s->CurrentStat(9u)<<"/"<<s->StatParameter(2u)<<std::endl;
}
void TeachNextPartyMagicForNaturalProbe(NativeMagicWindowLayout& layout,const std::function<void(unsigned)>& key,const std::function<void()>& rethrow){
    const auto previous=layout.State().selected_character_index;
    NativeCeguiWindowPointerEvents mouse(GetModuleHandleW(L"CEGUIBase.dll"));
    (void)mouse.HandleMouseAtCenter(layout.Panel().At(0x84u));rethrow();
    if(layout.State().selected_character_index==previous)throw std::runtime_error("actual Magic character arrow did not select next party member");
    key('2');
    (void)mouse.HandleMouseAtCenter(layout.Panel().At(0xc0u));rethrow();key(VK_RETURN);
    std::cout<<"Natural teaching policy: actual DLL next-character arrow and Water row/Enter for party index="<<layout.State().selected_character_index<<"; original point/learning path only\n";
}
NativeCombatPlayerInputPolicy::NativeCombatPlayerInputPolicy(NativeInventorySession& inventory,NativeSkillRecordCatalog& skills,NativeCombatMonsterManager& monsters)
    :inventory_(inventory),skills_(skills),monsters_(monsters){
    NativeInventoryPropertyReader reader(inventory_);unsigned entries{};
    for(int group=0;group<3;++group)for(int n=0,size=reader.Count(group);n<size;++n){const auto item=reader.AtReverse(group,static_cast<unsigned>(n));if(!item)throw std::logic_error("natural inventory snapshot failed");
        std::cout<<"Native natural inventory: group="<<group<<" ID="<<item->id<<" quantity="<<item->count
            <<" AI="<<*reinterpret_cast<const std::uint32_t*>(item->definition+8u)<<std::endl;++entries;}
    std::cout<<"Native natural inventory: combat_groups_entries="<<entries<<"; read only, no stock supplied\n";
}
bool NativeCombatPlayerInputPolicy::PlanRecovery(std::span<NativeCombatActorSkillRuntime* const> players,NativeCombatActorSkillRuntime& source,bool needs_mp){
    NativeCombatActorSkillRuntime* wounded{};double ratio=.65;
    for(auto* p:players){const auto max=p->StatParameter(0u),hp=p->CurrentStat(7u);if(max<=0 || hp<=0 || !p->CommandAvailability(4u))continue;
        const auto value=static_cast<double>(hp)/max;if(value<ratio){ratio=value;wounded=p;}}
    if(!wounded && !needs_mp)return false;
    const auto word=[](std::uint32_t a){return *reinterpret_cast<const std::uint32_t*>(a);};
    NativeInventoryPropertyReader reader(inventory_);
    for(int n=0,size=reader.Count(0);n<size;++n){const auto item=reader.AtReverse(0,static_cast<unsigned>(n));if(!item || !item->count)continue;
        const auto r=item->definition,s=skills_.Address(word(r+148u));
        if(word(r+8u)>=6u || word(r+12u)!=1u || !s || word(s+16u)!=9118u)continue;
        const auto next=word(s+56u),extra=next?skills_.Address(next):0u;
        if(next && (!extra || word(extra+16u)!=9132u || word(extra+56u)))continue;
        bool hp{},mp{};
        for(auto offset:{32u,44u}){const auto id=std::bit_cast<float>(word(s+offset)),value=std::bit_cast<float>(word(s+offset+4u));if(value>0.f){hp|=id==2.f;mp|=id==6.f;}}
        auto* receiver=wounded && hp?wounded:!wounded && needs_mp && mp?&source:nullptr;if(!receiver)continue;
        property_=item->id;target_=receiver->ActorAddress();
        std::cout<<"Natural input recovery plan: actual ID="<<property_<<" quantity="<<item->count<<" target="<<target_
            <<" HP="<<receiver->CurrentStat(7u)<<" MP="<<receiver->CurrentStat(9u)<<"; original Property UI and target keys only\n";
        return true;
    }
    return false;
}
std::uint32_t NativeCombatPlayerInputPolicy::NextTargetKey(NativeCombatTargetSelection& selection){
    if(property_)return selection.CurrentActor()==target_?28u:205u;
    std::uint32_t desired{};std::int32_t least=INT32_MAX;
    for(std::size_t n=0;n<monsters_.StageCount();++n){const auto a=monsters_.StageActorAt(n);const auto hp=monsters_.Skills(a).CurrentStat(7u);
        if(hp>0 && hp<least){least=hp;desired=a;}}
    if(!desired)throw std::logic_error("natural target policy requires living Monster candidate");
    return selection.CurrentActor()==desired?28u:205u;
}
std::uint32_t NativeCombatPlayerInputPolicy::NextPlayerKey(NativeCombatActionController& controller,NativeCombatActionWindow& window,
    NativeEventHandlerArray& active,std::span<NativeCombatActorSkillRuntime* const> players,const NativeMagicWindowRecords& records,bool scripted){
    NativeCombatActorSkillRuntime* source{};
    for(auto* player:players)if(player && player->ActorAddress()==window.Word(88u))source=player;
    if(!source)throw std::logic_error("natural input policy requires actual current Player");
    const auto* magic=records.FindMagic(5000u);unsigned element=5u;
    if(magic)for(unsigned n=0;n<5u;++n){
        const auto ids=source->MasteredMagic(n);
        if(std::find(ids.begin(),ids.end(),5000u)!=ids.end())element=n;
    }
    if(active.GetLast()==window.Address()+40u){
        property_=target_=0u;
        if(PlanRecovery(players,*source,element<5u && magic->cast_cost>source->CurrentStat(9u)))return 3u;
        // Finish an already weakened minion through the actual Attack UI,
        // preserving scarce MP for remaining opponents.
        bool weak{};for(std::size_t n=0;n<monsters_.StageCount();++n){const auto hp=monsters_.Skills(monsters_.StageActorAt(n)).CurrentStat(7u);weak|=hp>0 && hp<=12;}
        if(weak)return 2u;
        return element<5u && magic->cast_cost<=source->CurrentStat(9u)?4u:scripted?6u:2u;
    }
    if(!controller.OwnsSelectorActivity(active.GetLast()))throw std::logic_error("natural input policy foreign activity");
    if(property_){
        auto& layout=window.Selector(NativeCombatSelectorKind::Property);
        if(active.GetLast()!=layout.Address()+48u)throw std::logic_error("natural Property input activity mismatch");
        if(!inventory_.ItemCount(property_)){property_=target_=0u;return 1u;}
        if(layout.Word(56u)!=0u)return 203u;
        const auto item=reinterpret_cast<std::uint32_t>(detail::Export<void* (__thiscall*)(const void*)>(GetModuleHandleW(L"CEGUIBase.dll"),
            "?getHoverItem@MultiColumnList@CEGUI@@QBEPAVListboxItem@2@XZ")(reinterpret_cast<void*>(layout.List())));
        if(!item)throw std::logic_error("natural recovery inventory list unexpectedly empty");
        return *reinterpret_cast<const std::uint32_t*>(item+312u)==property_?28u:208u;
    }
    if(element>=5u || magic->cast_cost>source->CurrentStat(9u))return 1u;
    auto& layout=window.Selector(NativeCombatSelectorKind::Magic);
    if(layout.Word(56u)!=element)return layout.Word(56u)<element?205u:203u;
    const auto item=reinterpret_cast<std::uint32_t>(detail::Export<void* (__thiscall*)(const void*)>(GetModuleHandleW(L"CEGUIBase.dll"),
        "?getHoverItem@MultiColumnList@CEGUI@@QBEPAVListboxItem@2@XZ")(reinterpret_cast<void*>(layout.List())));
    if(!item)throw std::logic_error("natural learned Magic list unexpectedly empty");
    if(*reinterpret_cast<const std::uint32_t*>(item+312u)!=5000u)return 208u;
    if(*reinterpret_cast<const std::uint8_t*>(item+421u))throw std::logic_error("natural affordable Magic row disabled");
    return 28u;
}
void StartNativeCombatRecoveryPolicyProbe(NativeCombatActionController& controller,NativeCombatActionWindow& window,NativeEventHandlerArray& active,
    std::span<NativeCombatActorSkillRuntime* const> players,const NativeMagicWindowRecords& records,NativeInventorySession& inventory,
    NativeSkillRecordCatalog& skills,NativeCombatMonsterManager& monsters,NativeCombatTargetSelection& selection){
    if(players.empty())throw std::invalid_argument("recovery policy fixture requires actual Players");
    NativeCombatPlayerInputPolicy policy(inventory,skills,monsters);const auto actor=players.front()->ActorAddress();
    auto& automatic=*reinterpret_cast<std::uint32_t*>(actor+1200u);
    struct Restore {std::uint32_t& value;std::uint32_t old;~Restore(){value=old;}} restore{automatic,automatic};automatic=0u;
    window.Word(88u)=actor;window.SetVisible(actor,true);
    const auto stock=inventory.ItemCount(3000u);unsigned keys{};bool target_started{};
    for(;keys<64u;++keys){
        if(!window.Word(88u) && !selection.CurrentActor())break;
        const auto code=window.Word(88u)?policy.NextPlayerKey(controller,window,active,players,records,true):policy.NextTargetKey(selection);
        if(!window.Word(88u))target_started=true;
        std::array<std::uint32_t,5> e{};e[4]=code;
        (void)active.DispatchKeyDown(active.GetLast(),reinterpret_cast<std::uint32_t>(e.data()));window.RethrowEventError();
        if(inventory.ItemCount(3000u)!=stock)throw std::runtime_error("policy key transport consumed stock before animation");
    }
    if(!keys || keys==64u || !target_started || window.Word(88u) || selection.CurrentActor() || controller.ActiveIterators() || controller.PendingPayloads())
        throw std::runtime_error("recovery policy popup/target key sequence did not finish");
    std::cout<<"Recovery policy protocol: original actual inventory/Property popup/target key dispatch completed in"<<keys
        <<" keys, stock unchanged until AMF; caller's explicit stock/HP fixture, not naturally acquired inventory\n";
}
}
