#include "support/native_magic_window_presentation_probe.h"
#include "support/native_cegui_frame_session.h"
#include "support/native_cegui_test_click.h"
#include "support/native_magic_window_stunt_probe.h"
#include "pal4/native_magic_window_cast_action.h"
#include "pal4/native_magic_window_point_action.h"
#include "pal4/native_magic_window_layout.h"
#include "pal4/native_magic_window_records.h"
#include "pal4/native_magic_window_presentation.h"
#include "pal4/native_game_database.h"
#include "pal4/native_role_record_image.h"
#include "pal4/native_player_slot_copy_runtime.h"
#include "pal4/native_player_script_property_runtime.h"
#include "pal4/native_player_hud_stats_data.h"
#include "pal4/native_player_skill_runtime.h"
#include "pal4/native_player_skill_callbacks.h"
#include "pal4/native_player_script_refresh_runtime.h"
#include "pal4/native_skill_record_catalog.h"
#include "pal4/native_skill_system_owner.h"
#include "pal4/native_player_combat_events.h"
#include "pal4/native_role_selector.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/native_inventory_session.h"
#include "pal4/random_number_generator.h"
#include "pal4/player_manager_slot.h"
#include "pal4/indexed_key_value_collection.h"
#include "pal4/indexed_sorted_value_arrays.h"
#include "../native/ui/cegui_exports_native.h"
#include <cstring>
#include <iostream>
namespace pal4::test {
void VerifyNativeMagicWindowCastAction(NativeCeguiUiSession& ui,const std::filesystem::path& root,const NativeGameDatabase& db,NativeFrameSession& frame,bool stunt_mode){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};const auto addr=[](const void* p){return reinterpret_cast<unsigned>(p);};
    const auto word=[](unsigned a){unsigned v;std::memcpy(&v,reinterpret_cast<const void*>(a),4u);return v;};
    NativeMagicWindowRecords records(db);NativeSkillRecordCatalog catalog(db);NativeBaseObjectMemory memory;
    PlayerManagerObjectState players{};NativeRoleRecordCatalog roles(db);NativePlayerSlotCopyRuntime copier;
    for(unsigned i=0;i<4u;++i){const auto p=addr(&players.slots[i]);(void)PlayerManagerSlotDataCopier::Copy(p,roles.Address(i),copier);
        memory.WriteDword(p+0x654u,PlayerManagerSlot::kScriptVtable);memory.WriteDword(p+0xb08u,i==2u?0u:1u);
        constexpr std::array<unsigned,4> order{2u,0u,3u,1u};
        (void)NativePlayerScriptPropertyRuntime::Set(p+0x654u,0x18u,0u,order[i]);
        (void)NativePlayerScriptPropertyRuntime::Set(p+0x654u,0x44u,4u,99u);}
    NativeInventorySession inventory;(void)inventory.Start();inventory.BindDatabase(db);inventory.BindItemObserver([](auto){});
    NativePlayerScriptRefreshRuntime refresh(memory,inventory);NativePlayerSkillCallbacks callbacks(catalog,refresh);
    RandomNumberGenerator random;random.Seed(123u);NativePlayerCombatEvents events;NativeSkillSystemOwner system(random,events);
    check(system.Construct("H:/PAL4/objs")!=0u,"Magic actual SkillSystem initialization");
    NativePlayerSkillRuntime skills(memory,catalog,callbacks,system);refresh.BindSkills(&skills);
    struct Unbind{NativePlayerScriptRefreshRuntime& r;~Unbind(){r.BindSkills(nullptr);}} unbind{refresh};
    NativeEventHandlerArray activity([](unsigned a,unsigned){if(a!=0xabcdu)throw std::logic_error("Magic cast parent activity");return 1u;},
        [](unsigned a,unsigned,unsigned)->std::uint8_t{if(a!=0xabcdu)throw std::logic_error("Magic cast parent key");return 0u;});(void)activity.Insert(0xabcdu);
    std::array<unsigned,0x224u/4u> manager{};NativeRoleSelector selector(ui,root,players,activity,addr(manager.data()));
    NativeMagicWindowPointAction* current{};NativeMagicWindowLayout layout(ui,root,db,addr(manager.data()),[&](unsigned,unsigned handler,unsigned event){
        if(handler==0x459780u){check(current!=nullptr,"Magic point/cast callback owner");return current->Click(word(event+8u));}return true;});
    NativeMagicWindowPresentation view(layout,players,db,records,catalog);NativePlayerHudStatsData stats(players);
    std::vector<std::string> sounds;unsigned messages{};NativeMagicCastServices cast_services;
    cast_services.sound=[&](std::string_view sound){sounds.emplace_back(sound);};
    cast_services.message=[&](std::string_view text,float x,float y,unsigned style,unsigned flags){
        check(text==*db.FindTableTextById("GameString",0x6bu) && x==0.35f && y==0.4f && style==4u && !flags,"Magic insufficient MP original text and placement");++messages;};
    NativeMagicWindowCastAction cast(layout,players,records,db,selector,skills,std::move(cast_services));
    NativeMagicPointServices point_services;
    point_services.hint=[](auto,auto,auto,auto){throw std::logic_error("cast fixture must not relearn");};
    point_services.sound=[](auto){throw std::logic_error("cast fixture must not request learning sound");};
    point_services.select_magic=[&](unsigned key){check(cast.Select(key),"point action opens real target selector");};
    NativeMagicWindowPointAction point(layout,view,players,memory,records,std::move(point_services));current=&point;
    struct Collections final : IIndexedKeyValueCollectionRuntime,IIndexedSortedValueArrayRuntime {
        NativeBaseObjectMemory& m;explicit Collections(NativeBaseObjectMemory& memory):m(memory){}
        unsigned ReadDword(unsigned a)override{return m.ReadDword(a);}void WriteDword(unsigned a,unsigned v)override{m.WriteDword(a,v);}
        unsigned Allocate(unsigned n)override{return m.Allocate(n);}void Free(unsigned a)override{(void)m.Free(a);}
    } collections(memory);
    if(stunt_mode){
        VerifyNativeStuntActions(ui,frame,db,players,memory,records,layout,inventory,selector,skills,activity);
        cast.Close();selector.Close();layout.Close();refresh.BindSkills(nullptr);system.Destroy();
        check(!memory.OutstandingAllocations() && !system.OwnedAllocations(),"Stunt action fixture persistent memory cleanup");
        (void)activity.Remove(0xabcdu);activity.ReleaseStorage();return;
    }
    std::vector<const MagicDbSkillRecord*> usable;const MagicDbSkillRecord* single{};const MagicDbSkillRecord* party{};
    for(const auto& r:records.Magic()){const auto effect=catalog.Address(r.effect_id);if(!effect || word(effect+4u)!=3u)continue;
        const auto target=word(addr(&r)+0x50u);const auto selection=MagicWindowSkillCatalog::Select(records.Magic(),r.category);
        bool displayed{};for(unsigned i=0;i<selection.VisibleCount();++i)displayed|=records.Magic()[selection.matching_indices[i]].key==r.key;
        std::cout<<"Magic menu key="<<r.key<<" category="<<r.category<<" displayed="<<displayed<<" effect="<<r.effect_id<<" function="<<word(effect+16u)<<" target="<<target<<" cost="<<r.cast_cost<<std::endl;
        if(r.category>=5u || !displayed)continue;
        usable.push_back(&r);if(target==3u){if(!party)party=&r;}else if(!single)single=&r;
        const auto p=addr(&players.slots[0]);IndexedKeyValueCollectionState kv;std::memcpy(&kv,reinterpret_cast<void*>(p+0x938u),sizeof(kv));
        IndexedKeyValueCollection::AddDelta(kv,r.category,r.key,r.threshold_3c,collections);std::memcpy(reinterpret_cast<void*>(p+0x938u),&kv,sizeof(kv));
        IndexedSortedValueArrayState sorted;std::memcpy(&sorted,reinterpret_cast<void*>(p+0x938u),sizeof(sorted));
        (void)IndexedSortedValueArrays::InsertUnique(sorted,r.category,r.key,collections);std::memcpy(reinterpret_cast<void*>(p+0x938u),&sorted,sizeof(sorted));}
    check(single && party && !usable.empty(),"original menu Magic includes single and party casts");
    const auto module=GetModuleHandleA("CEGUIBase.dll");auto* ui_system=detail::Export<void* (__cdecl*)()>(module,"?getSingletonPtr@System@CEGUI@@SAPAV12@XZ")();
    const auto key=[&](unsigned code){(void)detail::Export<bool (__thiscall*)(void*,unsigned)>(module,"?injectKeyDown@System@CEGUI@@QAE_NI@Z")(ui_system,code);
        (void)detail::Export<bool (__thiscall*)(void*,unsigned)>(module,"?injectKeyUp@System@CEGUI@@QAE_NI@Z")(ui_system,code);selector.RethrowEventError();};
    const auto low=[&]{for(unsigned i=0;i<4u;++i){const auto s=addr(&players.slots[i])+0x654u;
        (void)NativePlayerScriptPropertyRuntime::Set(s,0x40u,0u,500u);(void)NativePlayerScriptPropertyRuntime::Set(s,0x40u,2u,500u);
        (void)NativePlayerScriptPropertyRuntime::Set(s,0x44u,7u,1u);(void)NativePlayerScriptPropertyRuntime::Set(s,0x44u,9u,500u);
        (void)NativePlayerScriptPropertyRuntime::Set(s,0x38u,2u,0x3f800000u);(void)NativePlayerScriptPropertyRuntime::Set(s,0x38u,3u,0x3f800000u);}};
    check(ui.BindDialogRootParent(layout.RootAddress(),true),"Magic original page visible for cast");
    const auto show=[&](const MagicDbSkillRecord& r){selector.Hide();layout.State().category_position=r.category+1u;view.UpdateCharacter(0);
        unsigned row=6u;for(unsigned i=0;i<6u;++i)if(word(layout.Panel().At(0xf0u+4u*i)+280u)==r.key)row=i;
        check(row<6u,"Magic cast bound visible original record");check(frame.Begin() && ui.Render() && !frame.ReadPixels().empty(),"Magic cast actual GPU frame");
        ClickNativeCeguiWindow(ui,layout.Panel().At(0xf0u+4u*row));layout.RethrowEventError();selector.RethrowEventError();
        check(selector.Visible() && selector.VisibleCardCount()==3u,"actual point click opens original three-role selector");
        check(word(selector.FrameAddress()+68u)==addr(&r),"selector preserves real Magic record context");};
    unsigned exercised{};
    for(const auto* r:usable){std::cout<<"Casting original Magic="<<r->key<<std::endl;low();show(*r);const auto chosen=selector.CardRole(0u);check(chosen==1u,"Magic selector card order maps to canonical other-role identity");const auto snapshot=players;
        const auto sound_count=sounds.size();key(28u);++exercised;
        check(sounds.size()==sound_count+1u && sounds.back()=="WD001.WAV" && !messages && selector.Visible(),"Magic cast requests success sound and retains selector");
        const auto caster=addr(&players.slots[0])+0x654u;check(stats.PlayerStat(caster,9u)==500-r->cast_cost,"Magic cast deducts MP exactly once");
        const bool all=word(addr(r)+0x50u)==3u;
        for(unsigned i=0;i<4u;++i){const bool targeted=all?i!=2u:i==chosen;const auto p=addr(&players.slots[i])+0x654u;
            const bool changed=stats.PlayerStat(p,7u)>1;check(changed==targeted,"actual Magic effect heals only original selected/active-party targets");
            if(!targeted && i!=0u)check(!std::memcmp(&snapshot.slots[i],&players.slots[i],sizeof(players.slots[i])),"Magic preserves unselected inactive party images");}
        selector.Hide();}
    low();show(*single);const auto caster_script=addr(&players.slots[0])+0x654u;
    key(28u);const auto first_target=selector.CardRole(0u);const auto second_target=selector.CardRole(2u);
    check(first_target!=second_target && second_target!=2u,"Magic role mouse target differs from keyboard target");
    check(frame.Begin() && ui.Render(),"Magic repeated role-choice frame");
    ClickNativeCeguiWindow(ui,selector.CardButton(2u));selector.RethrowEventError();
    check(stats.PlayerStat(caster_script,9u)==500-2*single->cast_cost &&
        stats.PlayerStat(addr(&players.slots[first_target])+0x654u,7u)>1 &&
        stats.PlayerStat(addr(&players.slots[second_target])+0x654u,7u)>1 &&
        stats.PlayerStat(addr(&players.slots[2])+0x654u,7u)==1,
        "repeated keyboard/mouse choices charge once each and heal distinct canonical active roles");
    low();show(*single);const auto before_cancel=players;const auto count=sounds.size();key(1u);
    check(!selector.Visible() && !std::memcmp(&before_cancel,&players,sizeof(players)) && count==sounds.size(),"Magic Esc cancels without MP cost or effect");
    low();show(*single);const auto caster=addr(&players.slots[0])+0x654u;(void)NativePlayerScriptPropertyRuntime::Set(caster,0x44u,9u,0u);
    const auto before_failure=players;key(28u);check(messages==1u && sounds.back()=="WA05.WAV" && !std::memcmp(&before_failure,&players,sizeof(players)),"MP changes after target dialog opens are rechecked with no cast on failure");
    selector.Hide();check(!cast.Select(0xfefefefeu),"missing original Magic does not open selector");
    check(cast.Select(single->key),"Magic selector before replacement");selector.Show(123u,[](auto,auto){});cast.Close();cast.Close();
    check(selector.Visible() && word(selector.FrameAddress()+68u)==123u,"retiring Magic action preserves another owner's replacement selector");selector.Hide();
    check(activity.Size()==1u && activity.GetLast()==0xabcdu,"Magic target dialog restores parent activity on all close paths");
    selector.Close();layout.Close();refresh.BindSkills(nullptr);system.Destroy();
    for(unsigned category=0;category<5u;++category)for(const auto offset:{0u,0x50u}){const auto d=addr(&players.slots[0])+0x938u+offset+category*16u;
        const auto buffer=word(d+4u);if(buffer)(void)memory.Free(buffer);for(unsigned f=4u;f<=12u;f+=4u)memory.WriteDword(d+f,0u);}
    check(!memory.OutstandingAllocations() && !system.OwnedAllocations(),"Magic cast real skill/system/learned allocation cleanup");
    (void)activity.Remove(0xabcdu);activity.ReleaseStorage();
    std::cout<<"Native Magic cast: original menu records="<<exercised<<", actual point/selector/SkillSystem effects, cost, cancellation, late MP gate and ownership pass\n";
}
}
