#include "support/native_smith_preview_probe.h"
#include "pal4/native_smith_window_preview.h"
#include "pal4/native_smith_window_layout.h"
#include "pal4/native_smith_window_lists.h"
#include "pal4/native_smith_window_tabs.h"
#include "pal4/native_smith_window_visibility.h"
#include "pal4/native_smith_window_panel_input.h"
#include "pal4/native_smith_window_confirmation.h"
#include "pal4/native_common_dialog_layout.h"
#include "pal4/native_inventory_prescriptions.h"
#include "support/native_cegui_test_click.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/native_cegui_widget_images.h"
#include "pal4/native_ui_collection_objects.h"
#include "pal4/native_ui_component_resource.h"
#include "pal4/native_effect_scene_session.h"
#include "pal4/native_pk_file_manager_runtime.h"
#include "pal4/native_pal_game_iv_texture_storage.h"
#include "pal4/native_game_database.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_prescription_record_catalog.h"
#include "pal4/native_player_inventory_state_runtime.h"
#include "pal4/player_manager_object.h"
#include "pal4/native_event_handler_array.h"
#include "../native/ui/cegui_exports_native.h"
#include <rwcore.h>
#include <rpworld.h>
#include <rtfsyst.h>
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <iostream>
#include <fstream>
#include <map>
namespace pal4::test {
void VerifyNativeSmithPreview(const std::filesystem::path& root,NativePkFileManagerRuntime& packages,NativePalGameIVTextureStorage& game,
    NativeGameDatabase& db,RpWorld* world,RwCamera* camera){
    auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};auto address=[](const void* p){return reinterpret_cast<unsigned>(p);};
    {struct Directory{std::filesystem::path previous=std::filesystem::current_path();~Directory(){std::filesystem::current_path(previous);}} dir;
        std::filesystem::current_path(root);check(packages.LoadAllScripts()!=0u,"Smith preview original packages");}
    struct Files{bool ready=RtFSManagerOpen(8)!=FALSE;~Files(){if(ready)RtFSManagerClose();}} files;check(files.ready,"Smith preview filesystem");
    NativeCeguiUiSession ui;check(ui.Start(root,800,600) && ui.LoadImageset(root,"gamedata\\ui\\fonts\\fontPuzzle_0.imageset") &&
        ui.InitializeWidgetRendering(root) && ui.LoadImageset(root,"gamedata\\ui\\imagesets\\OiramLook.imageset") && ui.LoadOriginalMainLayout() && ui.HideMainLayout(),"Smith preview original UI bootstrap");
    NativeInventorySession inventory;inventory.BindDatabase(db);(void)inventory.Start();inventory.Reset();NativePrescriptionRecordCatalog recipes(db);
    std::vector<unsigned> item_events;inventory.BindItemObserver([&](unsigned id){item_events.push_back(id);});
    NativeBaseObjectMemory memory;PlayerManagerObjectState players{};NativePlayerInventoryStateRuntime conditions(players,memory,inventory);
    std::array<unsigned,0x224u/4u> manager{};NativeEffectSceneSession effects(root,packages);effects.BindUiEffects(address(manager.data()));effects.BindUiCamera(address(camera));
    struct Retire{NativeEffectSceneSession& e;~Retire(){try{e.ClearUiEffects();e.UnbindUiEffects();}catch(...){}}} retire{effects};
    CollectionState collection;NativeUiCollectionObjects objects(collection,world,root,packages,game);
    NativeSmithWindowVisibility* current_visibility{};
    NativeSmithWindowPanelInput* current_panels{};NativeSmithWindowConfirmation* current_confirmation{};
    unsigned owner{},activity_calls{};NativeEventHandlerArray active([&](unsigned a,unsigned slot){if(current_visibility && a==owner+0x28u)return current_visibility->DispatchActivity(a,slot);if(current_confirmation && current_confirmation->OwnsActivity(a))return current_confirmation->Callback(a,slot);check(a==owner+0xe8u && (slot==12u || slot==16u || slot==20u),"Smith preview original constant activity slot");++activity_calls;return 1u;},
        [&](unsigned member,unsigned slot,unsigned event)->std::uint8_t{if(current_visibility && member==owner+0x28u)return current_visibility->DispatchInput(member,slot,event);if(current_panels && (member==owner+0xe8u || member==owner+0xf0u))return current_panels->Predicate(member,slot,event);throw std::logic_error("Smith preview foreign keyboard member");});
    std::unique_ptr<NativeSmithWindowLists> lists;std::unique_ptr<NativeSmithWindowPreview> preview;NativeSmithWindowTabs* current_tabs{};
    NativeSmithWindowLayout layout(ui,root,[&](unsigned,unsigned method,unsigned event){if(current_visibility && current_visibility->FromDllNavigation(method,event))return true;if(current_tabs && current_tabs->FromDll(method))return true;return lists?(lists->FromDllCategory(method) || lists->FromDllHover(method)):false;},
        [&](NativeSmithWindowLayout& page){lists=std::make_unique<NativeSmithWindowLists>(page,inventory,recipes,db,conditions);lists->Reset();},
        [&](){if(preview){preview->Close();preview.reset();}if(lists){lists->Close();lists.reset();}});owner=layout.FrameAddress();
    preview=std::make_unique<NativeSmithWindowPreview>(layout,objects,effects,packages,active,address(manager.data()));
    preview->ShowEffectPanel();check(layout.Word(0xa8u) && effects.UiActiveCount()==1u && active.GetLast()==owner+0xe8u && active.Size()==1u,"original Smith LH and effect activity are real");
    const auto effect_id=layout.Word(0xa4u);preview->ShowEffectPanel();check(layout.Word(0xa4u)==effect_id && effects.UiActiveCount()==1u && active.Size()==1u,"already shown panel retains one LH and activity record");
    (void)effects.UpdateUiEffects(1.0/60.0);
    auto* effect_frame=reinterpret_cast<RwFrame*>(effects.UiEffectFrame({layout.Word(0xa4u),layout.Word(0xa8u)}));check(effect_frame!=nullptr,"Smith LH has native RW frame");
    const auto* effect_matrix=RwFrameGetMatrix(effect_frame);check(std::fabs(effect_matrix->pos.x+59.67f)<0.02f && std::fabs(effect_matrix->pos.y-200.86f)<0.02f &&
        std::fabs(effect_matrix->pos.z-298.26f)<0.02f && std::fabs(RwV3dLength(&effect_matrix->right)-2.2f)<0.02f,"original Smith LH position and scale");
    auto draw=[&](NativeUiComponentResource& object){const auto& sphere=*reinterpret_cast<const RwSphere*>(object.State().Data()+56u);const auto radius=std::max(1.f,sphere.radius);
        const RwV3d eye{sphere.center.x,sphere.center.y,sphere.center.z-radius*3.f};auto* frame=RwCameraGetFrame(camera);RwFrameSetIdentity(frame);RwFrameTranslate(frame,&eye,rwCOMBINEREPLACE);RwFrameUpdateObjects(frame);
        RwCameraSetNearClipPlane(camera,radius*0.01f);RwCameraSetFarClipPlane(camera,radius*10.f);RwRGBA black{0,0,0,255};
        check(RwCameraClear(camera,&black,rwCAMERACLEARIMAGE|rwCAMERACLEARZ) && RwCameraBeginUpdate(camera),"Smith preview GPU begin");
        check(object.Render(address(camera)),"Smith model reaches existing native render");RwCameraEndUpdate(camera);
        auto* image=RwImageCreate(256,256,32);check(image && RwImageAllocatePixels(image) && RwImageSetFromRaster(image,RwCameraGetRaster(camera)),"Smith model GPU readback");unsigned lit{};
        for(int y=0;y<256;++y){const auto* row=reinterpret_cast<const RwRGBA*>(RwImageGetPixels(image)+y*RwImageGetStride(image));for(int x=0;x<256;++x)lit+=(row[x].red || row[x].green || row[x].blue)?1u:0u;}
        if(std::string_view(reinterpret_cast<const char*>(object.State().Read(4u)))=="zza"){
            std::ofstream out("H:/PAL4/objs/M09-smith-preview-20261020-zza.rgba",std::ios::binary);for(int y=0;y<256;++y)out.write(reinterpret_cast<const char*>(RwImageGetPixels(image)+y*RwImageGetStride(image)),256*4);}
        RwImageDestroy(image);return lit;};
    unsigned loaded{},drawn{};std::array<bool,7> categories{};
    for(const auto& row:db.Tables().at("Equip").rows){const auto id=std::get<unsigned>(row[0]),entry=inventory.EquipmentDefinitionAddress(id);if(!entry)continue;
        const auto type=inventory.ReadDword(entry+0x94u);const auto name_ptr=inventory.ReadDword(entry+0xdcu);if(type>=7u || categories[type] || !name_ptr || !*reinterpret_cast<const char*>(name_ptr))continue;
        const std::string name(reinterpret_cast<const char*>(name_ptr));const auto texture_ptr=inventory.ReadDword(entry+0xecu);const std::string texture=texture_ptr?reinterpret_cast<const char*>(texture_ptr):"";
        const auto old=layout.Word(0x9cu);preview->Update(name,texture,type<=2u?"PALWeapon\\":"palObject\\");
        auto* model=objects.Find(layout.Word(0x9cu));auto* flower=objects.Find(layout.Word(0xa0u));check(model && flower && objects.Size()==2u && !objects.Find(old),"Smith replaces old model and owns one pedestal");
        const auto* matrix=RwFrameGetMatrix(RpClumpGetFrame(model->Clump()));check(std::fabs(matrix->pos.x+59.67f)<0.02f && std::fabs(matrix->pos.y-369.86f)<0.02f && std::fabs(matrix->pos.z-298.26f)<0.02f,"Smith original model position");
        const auto* pedestal=RwFrameGetMatrix(RpClumpGetFrame(flower->Clump()));check(std::fabs(pedestal->pos.y-309.86f)<0.02f && std::fabs(RwV3dLength(&pedestal->right)-3.5f)<0.02f,"Smith original flower scale and position");
        const auto before=*matrix;objects.Advance(model->Address(),0.1);check(std::memcmp(&before,RwFrameGetMatrix(RpClumpGetFrame(model->Clump())),sizeof(before))!=0,"Smith model real rotation advances");
        const auto lit=draw(*model);check(lit>20u,"Smith equipment GPU has nonempty pixels");++drawn;++loaded;categories[type]=true;
    }
    check(loaded==7u && drawn==7u,"all original equipment model categories");
    for(const auto* texture:{"ZZA1","ZZA2","ZZA3"}){const auto previous=layout.Word(0x9cu);preview->Update("zza",texture,"PALObject\\");
        auto* model=objects.Find(layout.Word(0x9cu));check(model && layout.Word(0x9cu)!=previous && !objects.Find(previous) && objects.Size()==2u,"original tab model is recreated, not persisted UI state");
        check(draw(*model)>20u,"actual zza tab texture renders");}
    {
        NativeInventoryPrescriptions learned(inventory,recipes);for(const auto& row:recipes.Records())(void)learned.Add(row[0]);
        const auto module=GetModuleHandleA("CEGUIBase.dll");NativeCommonDialogLayout box(module,ui,root);
        NativeSmithWindowConfirmation confirmation(layout,*lists,inventory,recipes,db,box,active,address(manager.data()),
            NativeSmithConfirmationServices{[](){return false;},[](std::string_view,unsigned){throw std::logic_error("unexpected tab dialog sound");},[](SmithWindowTradeCallback,unsigned){throw std::logic_error("unexpected tab crafting callback");}});
        current_confirmation=&confirmation;
        std::vector<unsigned> events,tutorials;
        NativeSmithWindowTabs tabs(layout,*lists,*preview,confirmation,NativeSmithTabServices{
            [&](unsigned key,unsigned first,unsigned second){check(key==0x8b13a4u && !second,"original tab event route");events.push_back(first);},[&](unsigned key){tutorials.push_back(key);}});current_tabs=&tabs;
        check(ui.BindDialogRootParent(layout.RootAddress(),true),"Smith tab fixture root");
        const std::array<std::string_view,3> titles{"rongzhu","duanye","zhuling"},captions{"rongzhutupu","duanyetupu","zhulingtupu"};
        for(unsigned mode=0;mode<3u;++mode){ClickNativeCeguiWindow(ui,layout.Word(0x60u+mode*4u));layout.RethrowEventError();
            check(layout.Word(0xb4u)==mode && objects.Size()==2u && layout.Word(0xa8u),"real DLL tab click loads mode, original zza/pedestal and LH");
            const auto rows=detail::Export<unsigned (__thiscall*)(const void*)>(module,"?getRowCount@MultiColumnList@CEGUI@@QBEIXZ")(reinterpret_cast<void*>(layout.Word(0x44u)));
            check(rows==static_cast<unsigned>(learned.Count(static_cast<int>(mode))),"native tab uses actual acquired recipe category");
            for(unsigned i=0;i<3u;++i)check(detail::Export<bool (__thiscall*)(const void*)>(module,"?isSelected@ButtonBase@CEGUI@@QBE_NXZ")(reinterpret_cast<void*>(layout.Word(0x60u+i*4u)))==(i==mode),"exactly one original tab selected");
            const auto image=detail::Export<const void* (__thiscall*)(const void*)>(module,"?getImage@StaticImage@CEGUI@@QBEPBVImage@2@XZ");
            check(address(image(reinterpret_cast<void*>(layout.Word(0x78u))))==NativeCeguiWidgetImages(module).Image("zi0",titles[mode]) &&
                address(image(reinterpret_cast<void*>(layout.Word(0x70u))))==NativeCeguiWidgetImages(module).Image("zi0",captions[mode]),"real tab title and recipe caption images");
        }
        check(events==std::vector<unsigned>{9u,10u,11u} && tutorials==std::vector<unsigned>{5u,15u,16u},"each original tab emits one correct UI event and tutorial");
        tabs.ResetDisplay();check(!layout.Word(0x9cu) && !layout.Word(0xa0u) && !layout.Word(0xa8u) && !objects.Size() && !*reinterpret_cast<const unsigned char*>(owner+0x80u),"original display reset clears actual models and LH");
        (void)active.Remove(owner+0xe8u); // retire the preceding isolated tab fixture
        {
            const std::array<unsigned,3> original{game.GameDataEntry(16),game.GameDataEntry(17),game.GameDataEntry(18)};
            struct Restore{NativePalGameIVTextureStorage& game;std::array<unsigned,3> values;~Restore(){for(unsigned i=0;i<3u;++i)game.SetGameDataEntry(16+static_cast<int>(i),values[i]);}} restore{game,original};
            bool smith_active{};std::map<std::string,unsigned,std::less<>> frames;
            std::vector<std::array<unsigned,3>> visibility_events;
            std::vector<std::array<unsigned,3>> close_events;
            NativeSmithWindowVisibility* event_target{};
            NativeSmithWindowVisibility visibility(ui,layout,*lists,tabs,*preview,active,address(manager.data()),game,
                NativeSmithVisibilityServices{[&](unsigned key,unsigned first,unsigned second){visibility_events.push_back({key,first,second});
                    if(key==0x8a80b4u || key==0x8b0f1cu || key==0x8a80acu){close_events.push_back({key,first,second});if(event_target && key!=0x8a80acu)event_target->SetVisible(false);}},
                    [&](){return smith_active;},[&](bool v){smith_active=v;},[&](std::string_view name){const auto it=frames.find(name);return it==frames.end()?0u:it->second;}});
            current_visibility=&visibility;event_target=&visibility;
            const auto parent_of=detail::Export<void* (__thiscall*)(const void*)>(module,"?getParent@Window@CEGUI@@QBEPAV12@XZ");
            const auto enabled=detail::Export<bool (__thiscall*)(const void*,bool)>(module,"?isDisabled@Window@CEGUI@@QBE_N_N@Z");
            for(unsigned mask=0;mask<9u;++mask){smith_active=mask==8u;for(unsigned i=0;i<3u;++i)game.SetGameDataEntry(16+static_cast<int>(i),(mask&(1u<<i))?1u:0u);
                visibility_events.clear();visibility.SetVisible(true);
                check(visibility.IsAttached() && parent_of(reinterpret_cast<void*>(layout.RootAddress())) && objects.Size()==2u && layout.Word(0xa8u),"Smith visible attaches real root and creates model/LH");
                check(visibility_events==std::vector<std::array<unsigned,3>>{{0x8b1394u,1u,layout.RootAddress()},{0x8b13a0u,1u,layout.RootAddress()},{0x8b13a4u,9u,0u}},"Smith original visibility event sequence and actual root");
                for(unsigned i=0;i<3u;++i)check(!enabled(reinterpret_cast<void*>(layout.Word(0x60u+i*4u)),false)==(smith_active || (mask&(1u<<i))),"actual GameData 16-18 gates or active Smith unlock all");
                check(active.Size()==2u && active.GetLast()==owner+0xe8u,"Smith navigation then effect activity insertion order");
                active.Update();visibility.SetVisible(true);check(active.Size()==2u,"reopening refreshes original truncated activity stack without duplicates");
                visibility.SetVisible(false);check(!visibility.IsAttached() && !objects.Size() && !layout.Word(0xa8u) && !active.Size(),"actual detach clears models/effect and retires navigation/activity tail");
                check(smith_active==(mask==8u),"original post-detach parent test skips Smith singleton reset");
                visibility.SetVisible(false);check(!active.Size(),"already detached close is idempotent");
            }
            smith_active=true;visibility.SetVisible(true);
            // A different requested detach parent leaves the root attached, so
            // original IsVisible reaches ResetState and the registered-frame path.
            visibility.SetVisible(false,layout.Word(0x84u));check(visibility.IsAttached() && !smith_active && !active.Size(),"still-parented close executes original reset and singleton branch");
            visibility.SetVisible(false);
            std::array<unsigned,5> key{};const auto event=address(key.data());
            for(bool system:{false,true}){smith_active=system;for(int i=16;i<=18;++i)game.SetGameDataEntry(i,1u);visibility.SetVisible(true);
                key[4]=208u;check(active.DispatchKeyDown(owner+0x28u,event)==1u && layout.Word(0x3cu)==1u,"original down key changes live navigation index");
                key[4]=28u;check(active.DispatchKeyDown(owner+0x28u,event)==1u && layout.Word(0xb4u)==1u,"original Enter routes real DLL tab click without OS input");
                key[4]=200u;check(active.DispatchKeyDown(owner+0x28u,event)==1u && layout.Word(0x3cu)==0u,"original up key changes live navigation index");
                for(unsigned mode=0;mode<3u;++mode){key[4]=mode+2u;check(visibility.DispatchInput(owner+0x28u,8u,event)==1u && layout.Word(0xb4u)==mode,"original shortcut predicate selects each actual tab");}
                key[4]=30u;check(!active.DispatchKeyDown(owner+0x28u,event) && !visibility.DispatchInput(owner+0x28u,8u,event),"unhandled keys remain unconsumed");
                close_events.clear();key[4]=1u;check(active.DispatchKeyDown(owner+0x28u,event)==1u && !visibility.IsAttached() && !active.Size() && !objects.Size() && !layout.Word(0xa8u),"ESC clears preview and event receiver detaches actual page/effects/activity");
                const std::vector<std::array<unsigned,3>> expected=system?std::vector<std::array<unsigned,3>>{{0x8b0f1cu,0u,0u},{0x8a80acu,1u,0u}}:std::vector<std::array<unsigned,3>>{{0x8a80b4u,0u,0u}};
                check(close_events==expected,"ESC preserves exact normal/System event order and payload");
            }
            smith_active=false;visibility.SetVisible(true);
            for(unsigned i=0;i<3u;++i){ClickNativeCeguiWindow(ui,layout.Word(0x60u+i*4u));layout.RethrowEventError();
                check(layout.Word(0x3cu)==i && layout.Word(0xb4u)==i,"real DLL mouse-up tracks live navigation index and clicks matching tab");
                for(unsigned j=0;j<3u;++j)if(j!=i)check(!*reinterpret_cast<const unsigned char*>(layout.Word(0x60u+j*4u)+1201u),"real DLL hover clears other navigation highlights");
            }
            // A real list hover must invalidate the equipment selection and hide
            // the previous panel; updating only the recipe text leaves stale UI.
            const auto show=detail::Export<void (__thiscall*)(void*,bool)>(module,"?setVisible@Window@CEGUI@@QAEX_N@Z");
            *reinterpret_cast<unsigned char*>(owner+0x81u)=1u;
            for(const auto off:{0x90u,0x94u,0x98u})show(reinterpret_cast<void*>(layout.Word(off)),true);
            const auto hover=detail::Export<void (__thiscall*)(void*,int)>(module,"?setHoverRowIndex@MultiColumnList@CEGUI@@QAEXH@Z");
            hover(reinterpret_cast<void*>(layout.Word(0x44u)),1);layout.RethrowEventError();
            check(!*reinterpret_cast<const unsigned char*>(owner+0x81u),"original prescription hover hides stale selection");
            for(const auto off:{0x90u,0x94u,0x98u})check(!detail::Export<bool (__thiscall*)(const void*,bool)>(module,"?isVisible@Window@CEGUI@@QBE_N_N@Z")(reinterpret_cast<void*>(layout.Word(off)),false),"prescription hover hides actual equipment panels");
            hover(reinterpret_cast<void*>(layout.Word(0x44u)),-1);layout.RethrowEventError();check(!layout.Word(0xacu),"empty prescription hover keeps selection gate cleared");
            {
                unsigned trade_calls{};NativeSmithWindowPanelInput panels(layout,tabs,confirmation,active,[&](){++trade_calls;return true;});current_panels=&panels;
                tabs.Choose(1u);const auto recipe_list=layout.Word(0x44u),equipment_list=layout.Word(0x40u);
                const auto row=[&](unsigned list){return detail::Export<int (__thiscall*)(void*)>(module,"?getHoverRowIndex@MultiColumnList@CEGUI@@QAEHXZ")(reinterpret_cast<void*>(list));};
                key[4]=200u;check(active.DispatchKeyDown(owner+0xe8u,event)==1u && row(recipe_list)==0,"prescription up clamps at first row");
                key[4]=208u;check(active.DispatchKeyDown(owner+0xe8u,event)==1u && row(recipe_list)==1,"prescription down moves actual DLL hover");
                key[4]=200u;check(active.DispatchKeyDown(owner+0xe8u,event)==1u && row(recipe_list)==0,"prescription up returns to first row");
                inventory.Reset();tabs.Choose(1u);const auto empty_row=row(recipe_list);key[4]=208u;
                check(!detail::Export<void* (__thiscall*)(const void*)>(module,"?getHoverItem@MultiColumnList@CEGUI@@QBEPAVListboxItem@2@XZ")(reinterpret_cast<void*>(recipe_list)) && active.DispatchKeyDown(owner+0xe8u,event)==1u && row(recipe_list)==empty_row,"empty prescription list consumes arrows without selecting a row");
                for(const auto& r:recipes.Records())(void)learned.Add(r[0]);tabs.Choose(1u);
                unsigned chosen{},record{};for(unsigned i=0;i<static_cast<unsigned>(learned.Count(1));++i){const auto r=recipes.Address(learned.At(1,i));unsigned mask_count{};for(unsigned c=0;c<7u;++c)mask_count+=inventory.ReadDword(r+0x7cu+c*4u)>0u;
                    if(mask_count>1u){chosen=i;record=r;break;}}
                check(record!=0u,"original strengthening recipe covers multiple categories");hover(reinterpret_cast<void*>(recipe_list),static_cast<int>(chosen));
                std::array<bool,7> added{};for(const auto& entry:db.Tables().at("Equip").rows){const auto id=std::get<unsigned>(entry[0]),def=inventory.EquipmentDefinitionAddress(id);if(!def)continue;const auto type=inventory.ReadDword(def+0x94u);
                    if(type<7u && !added[type] && inventory.ReadDword(record+0x7cu+type*4u)>0u){(void)inventory.ProcessEquipment(inventory.Address()+0x7cu,id);(void)inventory.ProcessEquipment(inventory.Address()+0x7cu,id);added[type]=true;}}
                (void)inventory.UpdateCounter(10000);for(unsigned i=0;i<4u;++i){const auto id=inventory.ReadDword(record+0x98u+i*4u),amount=inventory.ReadDword(record+0xa8u+i*4u);if(static_cast<int>(amount)>0){if(inventory.PropertyAddress(id))(void)inventory.AddItem(id,static_cast<int>(amount));else for(unsigned n=0;n<amount;++n)(void)inventory.ProcessEquipment(inventory.Address()+0x7cu,id);}}
                key[4]=28u;check(active.DispatchKeyDown(owner+0xe8u,event)==1u && active.GetLast()==owner+0xf0u && *reinterpret_cast<const unsigned char*>(owner+0x81u),"prescription Enter executes real material gates and opens equipment selection activity");
                key[4]=208u;check(active.DispatchKeyDown(owner+0xf0u,event)==1u && row(equipment_list)==1,"equipment down changes real hovered sequence");
                key[4]=200u;check(active.DispatchKeyDown(owner+0xf0u,event)==1u && row(equipment_list)==0,"equipment up returns to first sequence");
                check(active.DispatchKeyDown(owner+0xf0u,event)==1u && *reinterpret_cast<const unsigned char*>(owner+0xf8u),"equipment up from row zero highlights original category arrows");
                key[4]=205u;const auto old_page=layout.Word(0xb0u);check(active.DispatchKeyDown(owner+0xf0u,event)==1u && layout.Word(0xb0u)!=old_page,"highlighted right key clicks real next arrow and changes category page");
                key[4]=203u;check(active.DispatchKeyDown(owner+0xf0u,event)==1u && layout.Word(0xb0u)==old_page,"highlighted left key clicks real previous arrow");
                key[4]=208u;check(active.DispatchKeyDown(owner+0xf0u,event)==1u && !*reinterpret_cast<const unsigned char*>(owner+0xf8u) && row(equipment_list)==0,"down leaves arrow focus and selects first equipment row");
                key[4]=28u;check(active.DispatchKeyDown(owner+0xf0u,event)==1u && trade_calls==1u,"equipment Enter invokes required trade-controller endpoint exactly once");
                key[4]=30u;check(!active.DispatchKeyDown(owner+0xf0u,event),"unhandled equipment key remains unconsumed");
                key[4]=1u;check(active.DispatchKeyDown(owner+0xf0u,event)==1u && active.GetLast()==owner+0xe8u && !*reinterpret_cast<const unsigned char*>(owner+0x81u),"equipment ESC retires only selection level and restores recipe activity");
                check(active.DispatchKeyDown(owner+0xe8u,event)==1u && active.GetLast()==owner+0x28u && !objects.Size() && !layout.Word(0xa8u),"recipe ESC resets actual preview and restores tab navigation level");current_panels=nullptr;
                check(!item_events.empty(),"real fixture inventory additions publish item observer events");
            }
            visibility.SetVisible(false);visibility.Close();visibility.Close();event_target=nullptr;current_visibility=nullptr;
        }
        tabs.Close();tabs.Close();current_tabs=nullptr;confirmation.Close();current_confirmation=nullptr;
    }
    // Recreate a standalone effect to verify independent null-model clearing.
    preview->ShowEffectPanel();preview->Update(std::nullopt);check(!objects.Size() && !layout.Word(0x9cu) && !layout.Word(0xa0u) && effects.UiActiveCount()==1u,"null Smith model clears models independently of LH");
    preview->ReleaseEffect();check(!layout.Word(0xa8u) && layout.Word(0xa4u)==0xffffffffu,"Smith LH handle reset before layout destruction");
    preview->Close();preview->Close();check(!objects.Size() && !active.Size() && !*reinterpret_cast<const unsigned char*>(owner+0x80u),"Smith preview idempotent shutdown clears models/panel/activity");
    preview.reset();lists->Close();lists.reset();layout.Close();active.ReleaseStorage();
    check(!objects.PendingAtomics(),"retired Smith leaves no borrowed collection draw entries");objects.Clear();
    check(!objects.OutstandingQueueAllocations() && !active.OutstandingAllocations() && !packages.FilesystemAccessDepth() && activity_calls,"collection owner releases its retained queue capacity; preview releases activity and package scopes");
    effects.ClearUiEffects();check(!effects.UiActiveCount(),"Smith LH payload drains before UI group retirement");effects.UnbindUiEffects();
    std::cout<<"PASS native Smith preview:7 equipment categories+3 zza tab textures, original transforms/pedestal/LH, actual GPU/rotation, repeat/clear/lifetime\n";
}
}
