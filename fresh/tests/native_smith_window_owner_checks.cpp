#include "support/native_smith_preview_probe.h"
#include "support/native_cegui_test_click.h"
#include "support/native_exception_trace.h"
#include "pal4/native_smith_window.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/native_common_dialog_layout.h"
#include "pal4/native_effect_scene_session.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/native_game_database.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_inventory_prescriptions.h"
#include "pal4/native_prescription_record_catalog.h"
#include "pal4/native_crafting_requirements.h"
#include "pal4/native_pal_game_iv_texture_storage.h"
#include "pal4/native_pk_file_manager_runtime.h"
#include "pal4/native_ui_collection_objects.h"
#include "pal4/native_player_script_refresh_runtime.h"
#include "pal4/player_manager_object.h"
#include "../native/ui/cegui_exports_native.h"
#include <rwcore.h>
#include <rtfsyst.h>
#include <array>
#include <iostream>
namespace pal4::test {
void VerifyNativeSmithOwner(const std::filesystem::path& root,NativePkFileManagerRuntime& packages,NativePalGameIVTextureStorage& game,
    NativeGameDatabase& db,RpWorld* world,RwCamera* camera){
    NativeExceptionTrace trace;
    auto check=[](bool v,const char* why){if(!v)throw std::runtime_error(why);};auto address=[](const void* p){return reinterpret_cast<unsigned>(p);};
    {struct Directory{std::filesystem::path old=std::filesystem::current_path();~Directory(){std::filesystem::current_path(old);}} dir;
        std::filesystem::current_path(root);check(packages.LoadAllScripts()!=0u,"Smith owner packages");}
    struct Files{bool ready=RtFSManagerOpen(8)!=FALSE;~Files(){if(ready)RtFSManagerClose();}} files;check(files.ready,"Smith owner filesystem");
    NativeCeguiUiSession ui;check(ui.Start(root,800,600) && ui.LoadImageset(root,"gamedata\\ui\\fonts\\fontPuzzle_0.imageset") && ui.InitializeWidgetRendering(root) &&
        ui.LoadImageset(root,"gamedata\\ui\\imagesets\\OiramLook.imageset") && ui.LoadOriginalMainLayout() && ui.HideMainLayout(),"Smith owner original UI");
    std::vector<unsigned> item_events;NativeInventorySession inventory;inventory.BindDatabase(db);(void)inventory.Start();inventory.Reset();inventory.BindItemObserver([&](unsigned id){item_events.push_back(id);});
    NativePrescriptionRecordCatalog recipes(db);NativeInventoryPrescriptions learned(inventory,recipes);NativeCraftingRequirements requirements(recipes,inventory);
    unsigned recipe{},product{};for(const auto& r:recipes.Records())if(!inventory.ReadDword(recipes.Address(r[0])+100u) && !requirements.HasEquipmentMaterial(r[0]) && inventory.HasEquipment(requirements.Product(r[0]))){
        bool self{};const auto record=recipes.Address(r[0]);for(unsigned i=0;i<4u;++i)self|=inventory.ReadDword(record+0x98u+i*4u)==requirements.Product(r[0]);if(!self){recipe=r[0];product=requirements.Product(recipe);break;}}
    check(recipe && product,"Smith owner plain foundry recipe");
    const auto product_type=inventory.ReadDword(inventory.EquipmentDefinitionAddress(product)+0x94u);
    const auto product_count=[&](){unsigned count{};for(unsigned i=0;i<inventory.EquipmentCount(product_type);++i){const auto entry=inventory.EquipmentAt(product_type,i);check(entry!=0u,"Smith owner actual equipment entry address");count+=inventory.ReadDword(entry+0x90u)==product;}return count;};
    NativeBaseObjectMemory memory;PlayerManagerObjectState players{};NativePlayerScriptRefreshRuntime refresh(memory,inventory);
    NativeSmithWindow* current{};NativeEventHandlerArray active([&](auto a,auto slot){check(current && current->OwnsActivity(a),"Smith owner callback route");return current->Callback(a,slot);},
        [&](auto a,auto slot,auto event){check(current && current->OwnsActivity(a),"Smith owner input route");return current->Predicate(a,slot,event);});
    std::array<unsigned,0x224u/4u> manager{};NativeEffectSceneSession effects(root,packages);effects.BindUiEffects(address(manager.data()));effects.BindUiCamera(address(camera));
    struct Retire{NativeEffectSceneSession& e;~Retire(){try{e.ClearUiEffects();e.UnbindUiEffects();}catch(...){}}} retire{effects};
    CollectionState collection;NativeUiCollectionObjects objects(collection,world,root,packages,game);NativeCommonDialogLayout dialog(GetModuleHandleA("CEGUIBase.dll"),ui,root);
    std::vector<std::string> sounds;std::vector<unsigned> events,tutorials;bool smith_active{},attempt_close{};unsigned deferred_closes{};
    std::cerr<<"Smith owner constructing"<<std::endl;
    NativeSmithWindow page(ui,root,inventory,players,memory,refresh,db,recipes,objects,effects,packages,active,address(manager.data()),game,dialog,
        NativeSmithWindowServices{[&](unsigned key,unsigned first,unsigned){events.push_back(key);if(current && !first && (key==0x8a80b4u || key==0x8b0f1cu))current->SetVisible(false);},
            [&](unsigned id){tutorials.push_back(id);if(attempt_close){try{current->Close();}catch(const std::logic_error&){++deferred_closes;}}},[&](std::string_view name,unsigned){sounds.emplace_back(name);},[&](){return smith_active;},[&](bool v){smith_active=v;},
            [](std::string_view){return 0u;}});current=&page;
    const auto parent=detail::Export<void* (__thiscall*)(const void*)>(GetModuleHandleA("CEGUIBase.dll"),"?getParent@Window@CEGUI@@QBEPAV12@XZ");
    auto prepare=[&](){inventory.Reset();(void)learned.Add(recipe);(void)inventory.UpdateCounter(1000);const auto record=recipes.Address(recipe);
        for(unsigned i=0;i<4u;++i){const auto id=inventory.ReadDword(record+0x98u+i*4u),amount=inventory.ReadDword(record+0xa8u+i*4u);if(static_cast<int>(amount)>0)(void)inventory.AddItem(id,static_cast<int>(amount));}};
    std::array<unsigned,5> key{};key[4]=28u;
    for(bool system:{false,true}){std::cerr<<"Smith owner mode="<<system<<std::endl;prepare();smith_active=system;page.SetVisible(true);const auto frame=page.FrameAddress();
        check(parent(reinterpret_cast<void*>(page.RootAddress())) && active.GetLast()==frame+0xe8u && objects.Size()==2u,"unified Smith shows actual page and preview");
        const auto before=product_count();check(active.DispatchKeyDown(frame+0xe8u,address(key.data()))==1u,"unified prescription Enter reaches original confirmation");
        ClickNativeCeguiWindow(ui,dialog.Window("BtNo"));dialog.RethrowEventError();page.RethrowEventError();
        check(product_count()==before && inventory.ReadDword(inventory.Address()+0x134u)==1000u,"unified No preserves real inventory/money");
        check(active.DispatchKeyDown(frame+0xe8u,address(key.data()))==1u,"unified prescription Enter reopens confirmation");
        ClickNativeCeguiWindow(ui,dialog.Window("BtYes"));dialog.RethrowEventError();page.RethrowEventError();
        check(product_count()==before+1u && inventory.ReadDword(inventory.Address()+0x134u)==(system?1000u:920u),"unified Yes executes real foundry and original normal/System fee");
        ClickNativeCeguiWindow(ui,dialog.Window("BtOk"));dialog.RethrowEventError();page.RethrowEventError();
        check(active.GetLast()==frame+0xe8u,"unified result OK restores recipe activity");
        key[4]=1u;check(active.DispatchKeyDown(frame+0xe8u,address(key.data()))==1u && active.GetLast()==frame+0x28u,"unified recipe ESC restores tabs");
        check(active.DispatchKeyDown(frame+0x28u,address(key.data()))==1u && !parent(reinterpret_cast<void*>(page.RootAddress())) && !active.Size() && !objects.Size(),"unified final ESC receiver detaches page and preview");key[4]=28u;
    }
    prepare();smith_active=true;page.SetVisible(true);attempt_close=true;ClickNativeCeguiWindow(ui,page.FrameWord(0x64u));page.RethrowEventError();attempt_close=false;
    std::cerr<<"Smith owner callback retirement rejected="<<deferred_closes<<" root="<<page.RootAddress()<<" activity="<<active.Size()<<std::endl;
    check(deferred_closes==1u && page.RootAddress() && active.Size(),"unified owner rejects retirement inside DLL callback before releasing children");
    page.SetVisible(false);smith_active=false;page.SetVisible(true);check(active.DispatchKeyDown(page.FrameAddress()+0xe8u,address(key.data()))==1u,"unified pending prompt");
    const auto saved=product_count();page.SetVisible(false);check(!parent(reinterpret_cast<void*>(dialog.RootAddress())) && product_count()==saved,"hide cancels only owned prompt before page retirement");
    page.SetVisible(true);check(active.DispatchKeyDown(page.FrameAddress()+0xe8u,address(key.data()))==1u,"unified prompt before foreign replacement");
    unsigned foreign_calls{};dialog.Show(U"foreign",4u,[&](unsigned){++foreign_calls;},[&](std::string_view name,unsigned){sounds.emplace_back(name);});
    page.SetVisible(false);check(parent(reinterpret_cast<void*>(dialog.RootAddress())) && !foreign_calls,"hiding Smith preserves newer foreign dialog and callback");dialog.Close();dialog.ReleaseSelectionCallback();
    page.Close();page.Close();current=nullptr;
    check(!page.RootAddress() && !active.Size() && !objects.Size() && !effects.UiActiveCount() && !item_events.empty() && !sounds.empty() && !tutorials.empty(),"unified owner retires resources and preserves actual event/audio endpoints");
    active.ReleaseStorage();objects.Clear();effects.ClearUiEffects();effects.UnbindUiEffects();
    check(!active.OutstandingAllocations() && !objects.OutstandingQueueAllocations() && !packages.FilesystemAccessDepth(),"unified owner balanced borrowed resource scopes");
    std::cout<<"PASS unified native Smith: normal/System Enter-No-Yes-real-foundry-result-ESC, owned prompt cancellation and lifetime\n";
}
}
