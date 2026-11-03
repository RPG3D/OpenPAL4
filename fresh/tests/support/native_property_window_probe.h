#pragma once
#include "pal4/native_property_window.h"
#include "pal4/native_role_selector.h"
#include "pal4/player_manager_object.h"
#include "pal4/native_property_window_selection.h"
#include "pal4/native_property_window_discard.h"
#include "pal4/native_scene_record_catalog.h"
#include "native_menu_window_probe.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/native_cegui_window_pointer_events.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_inventory_property_reader.h"
#include "pal4/native_ui_collection_objects.h"
#include "pal4/native_effect_scene_session.h"
#include "../../native/ui/cegui_exports_native.h"
#include <rtfsyst.h>
#include <array>
#include <map>
#include <iostream>
#include <tuple>
#include <cctype>
void VerifyNativeCommonDialogLayout(pal4::NativeCeguiUiSession&,const std::filesystem::path&);
namespace pal4::test {
inline void VerifyNativePropertyWindow(const std::filesystem::path& root,NativePkFileManagerRuntime& packages,
    NativePalGameIVTextureStorage& game,NativeGameDatabase& database,RpWorld* world,RwCamera* camera){
    using detail::Export;
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto address=[](const void* p){return reinterpret_cast<std::uint32_t>(p);};
    const auto ptr=[](std::uint32_t a){return reinterpret_cast<void*>(a);};
    struct Filesystem{bool ready{};~Filesystem(){if(ready)RtFSManagerClose();}} files;
    files.ready=RtFSManagerOpen(8)!=FALSE;check(files.ready,"Property owner SDK filesystem");
    NativeCeguiUiSession ui;check(ui.Start(root,800,600),"Property owner real DLL UI");
    check(ui.LoadImageset(root,"gamedata\\ui\\fonts\\fontPuzzle_0.imageset") && ui.InitializeWidgetRendering(root),"Property owner font/widget rendering");
    check(ui.LoadImageset(root,"gamedata\\ui\\imagesets\\OiramLook.imageset") && ui.LoadOriginalMainLayout() && ui.HideMainLayout(),"Property owner fonts/desktop");
    VerifyNativeMenuWindow(ui,camera);
    NativeInventorySession inventory;(void)inventory.Start();inventory.BindDatabase(database);inventory.BindItemObserver([](auto){});
    const auto& table=database.Tables().at("Property");
    const auto id_column=std::find_if(table.fields.begin(),table.fields.end(),[](const auto& f){return f.name=="ID";})-table.fields.begin();
    for(const auto& row:table.rows)check(inventory.AddItem(std::get<std::uint32_t>(row[id_column]),1)!=0u,"Property owner original inventory");
    NativeInventoryPropertyReader reader(inventory);
    CollectionState collection;NativeUiCollectionObjects objects(collection,world,root,packages,game);
    std::array<std::uint32_t,0x224u/4u> manager{};
    NativeEffectSceneSession effects(root,packages);effects.BindUiEffects(address(manager.data()));effects.BindUiCamera(address(camera));
    struct Retire{NativeEffectSceneSession& effects;~Retire(){try{effects.ClearUiEffects();effects.UnbindUiEffects();}catch(...){}}} retire{effects};
    NativePropertyWindow* page{};
    constexpr std::uint32_t parent_activity=0xabcdu;
    NativeEventHandlerArray activity([&](auto member,auto slot){
        if(member==parent_activity)return 1u;
        check(page && page->OwnsActivity(member),"Property owner active callback identity");return page->DispatchActiveCallback(member,slot);
    },[&](auto member,auto slot,auto event)->std::uint8_t{
        if(member==parent_activity)return 0u;
        check(page && page->OwnsActivity(member),"Property owner active predicate identity");return page->DispatchActivePredicate(member,slot,event);
    });
    (void)activity.Insert(parent_activity);
    std::vector<std::tuple<std::uint32_t,std::uint32_t,std::uint32_t>> events;
    std::vector<std::string> commands;unsigned tutorial{},selection{};std::uint32_t expected_argument=17u;
    NativePropertyWindowDiscard* discard_owner{};
    NativeSceneRecordCatalog scene_records(database);
    bool can_start=true;std::pair<std::string,std::string> scene_key{"q01","n01"};
    NativePropertyWindowSelectionServices actions;
    actions.can_start_action=[&]{return can_start?1:0;};actions.scene_key=[&]{return scene_key;};
    actions.command=[&](std::string_view name,auto item){check(page!=nullptr,"Property selected command page owner");page->DispatchCommand(name,item);};
    NativePropertyWindowSelection choices(ui,activity,inventory,database,scene_records,std::move(actions));
    PropertyWindowState* selection_image{};
    NativePropertyWindowServices services;
    services.event=[&](auto key,auto a,auto b){events.emplace_back(key,a,b);};
    services.tutorial=[&](auto key){check(key==2u,"Property original help context");++tutorial;};
    services.selection=[&](auto& state,auto position){check(page && &state==&page->State() && position,"Property required action selection endpoint");
        const auto* xy=reinterpret_cast<const float*>(position);check(xy[0]==400.0f && xy[1]==100.0f,"Property keyboard action original position");
        selection_image=&state;choices.Show(state,position);++selection;};
    services.command=[&](std::string_view name,auto& state,auto argument){check(page && &state==&page->State() && argument==expected_argument,"Property command endpoint");
        commands.emplace_back(name);if(discard_owner && name=="discard_property")check(discard_owner->Show(state,argument),"Property discard receives live menu entry");};
    NativePropertyWindow owned(ui,root,inventory,objects,effects,packages,activity,address(manager.data()),std::move(services));page=&owned;
    const auto owner=page->FrameAddress(),nav=owner+0x28u,rows=owner+0xb0u;
    const auto module=GetModuleHandleA("CEGUIBase.dll");
    check(activity.Size()==1u && !objects.Size() && !effects.UiActiveCount(),"unshown Property owner has no activity/model payload");
    check(page->State().Read(0u)==PropertyWindowLifecycle::kObjectVtable && page->State().Read(40u)==PropertyWindowLifecycle::kMemberVtable &&
        page->State().Read(180u)==owner,"Property original lifecycle initializes typed native members");
    const auto row_count=[&]{return Export<std::uint32_t (__thiscall*)(const void*)>(module,"?getRowCount@MultiColumnList@CEGUI@@QBEIXZ")(ptr(page->State().Read(88u)));};
    const auto key=[&](std::uint32_t code,bool shortcut=false){std::array<std::uint32_t,8> event{};event[4]=code;
        const auto a=address(event.data());return shortcut?activity.Contains(a):activity.DispatchKeyDown(activity.GetLast(),a)!=0u;};
    const auto render=[&]{RwRGBA black{0,0,0,255};check(RwCameraClear(camera,&black,rwCAMERACLEARIMAGE|rwCAMERACLEARZ) && RwCameraBeginUpdate(camera),"Property owner UI camera begin");
        check(ui.Render(),"Property owner actual DLL frame");RwCameraEndUpdate(camera);};
    const auto click=[&](std::uint32_t window){
        auto* system=ptr(ui.SystemAddress());
        auto* sheet=Export<void* (__thiscall*)(const void*)>(module,"?getGUISheet@System@CEGUI@@QBEPAVWindow@2@XZ")(system);
        const auto origin=NativeCeguiWindowPointerEvents(module).AbsolutePosition(window);
        const auto width=*reinterpret_cast<const float*>(window+268u),height=*reinterpret_cast<const float*>(window+272u);
        CeguiPoint position{};bool found{};
        for(unsigned y=1;y<16u && !found;++y)for(unsigned x=1;x<16u && !found;++x){
            position={origin.x+width*static_cast<float>(x)/16.0f,origin.y+height*static_cast<float>(y)/16.0f};
            found=Export<void* (__thiscall*)(const void*,const CeguiPoint*)>(module,
                "?getChildAtPosition@Window@CEGUI@@QBEPAV12@ABVVector2@2@@Z")(sheet,&position)==ptr(window);
        }
        check(found,"Property category has an actual DLL hit-test point");
        (void)Export<bool (__thiscall*)(void*,float,float)>(module,"?injectMousePosition@System@CEGUI@@QAE_NMM@Z")(system,position.x,position.y);
        (void)Export<bool (__thiscall*)(void*,int)>(module,"?injectMouseButtonDown@System@CEGUI@@QAE_NW4MouseButton@2@@Z")(system,0);
        (void)Export<bool (__thiscall*)(void*,int)>(module,"?injectMouseButtonUp@System@CEGUI@@QAE_NW4MouseButton@2@@Z")(system,0);
        page->RethrowEventError();
    };
    for(unsigned cycle=0;cycle<2u;++cycle){
        page->SetVisible(true);page->RethrowEventError();
        render();
        check(activity.Size()==3u && activity.GetLast()==rows && page->State().Read(108u)==5u && row_count()==reader.RecentSequences().size(),
            "Property shows recent category and original two-layer activity over parent");
        check(events.size()>=3u && std::get<0>(events[events.size()-3u])==0x8b1394u &&
            std::get<0>(events[events.size()-2u])==0x8b13a0u && std::get<0>(events.back())==0x8b13a4u,"Property original visibility broadcasts");
        for(unsigned category=0;category<6u;++category){
            render();
            click(page->State().Read(64u+category*4u));
            std::cout<<"Property click category="<<category<<" actual="<<page->State().Read(108u)<<" last="<<activity.GetLast()<<" expected="<<rows
                <<" tag="<<address(Export<void* (__thiscall*)(const void*)>(module,"?getUserData@Window@CEGUI@@QBEPAXXZ")(ptr(page->State().Read(64u+category*4u))))<<std::endl;
            check(page->State().Read(108u)==category && activity.GetLast()==rows,"actual DLL category click reaches persistent Property list");
            const auto expected=category==5u?reader.RecentSequences().size():static_cast<std::size_t>(reader.Count(category));
            check(row_count()==expected,"persistent Property category contains actual inventory rows");
        }
        check(key(3u,true) && page->State().Read(108u)==0u,"Property original category shortcut propagates through active array");
        check(row_count()>1u,"Property keyboard hover has original cure rows");
        Export<void (__thiscall*)(void*,std::int32_t)>(module,"?setHoverRowIndex@MultiColumnList@CEGUI@@QAEXH@Z")(ptr(page->State().Read(88u)),0);
        page->RethrowEventError();const auto old_sequence=page->State().Read(116u);
        check(key(208u),"Property down key handled by item layer");page->RethrowEventError();
        check(page->State().Read(116u)!=old_sequence && objects.Size()==2u,"Property keyboard hover updates actual model and keeps pedestal");
        check(key(28u) && selection==cycle+1u,"Property Enter calls required selection endpoint");
        check(choices.Menu().Visible() && choices.Menu().ButtonCount()==2u,"Property Enter opens original use/discard popup");
        expected_argument=reader.FindSequence(page->State().Read(116u))->address;
        render();click(choices.Menu().Button(1u));choices.RethrowEventError();
        check(!choices.Menu().Visible() && commands.back()=="discard_property","actual Property popup dispatches through page command catalog");
        expected_argument=17u;
        check(key(1u) && activity.Size()==2u && activity.GetLast()==nav && !objects.Size() && !effects.UiActiveCount(),
            "first Property Esc closes item panel and returns to category layer");
        check(key(208u),"Property category vertical navigation handles down");
        check(key(1u) && activity.Size()==1u && activity.GetLast()==parent_activity && std::get<0>(events.back())==0x8a80acu,
            "second Property Esc detaches page and requests original toolbar return");
        check(!Export<void* (__thiscall*)(const void*)>(module,"?getParent@Window@CEGUI@@QBEPAV12@XZ")(ptr(page->State().Read(36u))),
            "Property hidden page detaches from original DLL parent");
    }
    for(const auto* name:{"use_property","discard_property","after_use_property"})page->DispatchCommand(name,17u);
    check(commands.size()==5u && tutorial==2u,"Property command catalog and help routing survive reopen");
    bool unknown{};try{page->DispatchCommand("missing_property_action",17u);}catch(const std::invalid_argument&){unknown=true;}
    check(unknown,"Property rejects unregistered action command");
    page->SetVisible(true);page->RethrowEventError();
    const std::array<float,2> position{400,100};
    const auto enabled=[&](std::uint32_t index){return !Export<bool (__thiscall*)(const void*,bool)>(module,
        "?isDisabled@Window@CEGUI@@QBE_N_N@Z")(ptr(choices.Menu().Button(index)),false);};
    std::optional<std::size_t> earth_yes,earth_no;
    for(std::size_t i=0;i<scene_records.Records().size();++i){if(scene_records.Image(i)[14u]>0u && !earth_yes)earth_yes=i;
        if(!scene_records.Image(i)[14u] && !earth_no)earth_no=i;}
    check(earth_yes && earth_no,"original Scene table contains both Earthball conditions");
    bool earth_checked{},action_checked{};unsigned selection_records{};
    for(unsigned category=0;category<5u;++category){
        render();click(page->State().Read(64u+category*4u));
        const auto count=row_count();
        for(unsigned row=0;row<count;++row){
            Export<void (__thiscall*)(void*,std::int32_t)>(module,"?setHoverRowIndex@MultiColumnList@CEGUI@@QAEXH@Z")(ptr(page->State().Read(88u)),row);
            page->RethrowEventError();const auto item=reader.FindSequence(page->State().Read(116u));check(item.has_value(),"Property selection live row");
            choices.Show(*selection_image,address(position.data()));
            const auto type=*reinterpret_cast<const std::uint32_t*>(item->definition+4u);
            check(choices.Menu().ButtonCount()==2u && enabled(1u)==(type!=4u && item->id!=3067u),"original inventory discard eligibility");
            if(item->id!=3067u)check(enabled(0u)==(type==0u || item->id==3182u),"original inventory use eligibility");
            choices.Menu().HideAndCleanup();++selection_records;
            if(item->id==3067u){
                for(const auto index:{*earth_yes,*earth_no}){
                    const auto& original=scene_records.Records()[index];scene_key={original.world,original.scene};
                    for(auto* text:{&scene_key.first,&scene_key.second})for(auto& c:*text)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                    choices.Show(*selection_image,address(position.data()));
                    check(enabled(0u)==(index==*earth_yes) && !enabled(1u),"Earthball matches original case-insensitive Scene pair and signed field");choices.Menu().HideAndCleanup();
                }
                scene_key={"missing_world","missing_section"};choices.Show(*selection_image,address(position.data()));
                check(enabled(0u) && !enabled(1u),"Earthball unmatched Scene preserves original usability");choices.Menu().HideAndCleanup();earth_checked=true;
            }
            if(item->id==3182u || item->id==3067u){can_start=false;choices.Show(*selection_image,address(position.data()));
                check(!enabled(0u),"special item requires live CanStartAction service");choices.Menu().HideAndCleanup();can_start=true;action_checked=true;}
        }
    }
    check(selection_records==table.rows.size() && earth_checked && action_checked,"all original Property selection rows and special gates tested");
    // Leave a real DLL row alive while its inventory entry disappears. Both
    // opening that stale row and dispatching its already-open menu must be safe.
    render();click(page->State().Read(64u));
    Export<void (__thiscall*)(void*,std::int32_t)>(module,"?setHoverRowIndex@MultiColumnList@CEGUI@@QAEXH@Z")(ptr(page->State().Read(88u)),0);
    page->RethrowEventError();const auto removed=reader.FindSequence(page->State().Read(116u));check(removed.has_value(),"Property stale selection fixture");
    choices.Show(*selection_image,address(position.data()));render();const auto old_commands=commands.size();
    (void)inventory.RemoveItem(removed->id,1);click(choices.Menu().Button(1u));choices.RethrowEventError();
    check(commands.size()==old_commands && !choices.Menu().Visible(),"removed inventory command cannot dereference freed item");
    choices.Show(*selection_image,address(position.data()));check(!choices.Menu().Visible() && !choices.Menu().ButtonCount(),"stale DLL hover clears popup without creating choices");
    std::cout<<"PASS: Property selection actual DLL popup records="<<selection_records<<" Scene EARTHBALL/action gates and stale inventory rejection"<<std::endl;
    {
        NativeCommonDialogLayout confirmation(module,ui,root);
        std::vector<std::pair<std::string,std::uint32_t>> sounds;unsigned refreshes{};
        const auto sound=[&](std::string_view file,std::uint32_t mode){sounds.emplace_back(file,mode);};
        const auto refresh=[&](auto& state,auto category,bool preserve){check(&state==selection_image && category==page->State().Read(108u) && !preserve,"discard refreshes original current category");
            ++refreshes;page->RefreshItems(category,preserve);};
        NativePropertyWindowDiscard discard(inventory,database,confirmation,sound,refresh);discard_owner=&discard;
        page->RefreshItems(0u);
        const auto hover=[&](unsigned row){Export<void (__thiscall*)(void*,std::int32_t)>(module,"?setHoverRowIndex@MultiColumnList@CEGUI@@QAEXH@Z")(ptr(page->State().Read(88u)),row);
            page->RethrowEventError();const auto item=reader.FindSequence(page->State().Read(116u));check(item.has_value(),"discard live hover");return *item;};
        const auto modal=[&]{return Export<bool (__thiscall*)(const void*)>(module,"?getModalState@Window@CEGUI@@QBE_NXZ")(ptr(confirmation.RootAddress()));};
        const auto wa07=[&]{return std::count(sounds.begin(),sounds.end(),std::pair<std::string,std::uint32_t>{"wa07.wav",1u});};
        const auto open=[&]{const auto item=hover(0u);expected_argument=item.address;choices.Show(*selection_image,address(position.data()));render();
            click(choices.Menu().Button(1u));choices.RethrowEventError();confirmation.RethrowEventError();
            check(!choices.Menu().Visible() && modal(),"actual discard menu opens shared modal confirmation");return item;};
        const auto answer=[&](bool yes){render();click(confirmation.Window(yes?"BtYes":"BtNo"));confirmation.RethrowEventError();
            check(!modal(),"discard closes confirmation before operation");};
        auto item=hover(0u);(void)inventory.AddItem(item.id,3);page->RefreshItems(0u);
        item=open();const auto count=inventory.ItemCount(item.id),old_rows=row_count();
        check(count>1u,"discard fixture contains a real stack");answer(false);
        check(inventory.ItemCount(item.id)==count && !refreshes && !wa07(),"cancel preserves complete stack without discard sound or refresh");
        item=open();answer(true);
        check(!inventory.ItemCount(item.id) && row_count()+1u==old_rows && refreshes==1u && wa07()==1,
            "confirm removes whole original stack, requests original sound and refreshes actual DLL list");
        const auto prompted=open(),now=hover(1u);check(now.id!=prompted.id,"discard current-hover fixture differs from prompted item");answer(true);
        check(!inventory.ItemCount(now.id) && inventory.ItemCount(prompted.id)==prompted.count && refreshes==2u && wa07()==2,
            "discard confirmation re-resolves current hover exactly as original callback");
        const auto stale=open();(void)inventory.RemoveItem(stale.id,static_cast<std::int32_t>(stale.count));answer(true);
        check(refreshes==2u && wa07()==2 && !discard.Show(*selection_image,stale.address),"missing hovered inventory entry performs no discard action");
        page->RefreshItems(0u);(void)open();discard.Close();discard.Close();check(!modal(),"discard Close retires its own pending confirmation");
        discard_owner=nullptr;
        NativePropertyWindowDiscard replacement_owner(inventory,database,confirmation,sound,refresh);
        item=hover(0u);check(replacement_owner.Show(*selection_image,item.address),"discard replacement fixture");unsigned replacement_calls{};
        const auto before_replacement=confirmation.SelectionGeneration();
        confirmation.Show(U"Replacement prompt",3u,[&](auto selection){check(selection==1u,"replacement selection");++replacement_calls;},sound);
        std::cout<<"Discard replacement generations="<<before_replacement<<"/"<<confirmation.SelectionGeneration()<<" modal="<<modal()<<std::endl;
        check(modal(),"replacement common dialog is modal immediately after Show");
        replacement_owner.Close();check(modal(),"discard owner cannot close another owner's replacement prompt");answer(true);
        check(replacement_calls==1u && inventory.ItemCount(item.id)==item.count,"replacement callback and inventory survive discard owner retirement");
        confirmation.ReleaseSelectionCallback();expected_argument=17u;
        std::cout<<"PASS: actual DLL discard confirmation cancel/full-stack/current-hover/stale/replacement ownership and refresh"<<std::endl;
    }
    {
        PlayerManagerObjectState players{};
        NativeRoleSelector selector(ui,root,players,activity,address(manager.data()));
        const auto magic=reader.FindId(3182u);check(magic.has_value(),"original3182 exists in native inventory");
        (void)inventory.AddItem(3182u,1);
        const auto category=*reinterpret_cast<const std::uint32_t*>(magic->definition+4u);
        page->RefreshItems(category);
        const auto old_events=events.size();page->UseMagicItem();
        check(inventory.ItemCount(3182u)==1u && !objects.Size() && !effects.UiActiveCount() &&
            events.size()==old_events+1u && events.back()==std::tuple{0x8a7898u,1u,0u},
            "3182 actual use retires preview/LH, consumes one and emits original transition");
        selector.Show(3182u,[](auto,auto){});check(selector.Visible(),"3182 pending selector is actually shown");
        page->AfterUseMagicItem(selector);
        check(selector.Visible() && inventory.ReadDword(inventory.Address()+88u+category*4u)==magic->sequence && objects.Size()>=1u && effects.UiActiveCount()==1u,
            "3182 return restores real preview/LH and selects remaining stack");
        page->UseMagicItem();page->AfterUseMagicItem(selector);
        check(!inventory.ItemCount(3182u) && !inventory.ReadDword(inventory.Address()+88u+category*4u) && !selector.Visible(),
            "3182 exhausted return clears recent item and hides selector");
        page->SetVisible(false);const auto hidden_events=events.size(),hidden_objects=objects.Size();
        page->AfterUseMagicItem(selector);
        check(events.size()==hidden_events && objects.Size()==hidden_objects && !effects.UiActiveCount(),
            "3182 after callback ignores a hidden Property page");
        selector.Close();page->SetVisible(true);page->RefreshItems(0u);
        std::cout<<"PASS: original3182 native consume/transition, real preview retirement/restore, remaining/exhausted/hidden return"<<std::endl;
    }
    choices.Close();choices.Close();
    ::VerifyNativeCommonDialogLayout(ui,root);
    check(activity.GetLast()==rows && effects.UiActiveCount()==1u,"Property final Close starts with a live page and LH");
    page->Close();page->Close();check(!page->OwnsActivity(nav) && activity.Size()==1u && !objects.Size() && !effects.UiActiveCount(),
        "persistent Property Close removes subscribers/resources and preserves parent activity");
    check(!page->State().Read(36u) && !page->State().Read(48u) && !page->State().Read(52u) && !page->State().Read(56u),
        "Property destruction clears DLL layout and navigation storage words");
    objects.Clear();check(!objects.OutstandingQueueAllocations(),"persistent Property collection queue release");
    (void)activity.Remove(parent_activity);activity.ReleaseStorage();
    std::cout<<"PASS: native Property lifecycle, actual six-category clicks, two-layer keyboard/Esc, reopen, required action endpoints and cleanup\n";
}
}
