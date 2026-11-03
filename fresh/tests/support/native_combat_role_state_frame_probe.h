#pragma once
#include "pal4/native_combat_role_state_frame.h"
#include "pal4/native_combat_end_sequence.h"
#include "pal4/native_combat_main_session.h"
#include "pal4/native_combat_player_manager.h"
#include "pal4/native_combat_player_actor_owner.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_timed_events.h"
#include "pal4/native_skill_record_catalog.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_cegui_ui_session.h"
#include "../../native/ui/cegui_exports_native.h"
#include <array>
#include <bit>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
namespace pal4::test {
inline void VerifyNativeCombatRoleStateFrame(NativeCombatRoleStateFrame& frame,NativeCombatEndSequence& ui_events,
    NativeCombatMainSession& main,NativeCeguiUiSession& ui,std::span<std::uint32_t,131> image,
    NativeCombatPlayerManager& players,NativeCombatActorSkillRuntime& skills,NativeSkillRecordCatalog& catalog,
    NativeCombatTimedEvents& timers,NativeScriptEventRouter& router){
    const auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    const auto module=GetModuleHandleW(L"CEGUIBase.dll");
    const auto get_text=detail::Export<const void* (__thiscall*)(const void*)>(module,"?getText@Window@CEGUI@@QBEABVString@2@XZ");
    const auto cstr=detail::Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ");
    const auto width=detail::Export<float (__thiscall*)(const void*)>(module,"?getWidth@Window@CEGUI@@QBEMXZ");
    const auto image_get=detail::Export<const void* (__thiscall*)(const void*)>(module,"?getImage@StaticImage@CEGUI@@QBEPBVImage@2@XZ");
    const auto parent=detail::Export<void* (__thiscall*)(const void*)>(module,"?getParent@Window@CEGUI@@QBEPAV12@XZ");
    const auto text=[&](std::uint32_t w){return std::string(cstr(get_text(reinterpret_cast<void*>(w))));};
    const auto a=skills.ActorAddress(),slot=players.Actor(a).Slot();check(slot<3u && frame.SubscriptionCount()==9u,"Role uses actual stage actor and nine subscriptions");
    const auto baseline=timers.TimerCount();const auto hp=skills.CurrentStat(7u);std::vector<std::uint32_t> inserted;inserted.reserve(2u);
    struct Restore{NativeCombatRoleStateFrame& frame;NativeCombatEndSequence& ui;NativeCombatActorSkillRuntime& skills;std::int32_t hp;std::vector<std::uint32_t>& inserted;
        ~Restore(){try{for(auto id:inserted)skills.Remove(id);(void)skills.SetCurrentStatViaActor(7u,hp);frame.RefreshStatus(skills.ActorAddress());(void)ui.CloseUi();}catch(...){}}
    } restore{frame,ui_events,skills,hp,inserted};
    check(ui_events.ShowUi()==1u && main.SubscriptionCount()==19u && timers.TimerCount()==baseline+1u,"actual5764B0 shows Main/Action/Role and schedules original timer");
    check(parent(reinterpret_cast<void*>(image[9]))==reinterpret_cast<void*>(ui.CombatLayoutRoot(NativeCombatLayout::Main)),"Role actual DLL parent is Main frame36");
    check(ui_events.ShowUi()==1u && timers.TimerCount()==baseline+2u,"original repeated Show inserts two same-name Role timers");
    (void)ui_events.CloseUi();timers.Update(std::numeric_limits<float>::min());
    check(timers.TimerCount()==baseline,"CloseUi cancels both same-name callbacks and next update retires their nodes");
    check(ui_events.ShowUi()==1u && timers.TimerCount()==baseline+1u,"Role can reopen after actual timer retirement");
    frame.RefreshActor(a);
    check(text(image[37u+slot])==std::to_string(hp) && text(image[43u+slot])==std::to_string(skills.CurrentStat(9u)) && text(image[40u+slot])==std::to_string(skills.CurrentStat(8u)),"Role DLL HP/MP/Qi text uses actual stat chain");
    const auto hp_product=static_cast<float>(static_cast<double>(hp)*std::bit_cast<float>(image[28u+slot]));
    const auto expected=static_cast<float>(static_cast<double>(hp_product)/skills.StatParameter(0u));
    check(std::abs(width(reinterpret_cast<void*>(image[49u+slot]))-expected)<.001f,"Role actual HP bar uses original narrowing");
    check(*reinterpret_cast<std::uint32_t*>(image[64u+slot]+280u)==a,"Role button carries actual actor");
    const auto changed=hp>1?hp-1:hp+1;(void)skills.SetCurrentStatViaActor(7u,changed);
    check(text(image[37u+slot])==std::to_string(skills.CurrentStat(7u)),"actual re_attr_int_ext_changed refreshes Role text synchronously");
    (void)skills.SetCurrentStatViaActor(7u,hp);
    // Original status records normally have MUTEX_ID. Exclude any record whose
    // ID or same-family mutex would replace an existing entry; context0 keeps
    // the original no-script gate while exercising both real list formats.
    const auto present=[&](std::uint32_t record,bool permanent){
        const auto id=*reinterpret_cast<const std::uint32_t*>(record),mutex=*reinterpret_cast<const std::uint32_t*>(record+60u);
        for(auto offset:{680u,696u}){const auto stride=offset==680u?12u:8u;
            const auto begin=*reinterpret_cast<const std::uint32_t*>(a+offset),end=*reinterpret_cast<const std::uint32_t*>(a+offset+4u);
            for(auto p=begin;p!=end;p+=stride){const auto existing=*reinterpret_cast<const std::uint32_t*>(p);if(!existing)continue;
                if(*reinterpret_cast<const std::uint32_t*>(existing)==id || (mutex && (offset==696u)==permanent && *reinterpret_cast<const std::uint32_t*>(existing+60u)==mutex))return true;
            }
        }return false;
    };
    for(bool permanent:{true,false}){
        bool added{};for(std::size_t i=0;i<catalog.Size();++i){const auto& r=catalog.At(i);
            if((r[3]==0xffffffffu)!=permanent || r[16]<1u || r[16]>28u || present(catalog.Address(r[0]),permanent))continue;
            check(skills.AddRecord(r[0],0u),"Role inserts actual original status record");inserted.push_back(r[0]);added=true;break;
        }check(added,"Role has original permanent and timed status records for verification");
    }
    frame.RefreshStatus(a);std::vector<std::int32_t> icons=skills.StatusIconIds(true);const auto timed=skills.StatusIconIds(false);icons.insert(icons.end(),timed.begin(),timed.end());
    unsigned used{};for(auto id:icons)if(id>0 && id<29 && used<10u){const auto control=image[101u+slot*10u+used++];
        check(reinterpret_cast<std::uint32_t>(image_get(reinterpret_cast<void*>(control)))==image[72u+static_cast<unsigned>(id)],"Role original permanent/timed order reaches actual DLL images");}
    check(used>=2u && !frame.ActiveCursors() && !*reinterpret_cast<const std::uint8_t*>(a+892u),"Role nonempty status refresh drains cursors and dirty flag");
    const auto ticks=frame.ColourTicks();timers.Update(.25f);check(frame.ColourTicks()==ticks,"Role timer does not fire early");timers.Update(.26f);check(frame.ColourTicks()==ticks+1u,"actual member412 half-second Role timer dispatches colours");
    bool rejected{};try{frame.RefreshActor(0x1000u);}catch(const std::exception&){rejected=true;}check(rejected && !frame.ActiveCursors(),"Role rejects foreign actors without leaked cursors");
    std::uint32_t facet=0x1000u;rejected=false;try{(void)router.Dispatch("re_join_stage",reinterpret_cast<std::uint32_t>(&facet));}catch(const std::exception&){rejected=true;}check(rejected && !frame.ActiveCursors(),"Role rejects foreign event facet and releases callback scope");
    frame.RefreshActor(a);facet=a+716u;(void)router.Dispatch("re_leave_stage",reinterpret_cast<std::uint32_t>(&facet));
    check(text(image[37u+slot]).empty() && !*reinterpret_cast<const std::uint32_t*>(image[64u+slot]+280u),"original leave event clears the existing role slot");
    (void)router.Dispatch("re_join_stage",reinterpret_cast<std::uint32_t>(&facet));check(text(image[37u+slot])==std::to_string(hp),"original join event restores actual role data");
    for(auto id:inserted)skills.Remove(id);inserted.clear();frame.RefreshStatus(a);
    check(ui_events.CloseUi()==1u && frame.SubscriptionCount()==9u && !main.SubscriptionCount(),"CloseUi hides Role/cancels its timer while retaining constructor subscriptions");
    timers.Update(.001f);check(timers.TimerCount()==baseline && !frame.ActiveCursors(),"Role cancelled timer node and callback cleanup remain balanced");
    std::cout<<"Native Combat Role frame: actual5764B0 Main/Action/Role parent,9 events, real HP/MP/Qi and bar, nonempty permanent/timed icons, member412 half-second colours, foreign failures/retry, join/leave and timer cancellation pass; click gameplay/full Entry remain incomplete\n";
}
}
