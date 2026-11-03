#pragma once
#include "pal4/native_combat_action_availability.h"
#include "pal4/native_combat_action_window.h"
#include "pal4/native_combat_player_manager.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/indexed_collection_reset.h"
#include "../../native/ui/cegui_exports_native.h"
#include <array>
#include <cstring>
#include <iostream>
#include <vector>
namespace pal4::test {
inline void VerifyNativeCombatActionAvailability(NativeCombatActionAvailability& availability,
    NativeCombatActionWindow& window,NativeCombatPlayerManager& players,
    std::span<NativeCombatActorSkillRuntime* const> skills,NativeEventHandlerArray& active,std::uint32_t monster){
    const auto check=[](bool b,const char* why){if(!b)throw std::runtime_error(why);};
    const auto word=[](std::uint32_t p)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(p);};
    const auto previous=active.GetLast(),count=active.Size(),cursors=players.ActiveIterators();
    struct Restore {
        NativeCombatActionWindow& window;std::vector<std::pair<std::uint32_t,std::uint32_t>> words;
        ~Restore(){for(auto [p,v]:words)*reinterpret_cast<std::uint32_t*>(p)=v;try{window.SetVisible(0u,false);}catch(...){}}
        void Save(std::uint32_t p){words.emplace_back(p,*reinterpret_cast<std::uint32_t*>(p));}
    } restore{window};
    restore.Save(window.Address()+0x3cu);restore.Save(window.Address()+0x1e4u);
    check(!skills.empty() && skills.size()==players.Count(),"actual Action availability covers first Player roster");
    const auto module=GetModuleHandleW(L"CEGUIBase.dll");
    const auto disabled=detail::Export<bool (__thiscall*)(const void*,bool)>(module,"?isDisabled@Window@CEGUI@@QBE_N_N@Z");
    bool actual_magic{};unsigned checks{};
    for(auto* s:skills){
        for(unsigned i=0;i<6u;++i){check(availability.Property(s->ActorAddress(),i)==static_cast<std::uint8_t>(s->CommandAvailability(i)),"all six original command properties use actual actor800 slot8");++checks;}
        actual_magic|=s->HasMasteredMagic();
    }
    check(availability.HasAvailableMagic()==actual_magic && players.ActiveIterators()==cursors,"original first-roster scan agrees with actual copied Player magic collections and releases cursor");
    const auto actor=skills.front()->ActorAddress();
    window.SetVisible(actor,true);
    check(active.GetLast()==window.Address()+40u && active.Size()==count+1u,"actual global activity activates Action navigation on display");
    for(unsigned i=0;i<6u;++i)check(!disabled(reinterpret_cast<void*>(window.Button(i)),false)==(availability.Property(actor,i)!=0u && (i!=3u || actual_magic)),"actual DLL button availability matches original property plus team magic condition");
    check(window.InvokePredicate(window.Address()+40u,4u,0u)==0u && window.InvokePredicate(window.Address()+40u,8u,0u)==0u,"actual Action input capture slots preserve original constant zero callbacks");
    active.Update();
    check(window.InvokeVirtual(window.Address()+40u,16u)==1u &&
        *reinterpret_cast<const std::uint8_t*>(window.Button(window.Word(60u))+1201u)==1u,
        "Action resume refresh uses navigation subobject offsets and actual DLL highlight byte");
    window.SetVisible(0u,false);check(active.Size()==count && active.GetLast()==previous,"actual Action display returns previous global input owner");
    // Reversible empty-list case keeps original storage ownership and does not
    // create a learned spell or invoke selection. Restore before any retirement.
    std::vector<std::pair<std::uint32_t,std::uint32_t>> ends;
    for(auto* s:skills)for(unsigned i=0;i<5u;++i){const auto p=s->ActorAddress()+920u+16u*i;ends.emplace_back(p+8u,word(p+8u));restore.Save(p+8u);word(p+8u)=word(p+4u);}
    check(!availability.HasAvailableMagic(),"empty first five arrays across original first roster disables Magic");
    window.SetVisible(actor,true);check(disabled(reinterpret_cast<void*>(window.Button(3u)),false),"actual DLL Magic button is disabled for empty actual roster collections");window.SetVisible(0u,false);
    for(auto [p,v]:ends)word(p)=v;
    check(availability.HasAvailableMagic()==actual_magic,"original mastered magic data restored after empty-roster probe");
    // Collection-shape fixture only: HasAnyValues checks occupancy, never the
    // element ID. It does not learn/cast a spell or publish replacement storage.
    // Exercise each original category in each actual Player, then restore it.
    std::uint32_t occupancy_fixture{};unsigned positive_cases{};
    for(auto* s:skills)for(unsigned i=0;i<5u;++i){
        const auto p=s->ActorAddress()+920u+16u*i;
        const std::array saved{word(p+4u),word(p+8u),word(p+12u)};
        for(unsigned j=1u;j<4u;++j)restore.Save(p+4u*j);
        const auto data=reinterpret_cast<std::uint32_t>(&occupancy_fixture);
        word(p+4u)=data;word(p+8u)=data+4u;word(p+12u)=data+4u;
        check(s->HasMasteredMagic() && availability.HasAvailableMagic(),"native first-roster query finds every original category and releases early-return cursor");
        window.SetVisible(actor,true);check(!disabled(reinterpret_cast<void*>(window.Button(3u)),false),"actual DLL Magic button enables with reversible nonempty collection shape");window.SetVisible(0u,false);
        for(unsigned j=1u;j<4u;++j)word(p+4u*j)=saved[j-1u];++positive_cases;
    }
    check(availability.HasAvailableMagic()==actual_magic && players.ActiveIterators()==cursors,"all positive occupancy fixtures restore original collection ownership and cursor state");
    bool rejected{};try{(void)availability.Property(monster,0u);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"Monster cannot be used as a Player command context");
    rejected=false;try{(void)availability.Property(actor,6u);}catch(const std::out_of_range&){rejected=true;}check(rejected,"invalid command cannot alias normal attack");
    restore.Save(actor+800u);const auto property=word(actor+800u);word(actor+800u)=0u;
    rejected=false;try{(void)availability.Property(actor,0u);}catch(const std::logic_error&){rejected=true;}word(actor+800u)=property;
    check(rejected && players.ActiveIterators()==cursors && availability.Property(actor,0u)==static_cast<std::uint8_t>(skills.front()->CommandAvailability(0u)),"real missing-property exception releases first cursor and permits recovery");
    check(active.Size()==count && active.GetLast()==previous && players.ActiveIterators()==cursors,"production Action availability preserves global input and roster cursor ownership");
    std::cout<<"Native Combat Action availability: Players="<<skills.size()<<" property_checks="<<checks<<" copied_magic="<<actual_magic<<" positive_shape_cases="<<positive_cases<<" actual global navigation activation/resume/redraw, DLL six-button gating, empty collections, foreign/invalid contexts, real property failure/recovery and zero cursors pass; full controller remains unbound\n";
}
}
