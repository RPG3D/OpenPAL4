#include "support/native_magic_window_stunt_probe.h"
#include "support/native_cegui_frame_session.h"
#include "support/native_cegui_test_click.h"
#include "pal4/native_magic_window_stunt_action.h"
#include "pal4/native_magic_window_records.h"
#include "pal4/native_magic_window_layout.h"
#include "pal4/native_game_database.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_role_selector.h"
#include "pal4/native_player_skill_runtime.h"
#include "pal4/native_player_script_property_runtime.h"
#include "pal4/native_player_hud_stats_data.h"
#include "pal4/native_skill_record_catalog.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/indexed_sorted_value_arrays.h"
#include "pal4/condition_value_array.h"
#include "pal4/player_manager_object.h"
#include "../native/ui/cegui_exports_native.h"
#include <cstring>
#include <iostream>
namespace pal4::test {
void VerifyNativeStuntActions(NativeCeguiUiSession& ui,NativeFrameSession& frame,const NativeGameDatabase& db,
    PlayerManagerObjectState& players,NativeBaseObjectMemory& memory,const NativeMagicWindowRecords& records,
    NativeMagicWindowLayout& layout,NativeInventorySession& inventory,NativeRoleSelector& selector,NativePlayerSkillRuntime& skills,NativeEventHandlerArray& active){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};const auto addr=[](const void* p){return reinterpret_cast<unsigned>(p);};
    // Use the selector's global activity owner supplied by the common action fixture.
    // NativeMenuWindow must share that exact owner with other popup facades.
    std::vector<std::string> sounds;std::vector<unsigned> messages;NativeMagicCastServices services;
    services.sound=[&](std::string_view value){sounds.emplace_back(value);};
    services.message=[&](std::string_view value,float x,float y,unsigned style,unsigned flags){unsigned id{};
        for(const auto candidate:{0x6cu,0x3bu})if(value==*db.FindTableTextById("GameString",candidate))id=candidate;
        check(id && x==0.5f && y==0.4f && style==4u && !flags,"Stunt original failure message/placement");messages.push_back(id);};
    NativeMagicWindowStuntAction action(ui,active,layout,players,memory,records,db,inventory,selector,skills,std::move(services));
    struct Collections final : IIndexedSortedValueArrayRuntime,IConditionValueArrayRuntime {
        NativeBaseObjectMemory& m;explicit Collections(NativeBaseObjectMemory& memory):m(memory){}
        unsigned ReadDword(unsigned a)override{return m.ReadDword(a);}void WriteDword(unsigned a,unsigned v)override{m.WriteDword(a,v);}
        unsigned Allocate(unsigned n)override{return m.Allocate(n);}void Free(unsigned a)override{(void)m.Free(a);}
    } collections(memory);
    const auto module=GetModuleHandleA("CEGUIBase.dll");auto* system=detail::Export<void* (__cdecl*)()>(module,"?getSingletonPtr@System@CEGUI@@SAPAV12@XZ")();
    const auto key=[&](unsigned code){(void)detail::Export<bool (__thiscall*)(void*,unsigned)>(module,"?injectKeyDown@System@CEGUI@@QAE_NI@Z")(system,code);
        (void)detail::Export<bool (__thiscall*)(void*,unsigned)>(module,"?injectKeyUp@System@CEGUI@@QAE_NI@Z")(system,code);action.RethrowEventError();};
    const auto render=[&]{check(frame.Begin() && ui.Render() && !frame.ReadPixels().empty(),"Stunt real DLL/GPU frame");};
    const auto p=addr(&players.slots[0]),script=p+0x654u;layout.State().selected_character_index=0;layout.State().category_position=0u;
    NativePlayerHudStatsData stats(players);std::array<float,2> position{400.0f,300.0f};unsigned executed{},blocked{},item_costs{},counter_costs{},late_items{};bool late_rage{};
    NativeSkillRecordCatalog catalog(db);
    const auto set_rage=[&](unsigned n){(void)NativePlayerScriptPropertyRuntime::Set(script,0x44u,8u,n);};
    const auto learn=[&](unsigned key){FixedSortedValueArrayState c;std::memcpy(&c,reinterpret_cast<void*>(p+0x938u),sizeof(c));c.values.end=c.values.begin;
        FixedSortedValueArray::InsertUnique(c,key,collections);std::memcpy(reinterpret_cast<void*>(p+0x938u),&c,sizeof(c));};
    const auto disable=[&](unsigned key,bool enabled){ConditionValueArrayState c;std::memcpy(&c,reinterpret_cast<void*>(p+0x938u),sizeof(c));
        (void)ConditionValueArray::SetPresent(c,key,enabled?1u:0u,collections);std::memcpy(reinterpret_cast<void*>(p+0x938u),&c,sizeof(c));};
    for(const auto& row:db.Tables().at("Stunt").rows){const auto id=std::get<unsigned>(row[0]);const auto r=records.FindStunt(id);if(!r || r->kind!=2u)continue;
        const auto effect=catalog.Address(r->effect_id);check(effect!=0u,"original menu Stunt attached Skill exists");
        const auto function=memory.ReadDword(effect+16u),target_type=memory.ReadDword(records.StuntAddress(id)+0x88u);
        std::cout<<"Original menu Stunt="<<id<<" effect="<<r->effect_id<<" function="<<function<<" target="<<target_type<<" rage="<<r->required_stat<<" money="<<r->counter_cost<<" condition="<<r->condition_key<<" special="<<r->special_item_id<<std::endl;
        selector.Hide();inventory.Reset();(void)inventory.UpdateCounter(10000);learn(id);set_rage(100u);
        for(unsigned i=0;i<5u;++i)if(r->item_ids[i] && inventory.PropertyAddress(r->item_ids[i]))(void)inventory.AddItem(r->item_ids[i],r->item_counts[i]+3);
        if(r->special_item_id && inventory.PropertyAddress(r->special_item_id) && !inventory.ItemCount(r->special_item_id))(void)inventory.AddItem(r->special_item_id,1);
        for(unsigned i=0;i<4u;++i){const auto s=addr(&players.slots[i])+0x654u;
            (void)NativePlayerScriptPropertyRuntime::Set(s,0x40u,0u,500u);(void)NativePlayerScriptPropertyRuntime::Set(s,0x40u,2u,500u);
            (void)NativePlayerScriptPropertyRuntime::Set(s,0x44u,7u,1u);(void)NativePlayerScriptPropertyRuntime::Set(s,0x44u,9u,1u);
            (void)NativePlayerScriptPropertyRuntime::Set(s,0x38u,2u,0x3f800000u);(void)NativePlayerScriptPropertyRuntime::Set(s,0x38u,3u,0x3f800000u);}
        if(r->condition_key && !skills.HasSkillKey(script,r->condition_key)){check(!action.CanUse(id),"Stunt current-skill condition gate");++blocked;continue;}
        check(action.CanUse(id),"original learned menu Stunt meets real inventory/rage conditions");
        disable(id,true);check(!action.CanUse(id),"disabled Stunt collection blocks menu");disable(id,false);check(action.CanUse(id),"reenabled Stunt restores eligibility");
        check(action.ShowMenu(addr(position.data()),id) && action.MenuVisible(),"original Stunt popup opens");render();
        const auto disabled=detail::Export<bool (__thiscall*)(const void*,bool)>(module,"?isDisabled@Window@CEGUI@@QBE_N_N@Z")(reinterpret_cast<void*>(action.MenuButton()),false);
        check(!disabled,"original Stunt use button enabled from actual requirements");
        ClickNativeCeguiWindow(ui,action.MenuButton());action.RethrowEventError();check(selector.Visible(),"actual Stunt popup use command opens RoleSelector");
        check(memory.ReadDword(selector.FrameAddress()+68u)==records.StuntAddress(id),"Stunt selector carries original208-byte record address");
        const auto rage=stats.PlayerStat(script,8u),money=inventory.UpdateCounter(0);std::array<unsigned,5> counts{};
        for(unsigned i=0;i<5u;++i)if(inventory.PropertyAddress(r->item_ids[i]))counts[i]=inventory.ItemCount(r->item_ids[i]);
        const auto chosen=selector.CardRole(0u);const auto snapshot=players;
        const auto sound_count=sounds.size(),message_count=messages.size();key(28u);++executed;
        check(messages.size()==message_count && sounds.size()==sound_count+1u && sounds.back()=="WD001.WAV","Stunt success runs actual SkillSystem without failure prompt");
        check(stats.PlayerStat(script,8u)==rage-r->required_stat,"Stunt exact rage cost");
        for(unsigned i=0;i<4u;++i){const auto s=addr(&players.slots[i])+0x654u;const bool targeted=target_type==3u?i!=2u:i==chosen;
            const bool recovered=stats.PlayerStat(s,7u)>1 || stats.PlayerStat(s,9u)>1;
            if(function==9118u)check(recovered==targeted,"actual original Stunt recovery changes precisely selected/active-party HP or MP");
            if(!targeted && i!=0u)check(!std::memcmp(&snapshot.slots[i],&players.slots[i],sizeof(players.slots[i])),"Stunt preserves full non-target party image");}
        check(inventory.UpdateCounter(0)==money-(r->counter_cost>0?r->counter_cost:0),"Stunt exact counter cost");counter_costs+=r->counter_cost>0;
        for(unsigned i=0;i<5u;++i)if(r->item_ids[i] && inventory.PropertyAddress(r->item_ids[i])){
            check(inventory.ItemCount(r->item_ids[i])==counts[i]-r->item_counts[i],"Stunt exact original inventory cost");++item_costs;}
        selector.Hide();
        for(unsigned i=0;i<5u;++i)if(r->item_ids[i] && r->item_counts[i]>0 && inventory.PropertyAddress(r->item_ids[i])){
            set_rage(100u);check(action.Use(id) && selector.Visible(),"Stunt target selector before late inventory change");
            (void)inventory.RemoveItem(r->item_ids[i],inventory.ItemCount(r->item_ids[i]));
            const auto before=players;const auto prompt_count=messages.size();const auto money_before=inventory.UpdateCounter(0);
            key(28u);check(messages.size()==prompt_count+1u && messages.back()==0x3bu && sounds.back()=="WA05.WAV" &&
                !std::memcmp(&before,&players,sizeof(players)) && inventory.UpdateCounter(0)==money_before,
                "Stunt rechecks late inventory loss before rage/cost/effect");selector.Hide();
            check(!action.CanUse(id) && action.ShowMenu(addr(position.data()),id),"missing original material disables next popup");render();
            check(detail::Export<bool (__thiscall*)(const void*,bool)>(module,"?isDisabled@Window@CEGUI@@QBE_N_N@Z")(reinterpret_cast<void*>(action.MenuButton()),false),"actual Stunt material failure disables DLL use button");
            ClickNativeCeguiWindow(ui,action.MenuButton());action.RethrowEventError();check(!selector.Visible(),"disabled material button cannot open target selection");
            key(1u);check(!action.MenuVisible(),"Stunt popup Esc dismisses disabled material choice");
            (void)inventory.AddItem(r->item_ids[i],r->item_counts[i]+3);++late_items;
        }
        if(!late_rage && r->required_stat>0){set_rage(100u);check(action.Use(id) && selector.Visible(),"Stunt selector before late rage change");set_rage(0u);
            const auto before=players;const auto prompt_count=messages.size();key(28u);
            check(messages.size()==prompt_count+1u && messages.back()==0x6cu && sounds.back()=="WA05.WAV" && !std::memcmp(&before,&players,sizeof(players)),
                "Stunt rechecks late rage loss without effect or cost");selector.Hide();late_rage=true;}
        if(r->required_stat>0){set_rage(0u);const auto before=players;const auto failures=messages.size();(void)action.Use(id);
            check(!selector.Visible() && messages.size()==failures+1u && messages.back()==0x6cu && !std::memcmp(&before,&players,sizeof(players)),"Stunt insufficient rage selection has no mutation");set_rage(100u);}
    }
    check(executed==16u && late_items==2u && late_rage,"all original menu Stunts and real material/rage failure paths exercised");
    check(!action.CanUse(0xfefefefeu),"missing Stunt requirement query is false");
    const auto before_missing=players;check(action.ShowMenu(0u,0xfefefefeu) && !action.MenuVisible() && !std::memcmp(&before_missing,&players,sizeof(players)),"missing Stunt record leaves popup and player untouched");
    action.Close();action.Close();selector.Hide();
    for(const auto offset:{0xa4u,0xb4u}){const auto d=p+0x938u+offset;const auto buffer=memory.ReadDword(d);if(buffer)(void)memory.Free(buffer);
        for(unsigned f=0;f<12u;f+=4u)memory.WriteDword(d+f,0u);}
    check(active.Size()==1u,"Stunt popup and role selection preserve parent activity");
    std::cout<<"Native Stunt original effects="<<executed<<" condition-blocked="<<blocked<<" item-cost entries="<<item_costs<<" counter-cost records="<<counter_costs<<std::endl;
}
}
