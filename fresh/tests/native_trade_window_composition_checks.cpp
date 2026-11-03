#include "support/native_system_property_probe.h"
#include "support/native_exception_trace.h"
#include "pal4/native_trade_window.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_inventory_prescriptions.h"
#include "pal4/native_shop_record_catalog.h"
#include "pal4/native_prescription_record_catalog.h"
#include "pal4/native_role_record_image.h"
#include "pal4/native_player_slot_copy_runtime.h"
#include "pal4/player_manager_slot_data_copier.h"
#include "pal4/player_manager_object.h"
#include "pal4/player_manager_slot.h"
#include "pal4/native_player_script_property_runtime.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_skill_record_catalog.h"
#include "pal4/native_player_script_refresh_runtime.h"
#include "pal4/native_player_skill_callbacks.h"
#include "pal4/native_player_skill_runtime.h"
#include "pal4/native_skill_system_owner.h"
#include "pal4/native_player_combat_events.h"
#include "pal4/random_number_generator.h"
#include "pal4/native_ui_collection_objects.h"
#include "pal4/native_effect_scene_session.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/native_game_timer_queue.h"
#include "../native/ui/cegui_exports_native.h"
#include <bit>
#include <iostream>
namespace pal4::test {
namespace {
void Check(bool value,const char* why){if(!value)throw std::runtime_error(why);}
unsigned Address(const void* p){return reinterpret_cast<unsigned>(p);}
unsigned Word(unsigned p){return *reinterpret_cast<const unsigned*>(p);}
}
void VerifyNativeTradeWindowComposition(const std::filesystem::path& root,NativeCeguiUiSession& ui,
    const NativeGameDatabase& database,NativePkFileManagerRuntime& packages,NativePalGameIVTextureStorage& game,
    RpWorld* world,RwCamera* camera){
    NativeExceptionTrace trace;const auto module=GetModuleHandleA("CEGUIBase.dll");
    Check(ui.LoadOriginalMinimapLayout(root) && ui.SetMinimapLayoutVisible(false),"Trade composer original fonts");
    NativeGameDatabase shop_database;Check(shop_database.Load(root,"shopdb.db"),"Trade composer original shops");NativeShopRecordCatalog shops(shop_database);
    NativePrescriptionRecordCatalog recipes(database);NativeInventorySession inventory;inventory.BindDatabase(database);(void)inventory.Start();inventory.Reset();inventory.BindItemObserver([](auto){});(void)inventory.UpdateCounter(1000000);
    PlayerManagerObjectState players{};NativeRoleRecordCatalog roles(database);NativePlayerSlotCopyRuntime copier;
    NativeBaseObjectMemory memory;NativeSkillRecordCatalog skill_records(database);NativePlayerScriptRefreshRuntime refresh(memory,inventory);
    NativePlayerSkillCallbacks callbacks(skill_records,refresh);RandomNumberGenerator random;random.Seed(123u);NativePlayerCombatEvents events;
    NativeSkillSystemOwner skill_system(random,events);Check(skill_system.Construct("H:/PAL4/objs")!=0u,"Trade composer SkillSystem");NativePlayerSkillRuntime skills(memory,skill_records,callbacks,skill_system);refresh.BindSkills(&skills);
    for(unsigned n=0;n<4u;++n){const auto p=Address(&players.slots[n]);(void)PlayerManagerSlotDataCopier::Copy(p,roles.Address(n),copier);memory.WriteDword(p+0x654u,PlayerManagerSlot::kScriptVtable);
        memory.WriteDword(p+0xb08u,1u);(void)NativePlayerScriptPropertyRuntime::Set(p+0x654u,0x44u,4u,99u);(void)refresh.Refresh(p+0x654u);}
    CollectionState collection{};NativeUiCollectionObjects objects(collection,world,root,packages,game);
    std::array<unsigned,0x224u/4u> manager{};NativeEffectSceneSession effects(root,packages);effects.BindUiEffects(Address(manager.data()));effects.BindUiCamera(Address(camera));
    struct RetireEffects{NativeEffectSceneSession& e;~RetireEffects(){try{e.ClearUiEffects();e.UnbindUiEffects();}catch(...){}}} retire_effects{effects};
    std::unique_ptr<NativeTradeWindow> page;
    NativeGameTimerQueue timers([](auto,auto){throw std::logic_error("Trade composer unexpected basic timer");},[&](auto a,auto b,auto c,auto d){Check(bool(page),"Trade composer live timer owner");page->TimerCallback(a,b,c,d);});manager[0x1a0u/4u]=timers.Construct();
    NativeEventHandlerArray activity([&](auto member,auto slot){Check(page && page->OwnsActivity(member),"Trade composer activity owner");return page->Callback(member,slot);},
        [&](auto member,auto slot,auto event){Check(page && page->OwnsActivity(member),"Trade composer key owner");return page->Predicate(member,slot,event);});
    NativeCommonDialogLayout dialog(module,ui,root);unsigned resets{},selector_events{},help_events{},sounds{};bool retire_from_reset{};
    NativeTradeWindowServices services;
    services.sound=[&](auto,unsigned){++sounds;};
    // The global TradeSystem/selector are the next integration boundary. This
    // fixture executes the actual page hide from the synchronous reset service.
    services.reset_trade=[&]{++resets;if(retire_from_reset)page.reset();else page->SetVisible(false);};
    services.event=[&](unsigned key,unsigned a,unsigned b){if(key==0x8b0980u){Check(a==1u && !b,"Trade selector payload");++selector_events;}else{Check(key==0x8b1c30u && a==1u && b==4u,"Trade help payload");++help_events;}};
    page=std::make_unique<NativeTradeWindow>(ui,root,inventory,players,memory,refresh,skills,database,shops,recipes,objects,effects,packages,activity,timers,Address(manager.data()),game,dialog,services);
    auto frame=page->FrameAddress();const auto field=[&](unsigned offset){return Word(frame+offset);};
    const auto rows=[&]{return detail::Export<unsigned (__thiscall*)(const void*)>(module,"?getRowCount@MultiColumnList@CEGUI@@QBEIXZ")(reinterpret_cast<void*>(field(0x28u)));};
    const auto cell=[&](unsigned row,unsigned column){const std::array grid{row,column};return Address(detail::Export<void* (__thiscall*)(const void*,const void*)>(module,"?getItemAtGridReference@MultiColumnList@CEGUI@@QBEPAVListboxItem@2@ABUMCLGridRef@2@@Z")(reinterpret_cast<void*>(field(0x28u)),grid.data()));};
    const auto text=[&](unsigned offset){const auto value=detail::Export<const void* (__thiscall*)(const void*)>(module,"?getText@Window@CEGUI@@QBEABVString@2@XZ")(reinterpret_cast<void*>(field(offset)));return std::string(detail::Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ")(value));};
    const auto key=[&](unsigned code){std::array<unsigned,5> event{};event[4]=code;return activity.DispatchKeyDown(activity.GetLast(),Address(event.data()));};
    const auto fire=[&](unsigned target){std::array<unsigned,5> event{};detail::Export<void (__thiscall*)(void*,void*)>(module,"??0WindowEventArgs@CEGUI@@QAE@PAVWindow@1@@Z")(event.data(),reinterpret_cast<void*>(target));detail::NativeString empty(module,"");
        detail::Export<void (__thiscall*)(void*,const void*,void*,const void*)>(module,"?fireEvent@EventSet@CEGUI@@UAEXABVString@2@AAVEventArgs@2@0@Z")(reinterpret_cast<void*>(target),detail::Export<const void*>(module,"?EventClicked@PushButton@CEGUI@@2VString@2@B"),event.data(),empty.Data());detail::Export<void (__thiscall*)(void*)>(module,"??1WindowEventArgs@CEGUI@@UAE@XZ")(event.data());page->RethrowEventError();};
    const auto balance=[&]{return std::bit_cast<int>(Word(inventory.Address()+0x134u));};
    const auto open=[&](unsigned group){
        unsigned shop{};for(const auto& good:shops.Goods())if(good[1]==group && (group!=2u || recipes.Address(good[2]))){shop=shops.FindShop(shops.Text(Address(&good)+20u));if(shop)break;}
        Check(shop!=0u,"Trade composer original group shop");page->SetVisible(true);Check(page->Event(0x8b0a4cu,1u,0u) && page->Event(0x8b0a48u,group,shop),"Trade composer original selector/data events");
        Check(rows()>0u && activity.Size()==1u && field(0x120u) && field(0x12cu),"Trade composer actual list/activity/flower/LH");
        Check(!text(0x2cu).empty(),"Trade composer page hover reaches actual item details");
    };
    open(1u);const auto property=Word(cell(0u,0u)+0x138u);const auto price=Word(inventory.PropertyAddress(property)+144u);Check(key(205u),"Trade composer actual quantity keyboard");
    Check(text(0x38u)==std::to_string(price),"Trade composer hover and real total owner agree");
    auto before=balance();Check(key(28u),"Trade composer Enter opens real confirmation");const auto generation=dialog.SelectionGeneration();
    Check(!key(28u) && dialog.SelectionGeneration()==generation,"modal dialog blocks background Trade activity");fire(dialog.Window("BtYes"));
    Check(inventory.ItemCount(property)==1u && balance()==before-static_cast<int>(price),"Trade composer confirmed actual property transaction and refreshed page");fire(dialog.Window("BtOk"));
    Check(page->Event(0x8b0a4cu,0u,0u) && page->Event(0x8b0a48u,1u,0u) && rows()==1u,"Trade composer original sell selector and live property inventory");
    before=balance();Check(key(205u) && key(28u),"Trade composer actual quantity and sell confirmation");fire(dialog.Window("BtYes"));
    Check(!inventory.ItemCount(property) && balance()==before+static_cast<int>(price/4u),"Trade composer property sale removes live inventory and restores quarter price");fire(dialog.Window("BtOk"));
    open(1u);
    Check(page->Event(0x8b1860u,0u,0u) && timers.EventCount()>0u,"Trade composer schedules real held quantity");
    open(0u);timers.Update(1.f);Check(!timers.EventCount() && !timers.Count(),"Trade composer group replacement cancels old quantity callbacks");
    const auto equipment=Word(cell(0u,0u)+0x138u);const auto definition=inventory.EquipmentDefinitionAddress(equipment);before=balance();
    Check(field(0x11cu) && objects.Size()==2u,"Trade composer actual equipment preview plus flower");
    Check(key(28u),"Trade composer equipment prompt");fire(dialog.Window("BtYes"));Check(balance()==before-std::bit_cast<int>(Word(definition+376u)),"Trade composer equipment purchase balance");fire(dialog.Window("BtOk"));
    Check(page->Event(0x8b0a4cu,0u,0u) && page->Event(0x8b0a48u,0u,0u) && rows()==1u,"Trade composer actual equipment sale page");
    const auto sequence=Word(cell(0u,0u)+0x138u);before=balance();Check(key(28u),"Trade composer equipment sale confirmation");fire(dialog.Window("BtYes"));
    Check(!inventory.FindInventoryEntry(inventory.Address()+0x7cu,sequence,7u) && balance()==before+std::bit_cast<int>(Word(definition+376u))/4,"Trade composer sells exact equipment instance at quarter price");fire(dialog.Window("BtOk"));
    fire(field(0x4cu));Check(help_events==1u,"Trade composer actual Help button");
    fire(field(0x40u));Check(resets==1u && selector_events==1u && !activity.Size() && !rows() && !field(0x11cu) && !field(0x120u) && !objects.Size(),"Trade composer close button removes input, hides page, clears own models and returns selector");
    open(2u);Check(field(0x11cu)!=0u,"Trade composer reopen recipe preview");const auto recipe=Word(cell(0u,0u)+0x138u);before=balance();
    Check(key(28u),"Trade composer recipe confirmation");fire(dialog.Window("BtYes"));
    Check(NativeInventoryPrescriptions(inventory,recipes).Contains(static_cast<int>(recipe)) && balance()==before-std::bit_cast<int>(Word(recipes.Address(recipe)+120u)),"Trade composer actual learned recipe and money mutation");fire(dialog.Window("BtOk"));
    Check(key(1u),"Trade composer Esc close");Check(resets==2u && selector_events==2u && !activity.Size(),"Trade composer Esc uses same lifecycle");
    page->Close();Check(!page->RootAddress() && !objects.Size(),"Trade composer retires all child owners and layout");
    bool retired{};try{page->SetVisible(true);}catch(const std::logic_error&){retired=true;}Check(retired,"Trade composer retired page rejects reopen");
    page.reset();
    page=std::make_unique<NativeTradeWindow>(ui,root,inventory,players,memory,refresh,skills,database,shops,recipes,objects,effects,packages,activity,timers,Address(manager.data()),game,dialog,services);
    frame=page->FrameAddress();open(1u);retire_from_reset=true;Check(key(1u),"Trade composer callback retirement Esc");
    Check(!page && !activity.Size() && !objects.Size() && resets==3u && selector_events==3u,"Trade composer retains runtime until callback returns and then retires native resources");
    timers.Destroy();activity.ReleaseStorage();inventory.Close();objects.Clear();
    Check(!timers.OutstandingAllocations() && !activity.OutstandingAllocations() && !inventory.OutstandingAllocations() && !objects.OutstandingQueueAllocations(),"Trade composer native ownership balances");
    std::cout<<"PASS: native Trade composition actual original data/pages/input/quantities/details/comparison/model/confirmation/transactions/flower/LH; modal gate, timer cancellation, Help, close/Esc/reopen/retirement; global TradeSystem reset/selector boundary explicit\n";
}
}
