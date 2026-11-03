#pragma once
#include "native_property_window_layout.h"
#include "pal4/native_property_window_use.h"
#include "pal4/native_property_window_list.h"
#include "pal4/native_property_window_details.h"
#include "pal4/native_inventory_property_reader.h"
#include "pal4/native_player_skill_runtime.h"
#include "pal4/native_player_skill_callbacks.h"
#include "pal4/native_player_script_refresh_runtime.h"
#include "pal4/native_skill_record_catalog.h"
#include "pal4/native_skill_system_owner.h"
#include "pal4/native_player_combat_events.h"
#include "pal4/random_number_generator.h"
#include <iostream>
namespace pal4::test {
template<class Key>void VerifyNativePropertyUse(NativeCeguiUiSession& ui,const std::filesystem::path& root,
    NativeGameDatabase& database,PlayerManagerObjectState& players,NativeRoleSelector& selector,Key key){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto addr=[](const void* p){return reinterpret_cast<std::uint32_t>(p);};
    const auto word=[](std::uint32_t a){return *reinterpret_cast<const std::uint32_t*>(a);};
    selector.Hide();
    NativeInventorySession inventory;(void)inventory.Start();inventory.BindDatabase(database);inventory.BindItemObserver([](auto){});
    NativeInventoryPropertyReader reader(inventory);NativeBaseObjectMemory skill_memory;
    NativeSkillRecordCatalog records(database);NativePlayerScriptRefreshRuntime refresh(skill_memory,inventory);
    NativePlayerSkillCallbacks callbacks(records,refresh);RandomNumberGenerator random;random.Seed(123u);
    NativePlayerCombatEvents events;NativeSkillSystemOwner system(random,events);check(system.Construct("H:/PAL4/objs")!=0u,"Property use real SkillSystem");
    NativePlayerSkillRuntime skills(skill_memory,records,callbacks,system);refresh.BindSkills(&skills);
    struct Unbind{NativePlayerScriptRefreshRuntime& refresh;~Unbind(){refresh.BindSkills(nullptr);}} unbind{refresh};
    NativePropertyWindowLayout layout(ui,root,true);auto& state=layout.state;
    unsigned model_requests{};NativePropertyWindowDetails details(layout.module,state,inventory,[&](auto&,auto){++model_requests;return 1u;});
    NativePropertyWindowListServices list_services;
    list_services.item_panel=[&](auto& image,bool visible){for(auto offset:{140u,144u,148u})detail::Export<void (__thiscall*)(void*,bool)>(layout.module,
        "?setVisible@Window@CEGUI@@QAEX_N@Z")(reinterpret_cast<void*>(image.Read(offset)),visible);};
    list_services.clear_details=[&](auto&){details.Clear();};
    NativePropertyWindowList list(layout.module,layout.look,state,inventory,std::move(list_services));
    std::vector<std::pair<std::string,std::uint32_t>> sounds;unsigned refreshed{};
    NativePropertyWindowUseServices services;
    services.sound=[&](std::string_view file,auto mode){sounds.emplace_back(file,mode);};
    services.refresh=[&](auto& image,auto category,bool preserve){check(&image==&state && !preserve,"Property use refresh current page");++refreshed;list.Populate(category,preserve);details.RethrowEventError();};
    services.earthball=[](auto&){throw std::logic_error("unexpected Earthball in ordinary Property test");};
    services.magic=[](auto&){throw std::logic_error("unexpected magic item in ordinary Property test");};
    NativePropertyWindowUse use(inventory,players,selector,skills,std::move(services));
    const auto& table=database.Tables().at("Property");const auto id_field=std::find_if(table.fields.begin(),table.fields.end(),[](const auto& f){return f.name=="ID";})-table.fields.begin();
    std::uint32_t single{},party{};std::vector<std::uint32_t> cures;
    for(const auto& row:table.rows){const auto id=std::get<std::uint32_t>(row[id_field]),definition=inventory.PropertyAddress(id);
        if(word(definition+4u)!=0u)continue;const auto skill=word(definition+148u),record=records.Address(skill);
        cures.push_back(id);
        std::cout<<"Property cure id="<<id<<" target="<<word(definition+12u)<<" skill="<<skill<<" function="<<(record?word(record+16u):0u)<<std::endl;
        if(word(definition+12u)==3u){if(!party)party=id;}else if(!single)single=id;
    }
    check(single && party,"original single/party cure definitions");
    NativePlayerHudStatsData stats(players);
    const auto set_low=[&]{for(unsigned i=0;i<4u;++i){const auto script=addr(&players.slots[i])+0x654u;
        players.Word((i*0xb14u+0xb08u)/4u)=i==2u?0u:1u;
        (void)NativePlayerScriptPropertyRuntime::Set(script,68u,7u,1u);(void)NativePlayerScriptPropertyRuntime::Set(script,68u,9u,1u);
        // Explicit UI/effect fixture input: normal HP/MP recovery multipliers.
        (void)NativePlayerScriptPropertyRuntime::Set(script,0x38u,2u,0x3f800000u);
        (void)NativePlayerScriptPropertyRuntime::Set(script,0x38u,3u,0x3f800000u);}};
    const auto open=[&](std::uint32_t id,unsigned count){const auto address=inventory.AddItem(id,static_cast<std::int32_t>(count));list.Populate(0u);
        const auto entry=reader.FindAddress(address);check(entry.has_value(),"Property use live entry");
        check(use.Show(state,entry->address) && selector.Visible(),"ordinary item opens actual role selector");return *entry;};
    set_low();auto selected=open(single,2u);const auto chosen=selector.CardRole(0u),script=addr(&players.slots[chosen])+0x654u;
    key(28u);check(inventory.ItemCount(single)==1u && selector.Visible() && refreshed==1u,"first use consumes one item and keeps selector");
    check(stats.PlayerStat(script,7u)>1 || stats.PlayerStat(script,9u)>1,"real original single cure changes HP or MP");
    check(inventory.ReadDword(inventory.Address()+88u)==selected.sequence,"remaining stack is selected as recent item");
    key(28u);check(!inventory.ItemCount(single) && !selector.Visible() && refreshed==2u && !inventory.ReadDword(inventory.Address()+88u),"final use closes selector and clears recent selection");
    check(sounds.size()==2u && sounds.front()==std::pair<std::string,std::uint32_t>{"WD001.WAV",0u},"Property use requests original sound without audio in test");
    set_low();(void)open(party,1u);key(28u);
    for(unsigned i=0;i<4u;++i){const auto p=addr(&players.slots[i])+0x654u;
        const bool changed=stats.PlayerStat(p,7u)>1 || stats.PlayerStat(p,9u)>1;
        check(changed==(i!=2u),"party cure applies actual effect only to active players");}
    selected=open(single,1u);(void)inventory.RemoveItem(single,1);const auto old_sounds=sounds.size(),old_refreshes=refreshed;key(28u);
    check(!selector.Visible() && sounds.size()==old_sounds && refreshed==old_refreshes,"removed inventory entry closes stale role choice without using it");
    unsigned exercised{};
    for(const auto id:cures){set_low();(void)open(id,1u);std::cout<<"Property use exercise="<<id<<std::endl;key(28u);
        check(!inventory.ItemCount(id) && !selector.Visible(),"every original cure consumes through actual SkillSystem and closes exhausted selector");++exercised;}
    selected=open(single,1u);selector.Show(123u,[](auto,auto){});use.Close();use.Close();check(selector.Visible(),"retiring use request preserves replacement selector owner");selector.Hide();
    list.Clear();details.Close();refresh.BindSkills(nullptr);system.Destroy();check(!system.OwnedAllocations() && !skill_memory.OutstandingAllocations(),"Property use real skill/system cleanup");
    std::cout<<"PASS: actual original single/party Property effects, cure_records="<<exercised<<", one-item consumption, recent selection, real DLL list/role flow and stale ownership"<<std::endl;
}
}
