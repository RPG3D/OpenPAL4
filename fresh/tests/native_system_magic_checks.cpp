#include "support/native_system_magic_probe.h"
#include "support/native_cegui_test_click.h"
#include "pal4/native_system_menu_pages.h"
#include "pal4/native_system_toolbar_actions.h"
#include "pal4/native_magic_window_layout.h"
#include "pal4/native_magic_window_records.h"
#include "pal4/native_magic_window_stunt_action.h"
#include "pal4/native_role_selector.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_game_database.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_common_dialog_layout.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/native_game_timer_queue.h"
#include "pal4/native_pal_game_iv_texture_storage.h"
#include "pal4/native_player_script_property_runtime.h"
#include "pal4/native_player_hud_stats_data.h"
#include "pal4/indexed_sorted_value_arrays.h"
#include "pal4/indexed_key_value_collection.h"
#include "../native/ui/cegui_exports_native.h"
#include <cstring>
#include <iostream>
namespace pal4::test {
void VerifyNativeSystemMagicFactory(NativeSystemMenuPages& owner,NativeCeguiUiSession& ui,NativePalGameIVTextureStorage& game,
    PlayerManagerObjectState& players,NativeBaseObjectMemory& memory,NativeInventorySession& inventory,const NativeGameDatabase& db,
    NativeCommonDialogLayout& dialog,NativeEventHandlerArray& active,unsigned& created,unsigned& learning_sounds,
    const std::function<void()>& render,const std::function<unsigned(unsigned)>& key){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};const auto addr=[](const void* p){return reinterpret_cast<unsigned>(p);};
    const auto module=GetModuleHandleA("CEGUIBase.dll");
    NativeGameTimerQueue timers([&](unsigned object,unsigned method){check(method==0x4abe30u && object==ui.HintFrameAddress(),"System learning hint original timer callback");ui.CloseOriginalHint();});
    check(timers.Construct()!=0u,"System Magic owns a real UI hint timer fixture");ui.BindHintTimers(&timers);
    struct UnbindTimers{NativeCeguiUiSession& ui;~UnbindTimers(){try{ui.CloseOriginalHint();}catch(...){}ui.BindHintTimers(nullptr);}} unbind_timers{ui};
    const auto visible=[&](unsigned w){const auto* root=reinterpret_cast<void*>(w);
        return detail::Export<void* (__thiscall*)(const void*)>(module,"?getParent@Window@CEGUI@@QBEPAV12@XZ")(root)!=nullptr &&
            detail::Export<bool (__thiscall*)(const void*)>(module,"?getModalState@Window@CEGUI@@QBE_NXZ")(root);};
    const auto dll_key=[&](unsigned code){auto* system=reinterpret_cast<void*>(ui.SystemAddress());
        (void)detail::Export<bool (__thiscall*)(void*,unsigned)>(module,"?injectKeyDown@System@CEGUI@@QAE_NI@Z")(system,code);
        (void)detail::Export<bool (__thiscall*)(void*,unsigned)>(module,"?injectKeyUp@System@CEGUI@@QAE_NI@Z")(system,code);owner.RethrowError();};
    game.SetGameDataEntry(19u,1u);game.SetGameDataEntry(20u,1u);NativePlayerHudStatsData stats(players);
    const auto player=addr(&players.slots[0]),script=player+0x654u;const auto allocations=memory.OutstandingAllocations();
    for(const auto offset:{4u,0x54u,0xa4u})check(!memory.ReadDword(player+0x938u+offset),"System Magic fixture starts with unlearned collections");
    struct Collections final : IIndexedSortedValueArrayRuntime,IIndexedKeyValueCollectionRuntime {
        NativeBaseObjectMemory& m;explicit Collections(NativeBaseObjectMemory& memory):m(memory){}
        unsigned ReadDword(unsigned a)override{return m.ReadDword(a);}void WriteDword(unsigned a,unsigned v)override{m.WriteDword(a,v);}
        unsigned Allocate(unsigned n)override{return m.Allocate(n);}void Free(unsigned a)override{(void)m.Free(a);}
    } collections(memory);
    NativeMagicWindowRecords records(db);const auto stunt=records.FindStunt(5513u);check(stunt && stunt->kind==2u,"System original menu Stunt fixture");
    FixedSortedValueArrayState fixed;std::memcpy(&fixed,reinterpret_cast<void*>(player+0x938u),sizeof(fixed));FixedSortedValueArray::InsertUnique(fixed,stunt->key,collections);
    std::memcpy(reinterpret_cast<void*>(player+0x938u),&fixed,sizeof(fixed));
    for(unsigned i=0;i<5u;++i)if(stunt->item_ids[i] && inventory.PropertyAddress(stunt->item_ids[i]))(void)inventory.AddItem(stunt->item_ids[i],stunt->item_counts[i]+3);
    (void)NativePlayerScriptPropertyRuntime::Set(script,0x44u,4u,99u);
    const auto low=[&]{
        (void)NativePlayerScriptPropertyRuntime::Set(script,0x40u,0u,1000u);(void)NativePlayerScriptPropertyRuntime::Set(script,0x40u,2u,200u);
        (void)NativePlayerScriptPropertyRuntime::Set(script,0x38u,2u,0x3f800000u);(void)NativePlayerScriptPropertyRuntime::Set(script,0x38u,3u,0x3f800000u);
        (void)NativePlayerScriptPropertyRuntime::Set(script,0x44u,7u,1u);(void)NativePlayerScriptPropertyRuntime::Set(script,0x44u,9u,100u);(void)NativePlayerScriptPropertyRuntime::Set(script,0x44u,8u,100u);};
    owner.Event(0x8a81d0u,0u,0u);check(!owner.Magic() && !created,"absent hidden Magic does not invoke lazy factory");
    owner.Event(0x8a80acu,1u,0u);low();render();ClickNativeCeguiWindow(ui,ui.SystemToolbarButtons()[1]);owner.RethrowError();
    check(owner.Magic() && owner.Magic()->Visible() && created==1u,"actual System toolbar enters production Magic factory");
    auto* page=owner.Magic();const auto identity=page->FrameAddress();
    check(page->Layout().State().category_position==0u,"production Magic opens original Stunt category");
    check(key(28u) && page->Stunts().MenuVisible(),"global Stunt Enter reaches real owned popup through synthetic original event");render();
    ClickNativeCeguiWindow(ui,page->Stunts().MenuButton());owner.RethrowError();check(page->Selector().Visible(),"composed Stunt use reaches owned RoleSelector");
    dll_key(28u);std::cout<<"System Magic Stunt rage="<<stats.PlayerStat(script,8u)<<" HP="<<stats.PlayerStat(script,7u)<<" MP="<<stats.PlayerStat(script,9u)<<" dialog="<<visible(dialog.RootAddress())<<std::endl;
    check(stats.PlayerStat(script,8u)==90 && (stats.PlayerStat(script,7u)>1 || stats.PlayerStat(script,9u)>100),"production page Stunt actually consumes rage and heals");dll_key(1u);
    check(!page->Selector().Visible(),"role Esc returns to composed Magic page");
    check(key(1u) && key(1u) && !page->Visible(),"two global Esc levels return from Magic page to System toolbar");
    check(active.GetLast()==owner.Toolbar().ActiveComponentAddress(),"Magic hide restores original parent toolbar activity");
    for(unsigned cycle=0;cycle<2u;++cycle){
        low();owner.Event(0x8a80acu,1u,0u);render();ClickNativeCeguiWindow(ui,ui.SystemToolbarButtons()[1]);owner.RethrowError();
        check(owner.Magic()==page && page->FrameAddress()==identity && created==1u,"production Magic reuses persistent page across show/hide");
        (void)detail::Export<bool (__thiscall*)(void*,float,float)>(module,"?injectMousePosition@System@CEGUI@@QAE_NMM@Z")(reinterpret_cast<void*>(ui.SystemAddress()),10.0f,10.0f);
        check(key(3u) && page->Layout().State().category_position==1u,"System global shortcut selects Water and native row activity");
        const auto select_row=[&](unsigned id){unsigned row=6u;for(unsigned n=0;n<6u;++n)if(memory.ReadDword(page->Layout().Panel().At(0xf0u+4u*n)+280u)==id)row=n;
            check(row<6u,"System requested original Magic is displayed");render();ClickNativeCeguiWindow(ui,page->Layout().Panel().At(0xc0u+4u*row));owner.RethrowError();
            check(detail::Export<bool (__thiscall*)(const void*)>(module,"?isSelected@ButtonBase@CEGUI@@QBE_NXZ")(reinterpret_cast<void*>(page->Layout().Panel().At(0xc0u+4u*row))),"System native hover selects the requested Magic row");};
        if(!cycle){const auto first_key=memory.ReadDword(page->Layout().Panel().At(0xf0u)+280u);select_row(first_key);
            check(key(28u) && ui.HintVisible() && learning_sounds==1u,"composed Enter learns original first Water prerequisite and shows formatted hint");
            IndexedKeyValueCollectionState spent;std::memcpy(&spent,reinterpret_cast<void*>(player+0x938u),sizeof(spent));
            check(IndexedKeyValueCollection::GetValue(spent,0u,first_key,collections)==records.FindMagic(first_key)->threshold_3c,"System prerequisite learning persists exact original points");
            timers.Update(2.01f);check(!ui.HintVisible(),"original prerequisite hint expires through the real timer queue");
            timers.Update(0.001f);check(!timers.EventCount(),"next native timer update reclaims expired prerequisite hint node");
            select_row(5001u);check(key(28u) && ui.HintVisible() && learning_sounds==2u,"composed Enter learns original healing Magic after its Water prerequisite");
            std::memcpy(&spent,reinterpret_cast<void*>(player+0x938u),sizeof(spent));
            check(IndexedKeyValueCollection::GetValue(spent,0u,5001u,collections)==records.FindMagic(5001u)->threshold_3c,"System healing Magic learning persists exact original points");
            timers.Update(2.01f);check(!ui.HintVisible(),"original healing hint expires through the real timer queue");
            timers.Update(0.001f);check(!timers.EventCount(),"next native timer update reclaims expired healing hint node");
        }else select_row(5001u);
        check(key(28u) && page->Selector().Visible(),"learned Enter reaches real Magic target selector through page input");
        dll_key(28u);check(stats.PlayerStat(script,9u)==75 && stats.PlayerStat(script,7u)>1,"composed page Magic deducts MP once and executes healing");dll_key(1u);
        if(!cycle){check(key(28u) && page->Selector().Visible(),"same-page next cast reopens target choice");(void)NativePlayerScriptPropertyRuntime::Set(script,0x44u,9u,0u);
            const auto before=players;dll_key(28u);check(visible(dialog.RootAddress()) && !std::memcmp(&before,&players,sizeof(players)),"composed late MP failure opens actual CommonDialog with no effect");
            owner.Event(0x8a81d0u,0u,0u);check(!page->Visible() && !page->Selector().Visible() && !visible(dialog.RootAddress()),"hiding page retires owned target choice and failure dialog");
        }else {check(key(1u) && key(1u) && !page->Visible() && learning_sounds==2u,"reopened learned page does not spend or learn twice");}
    }
    owner.Event(0x8a81d0u,1u,0u);page->DispatchCommand("use_stunt",stunt->key);check(page->Selector().Visible(),"registered use_stunt command routes to composed actions");
    dialog.Show(U"Foreign prompt",4u,{},[](auto,auto){});owner.Event(0x8a81d0u,0u,0u);
    check(visible(dialog.RootAddress()) && !page->Selector().Visible(),"page cancellation preserves a replacement common dialog");dialog.Close();dialog.ReleaseSelectionCallback();
    owner.ReleaseWorldPages();check(!owner.Magic(),"System world retirement releases Magic page/actions");
    owner.Event(0x8a81d0u,1u,0u);check(owner.Magic() && owner.Magic()->Visible() && created==2u,"retired Magic factory constructs a new page using persistent learned party");
    owner.Event(0x8a81d0u,0u,0u);owner.ReleaseWorldPages();
    for(const auto offset:{4u,0x54u,0xa4u}){const auto d=player+0x938u+offset;const auto buffer=memory.ReadDword(d);check(buffer!=0u,"composed Magic has persistent learned allocation");(void)memory.Free(buffer);
        for(unsigned n=0;n<12u;n+=4u)memory.WriteDword(d+n,0u);}
    check(memory.OutstandingAllocations()==allocations,"System Magic retirement and persistent party cleanup balance");
    std::cout<<"PASS: production System Magic toolbar/factory/global input, actual learning/hint/MP/rage/SkillSystem, Stunt popup, modal cancellation, persistent reopen and world retirement\n";
}
}
