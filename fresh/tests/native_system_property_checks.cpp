#include "support/native_system_property_probe.h"
#include "support/native_system_magic_probe.h"
#include <rwcore.h>
#include <rpworld.h>
#include <rtfsyst.h>
#include <array>
#include <bit>
#include <climits>
#include <cstring>
#include <iostream>
#include <set>
#include "pal4/native_base_object_memory.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/native_common_dialog_layout.h"
#include "pal4/native_effect_scene_session.h"
#include "pal4/native_equipment_window.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/native_game_database.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_pal_game_iv_texture_storage.h"
#include "pal4/native_pk_file_manager_runtime.h"
#include "pal4/native_save_window.h"
#include "pal4/native_scene_record_catalog.h"
#include "pal4/native_ui_collection_objects.h"
#include "support/native_no_arena_operations.h"
#include "support/native_cegui_test_click.h"
#include "pal4/native_property_window_script.h"
#include "pal4/native_timed_script_stack.h"
#include "pal4/native_system_menu_pages.h"
#include "pal4/dialog_manager_lifecycle.h"
#include "pal4/native_system_toolbar_actions.h"
#include "pal4/native_game_arena_manager_owner.h"
#include "pal4/native_world_loading_state_owner.h"
#include "pal4/native_pal_game_iv_state_list.h"
#include "pal4/native_camera_manager_owner.h"
#include "pal4/native_role_record_image.h"
#include "pal4/player_manager_object.h"
#include "pal4/player_manager_slot.h"
#include "pal4/native_player_hud_stats_data.h"
#include "pal4/native_player_slot_copy_runtime.h"
#include "pal4/player_manager_slot_data_copier.h"
#include "pal4/game_object_property_access.h"
#include "pal4/native_player_script_property_runtime.h"
#include "pal4/native_skill_record_catalog.h"
#include "pal4/native_player_script_refresh_runtime.h"
#include "pal4/native_player_skill_callbacks.h"
#include "pal4/native_player_skill_runtime.h"
#include "pal4/native_skill_system_owner.h"
#include "pal4/native_player_combat_events.h"
#include "pal4/random_number_generator.h"
#include "pal4/native_inventory_property_reader.h"
#include "pal4/native_system_property_page.h"
#include "pal4/native_menu_window.h"
#include "pal4/native_role_selector.h"
#include "pal4/input_manager.h"
#include "pal4/game_render_system.h"
#include "pal4/equipment_item_details.h"
#include "pal4/script_skill_collection.h"
#include "pal4/native_player_field_schema.h"
#include "pal4/inventory_item_slots.h"
#include "pal4/native_prescription_record_catalog.h"
#include "pal4/native_inventory_prescriptions.h"
#include "pal4/native_crafting_requirements.h"
#include "pal4/native_game_script_callbacks.h"
#include "pal4/native_script_frame_runner.h"
#include "pal4/native_script_system_ui_runtime.h"
namespace pal4::test {
void VerifyNativeSystemPropertyFactory(const std::filesystem::path& root,NativePkFileManagerRuntime& packages,
    NativePalGameIVTextureStorage& game,NativeGameDatabase& database,RpWorld* world,RwCamera* camera){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto address=[](const void* p){return reinterpret_cast<std::uint32_t>(p);};
    {struct Directory{std::filesystem::path previous=std::filesystem::current_path();~Directory(){std::filesystem::current_path(previous);}} directory;
        std::filesystem::current_path(root);check(packages.LoadAllScripts()!=0u,"System Property startup package mounts");}
    struct Filesystem{bool ready=RtFSManagerOpen(8)!=FALSE;~Filesystem(){if(ready)RtFSManagerClose();}} fs;
    check(fs.ready,"System Property filesystem");NativeCeguiUiSession ui;check(ui.Start(root,800,600),"System Property UI");
    check(ui.LoadImageset(root,"gamedata\\ui\\fonts\\fontPuzzle_0.imageset") && ui.InitializeWidgetRendering(root),"System Property fonts");
    for(const auto* set:{"OiramLook","zhujiemian_0","biaoti_0","biaoti2_0","biaoti3_0"})
        check(ui.LoadImageset(root,(std::string("gamedata\\ui\\imagesets\\")+set+".imageset").c_str()),"System Property startup imageset");
    check(ui.LoadOriginalMainLayout() && ui.HideMainLayout(),"System Property main layout");
    VerifyNativeShopSelectorLayout(root,ui,database,game,camera);
    VerifyNativeTradeWindowLayout(root,ui);
    VerifyNativeTradeWindowInput(root,ui,camera);
    VerifyNativeTradeWindowConfirmation(root,ui,database);
    game.SetGameDataEntry(13,1u);
    for(const auto story:game.StoryObjects())*reinterpret_cast<std::int32_t*>(story)=INT32_MAX;
    *reinterpret_cast<std::int32_t*>(game.GlobalAddress(0x90dec0u))=INT32_MAX;
    VerifyNativeTradeWindowData(root,ui,database,game);
    VerifyNativeTradeWindowDetails(root,ui,database,packages,game,world,camera);
    VerifyNativeTradeWindowComposition(root,ui,database,packages,game,world,camera);
    NativeGameArenaManagerOwner arena;check(arena.Construct()!=0u,"System Property arena");
    NativeWorldLoadingStateOwner loading;check(loading.Construct()!=0u,"System Property loading");
    NativePalGameIVStateList states(game.Address());check(states.Register(loading.Address(),6u,[&]{loading.Destroy();}),"System Property states");
    NativeCameraManagerOwner cameras;GameRenderState render_state(0x8e31e0u);NoArenaOperations host;
    NativeArenaLoadRuntime transitions(arena,game,states,loading,cameras,render_state,host);
    VerifyNativeShopSelectorWindow(root,ui,database,game,transitions,packages);
    PlayerManagerObjectState players{};NativeRoleRecordCatalog roles(database);NativePlayerSlotCopyRuntime copier;
    const auto player=address(&players.slots[0]),script=player+0x654u;
    (void)PlayerManagerSlotDataCopier::Copy(player,roles.Address(0u),copier);
    GameObjectPropertyState properties{};std::memcpy(&properties,reinterpret_cast<void*>(script),sizeof(properties));
    properties.words[0]=PlayerManagerSlot::kScriptVtable;
    (void)GameObjectPropertyAccess::SetIndexed86(properties,0u,1000u);
    (void)GameObjectPropertyAccess::SetIndexed86(properties,2u,200u);
    (void)GameObjectPropertyAccess::Set167And83(properties,0u);
    std::memcpy(reinterpret_cast<void*>(script),&properties,sizeof(properties));players.Word(0xb08u/4u)=1u;
    (void)NativePlayerScriptPropertyRuntime::Set(script,68u,7u,1u);
    (void)NativePlayerScriptPropertyRuntime::Set(script,0x38u,2u,0x3f800000u);
    NativeInventorySession inventory;(void)inventory.Start();inventory.BindDatabase(database);inventory.Reset();inventory.BindItemObserver([](auto){});
    (void)inventory.AddItem(3000u,2);NativeInventoryPropertyReader reader(inventory);
    NativeBaseObjectMemory memory;NativeSkillRecordCatalog records(database);NativePlayerScriptRefreshRuntime refresh(memory,inventory);
    NativePlayerSkillCallbacks callbacks(records,refresh);RandomNumberGenerator random;random.Seed(123u);NativePlayerCombatEvents events;
    NativeSkillSystemOwner system(random,events);check(system.Construct("H:/PAL4/objs")!=0u,"System Property skills");
    NativePlayerSkillRuntime skills(memory,records,callbacks,system);refresh.BindSkills(&skills);
    InputManager keys;(void)keys.InitializeState();NativeSystemMenuPages* pages{};
    NativeEventHandlerArray activity([&](auto a,auto slot){check(pages && pages->OwnsActivity(a),"System Property callback router");return pages->Callback(a,slot);},
        [&](auto a,auto slot,auto e){check(pages && pages->OwnsActivity(a),"System Property predicate router");return pages->Predicate(a,slot,e);});
    std::array<std::uint32_t,0x224u/4u> manager{};
    CollectionState collection;NativeUiCollectionObjects objects(collection,world,root,packages,game);
    NativeEffectSceneSession effects(root,packages);effects.BindUiEffects(address(manager.data()));effects.BindUiCamera(address(camera));
    struct Retire{NativeEffectSceneSession& e;~Retire(){try{e.ClearUiEffects();e.UnbindUiEffects();}catch(...){}}} retire{effects};
    const auto module=GetModuleHandleA("CEGUIBase.dll");NativeCommonDialogLayout confirmation(module,ui,root);NativeSceneRecordCatalog scenes(database);
    NativeAngelScriptEngine earth_engine;unsigned earth_closed{};
    NativeTimedScriptStack earth_scripts(earth_engine,root,[&](auto source,auto module){
        check(source=="Q01.csb" && module=="PFNSkipScript[EarthBall]","factory EarthBall module source");
        const char* program="void EarthBall(){ global_mvar+=7; }";const std::string name(module);
        check(earth_engine.Raw()->AddScriptSection(name.c_str(),"factory EarthBall",program,static_cast<int>(std::strlen(program)))>=0 &&
            earth_engine.Raw()->Build(name.c_str())>=0,"factory EarthBall SDK fixture build");});
    NativePrescriptionRecordCatalog smith_recipes(database);unsigned smith_created{},smith_sounds{};
    unsigned created{},equipment_created{},magic_created{},magic_learning_sounds{},sounds{},discard_sounds{},equipment_discard_sounds{},tutorials{};DialogManagerState save_globals;NativeSystemMenuPageServices services;
    services.decorations.money=[&]{return std::bit_cast<int>(inventory.ReadDword(inventory.Address()+0x134u));};services.decorations.elapsed_seconds=[]{return 0u;};services.decorations.clock_offset_seconds=[]{return 0u;};
    services.decorations.game_string=[&](auto id){return std::string(*database.FindTableTextById("GameString",id));};
    services.tutorial=[&](auto){++tutorials;};
    services.property=[&]{++created;NativeSystemPropertyServices endpoints;
        endpoints.event=[&](auto k,auto a,auto b){pages->Event(k,a,b);};endpoints.tutorial=[&](auto){++tutorials;};
        endpoints.can_start_action=[]{return 1;};endpoints.scene_key=[]{return std::pair{std::string("q01"),std::string("n01")};};
        endpoints.sound=[&](std::string_view file,auto mode){++sounds;if(file=="wa07.wav" && mode==1u)++discard_sounds;};
        endpoints.earthball=[&](auto&){earth_scripts.DispatchUiScript({},[]{},[&](auto& selected){
            NativePropertyWindowScript::Execute({packages,selected,arena.Address(),address(manager.data()),"Q01.csb",[](auto,auto){},
                [&](auto key,auto a,auto b){check(key==0x8a80b4u && !a && !b,"factory EarthBall close event");++earth_closed;
                    // Isolate the UI close boundary: this fixture rejects arena transitions.
                    pages->Event(0x8a80bcu,0u,0u);}});});};
        endpoints.magic=[](auto& page){page.UseMagicItem();};
        return std::make_unique<NativeSystemPropertyPage>(ui,root,inventory,objects,effects,packages,activity,address(manager.data()),
            players,database,scenes,skills,confirmation,std::move(endpoints));};
    services.magic=[&]{++magic_created;NativeSystemMagicServices endpoints;
        endpoints.event=[&](auto k,auto a,auto b){pages->Event(k,a,b);};endpoints.tutorial=[&](auto){++tutorials;};
        endpoints.sound=[](std::string_view,unsigned){};
        endpoints.learning_sound=[&](std::string_view file){check(file=="gamedata\\palSound\\wa08.wav","System Magic learning uses original full resource path");++magic_learning_sounds;};
        return std::make_unique<NativeSystemMagicPage>(ui,root,inventory,players,database,records,memory,skills,game,activity,address(manager.data()),confirmation,std::move(endpoints));};
    services.equipment=[&]{++equipment_created;NativeEquipmentWindowServices endpoints;
        endpoints.event=[&](auto k,auto a,auto b){pages->Event(k,a,b);};endpoints.tutorial=[&](auto){++tutorials;};
        endpoints.sound=[&](std::string_view file,unsigned mode){check(mode==1u,"System Equipment original sound mode");if(file=="wa07.wav")++equipment_discard_sounds;};
        return std::make_unique<NativeEquipmentWindow>(ui,root,inventory,players,database,refresh,objects,effects,packages,activity,
            address(manager.data()),confirmation,std::move(endpoints));};
    services.smith=[&]{++smith_created;NativeSmithWindowServices endpoints;
        endpoints.event=[&](auto k,auto a,auto b){pages->Event(k,a,b);};endpoints.tutorial=[&](auto){++tutorials;};
        endpoints.sound=[&](std::string_view,unsigned){++smith_sounds;};endpoints.smith_active=[&]{return pages->SmithActive();};
        endpoints.set_smith_active=[&](bool v){pages->SetSmithActive(v);};endpoints.find_frame_root=[&](auto name){return pages->FindFrameRoot(name);};
        return std::make_unique<NativeSmithWindow>(ui,root,inventory,players,memory,refresh,database,smith_recipes,objects,effects,packages,activity,address(manager.data()),game,confirmation,std::move(endpoints));};
    services.save=[&]{NativeSaveWindowServices endpoints;
        endpoints.current_state=[]{return std::pair{0x1234u,2u};};endpoints.capture_raster=[]{throw std::logic_error("System save should not capture raster");};
        endpoints.event=[&](auto k,auto a,auto b){pages->Event(k,a,b);};endpoints.sound=[](auto,auto){};
        endpoints.save_settings=[]{throw std::logic_error("factory return fixture does not confirm a save");};
        endpoints.save_slot=[](auto){throw std::logic_error("factory return fixture does not confirm a save");};
        return std::make_unique<NativeSaveWindow>(ui,root,root,database,save_globals,activity,confirmation,address(manager.data()),std::move(endpoints));};
    NativeSystemMenuPages owner(ui,root,transitions,game,states,players,database,activity,keys,inventory,address(manager.data()),std::move(services));pages=&owner;
    owner.Event(0x8a80bcu,0u,0u);check(!owner.Property() && !created,"absent hidden Property remains unconstructed");
    owner.Event(0x8b0f1cu,0u,0u);check(!owner.Smith() && !smith_created,"absent hidden Smith remains unconstructed");
    owner.Event(0x8a80acu,1u,0u);
    const auto render=[&]{RwRGBA black{0,0,0,255};check(RwCameraClear(camera,&black,rwCAMERACLEARIMAGE|rwCAMERACLEARZ) && RwCameraBeginUpdate(camera),"System Property render start");
        check(ui.Render(),"System Property real DLL render");RwCameraEndUpdate(camera);};
    const auto key=[&](unsigned code){std::array<std::uint32_t,5> event{};event[4]=code;const auto e=address(event.data());
        const auto result=activity.Contains(e)?1u:activity.DispatchKeyDown(activity.GetLast(),e);owner.RethrowError();return result;};
    std::uint32_t retained{};
    for(unsigned cycle=0;cycle<2u;++cycle){
        (void)NativeCeguiWindowPointerEvents(module).HandleMouseAtCenter(ui.SystemToolbarButtons()[0]);owner.RethrowError();
        check(owner.Property() && created==1u,"DLL toolbar constructs/reuses production Property factory");auto& group=*owner.Property();auto& page=group.Page();
        check(!retained || retained==page.FrameAddress(),"Property factory keeps page identity while world remains live");retained=page.FrameAddress();
        check(key(3u) && key(28u),"System global router selects cure category and opens Property menu");
        check(group.Menu().Visible(),"factory composition opens actual choices");render();
        (void)NativeCeguiWindowPointerEvents(module).HandleMouseAtCenter(group.Menu().Button(0u));owner.RethrowError();
        check(group.Selector().Visible(),"factory use command opens owned actual role selector");render();
        auto* dll_system=reinterpret_cast<void*>(ui.SystemAddress());
        (void)detail::Export<bool (__thiscall*)(void*,unsigned)>(module,"?injectKeyDown@System@CEGUI@@QAE_NI@Z")(dll_system,28u);
        (void)detail::Export<bool (__thiscall*)(void*,unsigned)>(module,"?injectKeyUp@System@CEGUI@@QAE_NI@Z")(dll_system,28u);owner.RethrowError();
        check(inventory.ItemCount(3000u)==1u-cycle && sounds==cycle+1u,"factory real use consumes exactly one item");
        check(NativePlayerHudStatsData(players).PlayerStat(script,7u)>1,"factory actual recovery changes Player HP");
        if(cycle==0u){check(group.Selector().Visible(),"remaining cure keeps pending selector before page hide");
            owner.Event(0x8a80bcu,0u,0u);check(!group.Selector().Visible(),"page hide retires its active use request");owner.Event(0x8a80acu,1u,0u);}
    }
    (void)inventory.AddItem(3000u,3);owner.Property()->Page().RefreshItems(0u);
    detail::Export<void (__thiscall*)(void*,std::int32_t)>(module,"?setHoverRowIndex@MultiColumnList@CEGUI@@QAEXH@Z")(
        reinterpret_cast<void*>(owner.Property()->Page().State().Read(88u)),0);owner.RethrowError();
    owner.Property()->DispatchCommand("discard_property",reader.FindId(3000u)->address);render();
    ClickNativeCeguiWindow(ui,confirmation.Window("BtYes"));owner.RethrowError();
    std::cout<<"System Property discard remaining="<<inventory.ItemCount(3000u)<<" sound_requests="<<sounds<<" wa07="<<discard_sounds<<std::endl;
    check(!inventory.ItemCount(3000u) && discard_sounds==1u,"factory discard command confirms real full-stack removal and original discard sound");
    const auto save_item=inventory.AddItem(3182u,2);const auto save_definition=inventory.PropertyAddress(3182u);
    owner.Property()->Page().RefreshItems(*reinterpret_cast<const std::uint32_t*>(save_definition+4u));
    owner.Property()->DispatchCommand("use_property",save_item);
    check(owner.Save() && owner.Save()->Visible() && inventory.ItemCount(3182u)==1u && !objects.Size() && !effects.UiActiveCount() &&
        activity.GetLast()==owner.Save()->InputAddress(),"3182 consumes one and opens actual SaveWindow through production event factory");
    check(key(1u) && !owner.Save()->Visible() && objects.Size()>=1u && effects.UiActiveCount()==1u,
        "SaveWindow Escape routes after_use_property and restores original Property preview");
    owner.Property()->DispatchCommand("use_property",reader.FindId(3182u)->address);
    check(owner.Save()->Visible() && !inventory.ItemCount(3182u) && key(1u) && !owner.Save()->Visible() &&
        !inventory.ReadDword(inventory.Address()+88u+owner.Property()->Page().State().Read(108u)*4u),
        "3182 last item returns through same persistent SaveWindow and clears recent selection");
    const auto earth_item=inventory.AddItem(3067u,1);
    owner.Property()->DispatchCommand("use_property",earth_item);
    check(earth_closed==1u && earth_scripts.Depth()==1u && !objects.Size() && !effects.UiActiveCount() && inventory.ItemCount(3067u)==1u,
        "EarthBall closes its page and retires the use controller safely while queuing the actual SDK stack");
    check(earth_scripts.ProcessSelected(0.0f) && !earth_scripts.Active() && earth_engine.GlobalStoryVariable()==7,
        "factory EarthBall executes only after the UI callback returns");
    owner.Event(0x8a80bcu,0u,0u);owner.Event(0x8a80acu,1u,0u);
    owner.ReleaseProperty();check(!owner.Property() && !objects.Size() && !effects.UiActiveCount(),"factory retirement drops all world borrows");
    owner.Event(0x8a80bcu,1u,0u);check(owner.Property() && created==2u,"retired factory can construct a fresh page");
    owner.Event(0x8a80bcu,0u,0u);
    const auto retained_property=owner.Property()->Page().FrameAddress();
    check(ui.LoadOriginalMinimapLayout(root) && ui.SetMinimapLayoutVisible(false),"System Equipment original systemBold startup font");
    owner.Event(0x8a8098u,0u,0u);check(!owner.Equipment() && !equipment_created,"absent hidden Equipment remains unconstructed during toolbar clear");
    for(const auto& row:database.Tables().at("Equip").rows)(void)inventory.ProcessEquipment(inventory.Address()+0x7cu,std::get<unsigned>(row[0]));
    (void)NativePlayerScriptPropertyRuntime::Set(script,0x44u,4u,40u);(void)refresh.Refresh(script);
    const NativeSkillRecordCatalog::Record* passive{};
    for(std::size_t i=0;i<records.Size();++i){const auto& row=records.At(i);if(row[2]==1u && row[3]==0xffffffffu && row[4]==9114u && !row[14]){passive=&row;break;}}
    check(passive,"System Equipment original indexed passive exists");
    const auto passive_index=static_cast<unsigned>(std::bit_cast<float>((*passive)[5]));
    const std::array<const char*,7u> race_names{"ERR_HUMAN_RACE_ATK","ERR_DEMON_RACE_ATK","ERR_FAIRY_RACE_ATK","ERR_GHOST_RACE_ATK","ERR_IMMORTAL_RACE_ATK","ERR_SPIRIT_RACE_ATK","ERR_BEAST_RACE_ATK"};
    check(passive_index>=50u && passive_index<=56u,"original passive race selector");unsigned passive_field{};
    for(const auto& field:NativePlayerFieldSchema::Build())if(field.name==race_names[passive_index-50u])passive_field=script+field.slot_offset-0x654u;
    check(passive_field,"original passive derived field resolves independently from BASIC event getter");
    const auto passive_value=[&]{return std::bit_cast<float>(memory.ReadDword(passive_field));};
    const auto passive_expected=passive_value()+std::bit_cast<float>((*passive)[6]);
    check(skills.Apply(script,(*passive)[0],script),"System Equipment applies actual original permanent Skill through bound SkillSystem");
    const auto passive_applied=passive_value();(void)refresh.Refresh(script);
    std::cout<<"System Equipment passive="<<(*passive)[0]<<" event="<<std::bit_cast<float>((*passive)[5])<<" delta="<<std::bit_cast<float>((*passive)[6])<<" expected="<<passive_expected
        <<" applied="<<passive_applied<<" refreshed="<<passive_value()<<" type="<<memory.ReadDword(script+8u)
        <<" permanent_begin="<<memory.ReadDword(script+696u)<<" end="<<memory.ReadDword(script+700u)<<std::endl;
    check(passive_value()==passive_expected && memory.ReadDword(script+700u)>memory.ReadDword(script+696u),"original passive remains nonempty and refresh reapplies exactly once");
    EquipmentStatSnapshot passive_baseline{};std::memcpy(passive_baseline.data(),reinterpret_cast<const void*>(player+0x7a8u),sizeof(passive_baseline));
    const auto passive_allocations=memory.OutstandingAllocations();
    const auto inventory_sequences=[&]{std::set<unsigned> result;
        for(unsigned type=0u;type<7u;++type)
            for(unsigned row=0u;row<inventory.EquipmentCount(type);++row)
                result.insert(inventory.ReadDword(inventory.EquipmentAt(type,row)+400u));
        return result;};
    std::uint32_t retained_equipment{};
    for(unsigned cycle=0;cycle<2u;++cycle){
        owner.Event(0x8a80acu,1u,0u);render();ClickNativeCeguiWindow(ui,ui.SystemToolbarButtons()[3]);owner.RethrowError();
        check(owner.Equipment() && equipment_created==1u,"actual toolbar constructs/reuses production Equipment factory");auto& page=*owner.Equipment();
        check(owner.Property() && owner.Property()->Page().FrameAddress()==retained_property && owner.Property()->Menu().FrameAddress()==page.Menu().FrameAddress(),"persistent Property and Equipment retain identity while sharing original popup");
        check(!retained_equipment || retained_equipment==page.FrameAddress(),"Equipment factory preserves identity in live System world");retained_equipment=page.FrameAddress();
        check(owner.OwnsActivity(activity.GetLast()) && key(3u) && objects.Size()==2u && effects.UiActiveCount()==1u,"System-owned global activity routes Equipment category and actual item/pedestal/LH");
        EquipmentStatSnapshot restored{};std::memcpy(restored.data(),reinterpret_cast<const void*>(player+0x7a8u),sizeof(restored));
        check(restored==passive_baseline && passive_value()==passive_expected && memory.OutstandingAllocations()==passive_allocations,
            "System Equipment temporary projection restores all84 words with real nonempty passive list and no compounding/allocation leak");
        check(key(28u) && page.Menu().Visible(),"System Equipment list Enter opens owned native popup");
        page.Menu().DispatchCommand("close_popup_menu");owner.RethrowError();
        {
            // Exercise persistent mutations through DLL controls while the real
            // permanent Skill remains bound, rather than only projecting details.
            InventoryItemSlotsState empty_slots;unsigned wearable{},wearable_slot{},category{};
            for(unsigned type=3u;type<7u && !wearable;++type)
                for(unsigned index=0;index<inventory.EquipmentCount(type);++index){
                    const auto record=inventory.EquipmentAt(type,index);
                    const auto required=inventory.ReadDword(record+0x168u);
                    const auto slot=InventoryItemSlots::FindSlotOrClassify(empty_slots,record,inventory);
                    if(required && required<=40u && slot>0u && slot<6u && !players.Word((0x920u+slot*4u)/4u)){
                        wearable=record;wearable_slot=slot;
                        for(const auto& relation:database.Tables().at("EquipRelation").rows)
                            if(std::get<unsigned>(relation[2])==type)category=std::get<unsigned>(relation[1]);
                        break;
                    }
                }
            check(wearable,"System Equipment has original wearable in an initially empty slot");
            render();
            const auto ui_system=reinterpret_cast<void*>(ui.SystemAddress());
            const auto modal=detail::Export<void* (__thiscall*)(const void*)>(module,"?getModalTarget@System@CEGUI@@QBEPAVWindow@2@XZ")(ui_system);
            const auto capture=detail::Export<void* (__cdecl*)()>(module,"?getCaptureWindow@Window@CEGUI@@SAPAV12@XZ")();
            std::cout<<"System Equipment before category cycle="<<cycle<<" modal="<<address(modal)<<" capture="<<address(capture)
                <<" popup_root="<<*reinterpret_cast<const unsigned*>(page.Menu().FrameAddress()+36u)<<std::endl;
            const auto sequence=inventory.ReadDword(wearable+400u);
            const auto select_sequence=[&]{
                ClickNativeCeguiWindow(ui,page.State().Read(0x50u+category*4u));owner.RethrowError();
                check(page.State().Read(0xf0u)==category,"System Equipment category click after actual frame selects requested rows");
                auto* list=reinterpret_cast<void*>(page.State().Read(0x40u));
                const auto rows=detail::Export<unsigned (__thiscall*)(const void*)>(module,"?getRowCount@MultiColumnList@CEGUI@@QBEIXZ")(list);
                for(unsigned row=0;row<rows;++row){const std::array<unsigned,2> grid{row,0u};
                    auto* item=detail::Export<void* (__thiscall*)(const void*,const void*)>(module,"?getItemAtGridReference@MultiColumnList@CEGUI@@QBEPAVListboxItem@2@ABUMCLGridRef@2@@Z")(list,grid.data());
                    if(detail::Export<unsigned (__thiscall*)(const void*)>(module,"?getID@ListboxItem@CEGUI@@QBEIXZ")(item)==sequence){
                        detail::Export<void (__thiscall*)(void*,int)>(module,"?setHoverRowIndex@MultiColumnList@CEGUI@@QAEXH@Z")(list,row);
                        return;
                    }
                }
                throw std::runtime_error("System Equipment original eligible row missing from selected category");
            };
            select_sequence();
            const auto menu_key=key(28u);
            std::cout<<"System Equipment mutation enter="<<menu_key<<" popup="<<page.Menu().Visible()<<std::endl;
            check(menu_key && page.Menu().Visible(),"System Equipment original eligible row opens popup");
            ClickNativeCeguiWindow(ui,page.Menu().Button(0u));owner.RethrowError();
            check(players.Word((0x920u+wearable_slot*4u)/4u)==sequence && passive_value()==passive_expected,
                "System Equipment real DLL equip persists instance and retains permanent passive exactly once");
            (void)refresh.Refresh(script);
            check(passive_value()==passive_expected && memory.OutstandingAllocations()==passive_allocations,
                "System Equipment subsequent real refresh does not compound or allocate passive state");
            unsigned control{};
            for(unsigned slot=0;slot<6u;++slot){const auto window=page.State().Read(0x98u+slot*4u);
                if(address(detail::Export<void* (__thiscall*)(const void*)>(module,"?getUserData@Window@CEGUI@@QBEPAXXZ")(reinterpret_cast<void*>(window)))==wearable_slot)control=window;
            }
            check(control,"System Equipment equipped slot retains original userData routing");
            ClickNativeCeguiWindow(ui,control);owner.RethrowError();
            std::memcpy(restored.data(),reinterpret_cast<const void*>(player+0x7a8u),sizeof(restored));
            check(!players.Word((0x920u+wearable_slot*4u)/4u) && restored==passive_baseline && passive_value()==passive_expected &&
                memory.OutstandingAllocations()==passive_allocations && inventory.FindInventoryEntry(inventory.Address()+0x7cu,sequence,7u)==wearable,
                "System Equipment real DLL unequip restores84 words, inventory identity and nonempty passive without leaks");
            const auto before_discard=inventory_sequences();
            for(const bool confirm:{false,true}){
                render();select_sequence();
                check(key(28u) && page.Menu().Visible(),"System Equipment discard uses original row popup after unequip");
                ClickNativeCeguiWindow(ui,page.Menu().Button(1u));owner.RethrowError();
                check(detail::Export<bool (__thiscall*)(const void*)>(module,"?getModalState@Window@CEGUI@@QBE_NXZ")(
                          reinterpret_cast<void*>(confirmation.RootAddress())),"System Equipment menu opens actual modal confirmation");
                ClickNativeCeguiWindow(ui,confirmation.Window(confirm?"BtYes":"BtNo"));owner.RethrowError();
                std::memcpy(restored.data(),reinterpret_cast<const void*>(player+0x7a8u),sizeof(restored));
                auto expected=before_discard;if(confirm)expected.erase(sequence);
                check(inventory_sequences()==expected && equipment_discard_sounds==cycle+(confirm?1u:0u),
                    "System Equipment real No preserves all instances; Yes removes exactly selected instance with one discard sound");
                check(restored==passive_baseline && passive_value()==passive_expected && memory.OutstandingAllocations()==passive_allocations,
                    "System Equipment cancel/confirm list previews restore84 words and nonempty permanent passive without compounding or leaks");
            }
            check(!inventory.FindInventoryEntry(inventory.Address()+0x7cu,sequence,7u) && !inventory.RemoveEquipment(sequence),
                "System Equipment confirmed discard cannot resolve or remove same instance twice");
            (void)refresh.Refresh(script);
            check(passive_value()==passive_expected && memory.OutstandingAllocations()==passive_allocations,
                "System Equipment later real refresh preserves permanent Skill after inventory removal");
        }
        check(key(1u) && !objects.Size() && !effects.UiActiveCount() && key(1u) && activity.GetLast()==owner.Toolbar().ActiveComponentAddress(),"Equipment two Esc levels return actual System toolbar and retire previews");
    }
    // A party mutation in the same System session must refresh character routing
    // without leaking another role's temporary equipment projection or passive.
    const auto other_player=address(&players.slots[3]),other_script=other_player+0x654u;
    (void)PlayerManagerSlotDataCopier::Copy(other_player,roles.Address(3u),copier);
    memory.WriteDword(other_script,PlayerManagerSlot::kScriptVtable);
    (void)NativePlayerScriptPropertyRuntime::Set(other_script,0x44u,4u,40u);
    players.Word((3u*0xb14u+0xb08u)/4u)=1u;(void)refresh.Refresh(other_script);
    EquipmentStatSnapshot other_baseline{};std::memcpy(other_baseline.data(),reinterpret_cast<const void*>(other_player+0x7a8u),sizeof(other_baseline));
    const auto party_inventory=inventory_sequences();
    owner.Event(0x8a8098u,1u,0u);auto& party_page=*owner.Equipment();render();
    const auto switch_character=[&](unsigned control,unsigned expected){
        ClickNativeCeguiWindow(ui,party_page.State().Read(control));owner.RethrowError();
        check(party_page.State().Read(0xf4u)==expected,"System Equipment DLL arrow follows current party and skips inactive roles");
        EquipmentStatSnapshot first{},other{};
        std::memcpy(first.data(),reinterpret_cast<const void*>(player+0x7a8u),sizeof(first));
        std::memcpy(other.data(),reinterpret_cast<const void*>(other_player+0x7a8u),sizeof(other));
        check(first==passive_baseline && other==other_baseline && passive_value()==passive_expected &&
                  memory.OutstandingAllocations()==passive_allocations && inventory_sequences()==party_inventory,
            "System Equipment party changes isolate both84-word stat blocks, permanent Skill and inventory without leaks");
    };
    switch_character(0x4cu,3u);switch_character(0x48u,0u);
    players.Word((3u*0xb14u+0xb08u)/4u)=0u;switch_character(0x4cu,0u);
    players.Word((3u*0xb14u+0xb08u)/4u)=1u;switch_character(0x48u,3u);switch_character(0x4cu,0u);
    owner.Event(0x8a8098u,0u,0u);players.Word((3u*0xb14u+0xb08u)/4u)=0u;
    owner.Event(0x8a80bcu,1u,0u);check(owner.Property()->Page().FrameAddress()==retained_property && created==2u,"return to existing Property after Equipment preserves factory identity");
    owner.Event(0x8a80bcu,0u,0u);
    owner.ReleaseWorldPages();check(!owner.Equipment() && !owner.Property() && !objects.Size() && !effects.UiActiveCount(),"System world retirement releases Equipment and Property borrows");
    owner.Event(0x8a8098u,1u,0u);check(owner.Equipment() && equipment_created==2u,"retired Equipment factory constructs a fresh world-bound owner");
    owner.Event(0x8a8098u,0u,0u);owner.ReleaseWorldPages();
    VerifyNativeSystemMagicFactory(owner,ui,game,players,memory,inventory,database,confirmation,activity,magic_created,magic_learning_sounds,render,key);
    {
        const auto smith_party_index=(3u*0xb14u+0xb08u)/4u;const auto prior_party_flag=players.Word(smith_party_index);
        struct RestoreSmithParty{PlayerManagerObjectState& players;unsigned index,value;~RestoreSmithParty(){players.Word(index)=value;}} restore_smith_party{players,smith_party_index,prior_party_flag};
        players.Word(smith_party_index)=1u; // original forging gate requires the fourth party member
        {
            std::array<unsigned,6> values{};for(unsigned i=0;i<6u;++i)values[i]=game.GameDataEntry(14+static_cast<int>(i));
            struct RestoreGrantFlags{NativePalGameIVTextureStorage& game;std::array<unsigned,6> values;~RestoreGrantFlags(){for(unsigned i=0;i<6u;++i)game.SetGameDataEntry(14+static_cast<int>(i),values[i]);}} restore_grant{game,values};
            NativeAngelScriptEngine engine;std::string source;
            for(unsigned mask=0;mask<16u;++mask){source+="void Grant"+std::to_string(mask)+"(){giGrantSmithSystem(";for(unsigned i=0;i<4u;++i){if(i)source+=",";source+=(mask&(1u<<i))?"true":"false";}source+=");}";}
            for(unsigned i=0;i<7u;++i)source+="void Open"+std::to_string(i)+"(){giOpenSystemUi("+std::to_string(i)+");}";
            source+="void OpenInvalid(){giOpenSystemUi(-1);giOpenSystemUi(7);giOpenSystemUi(-2147483647-1);giOpenSystemUi(2147483647);}";
            check(engine.Raw()->AddScriptSection("smith_grant","grant.cs",source.c_str(),static_cast<int>(source.size()))>=0 && engine.Raw()->Build("smith_grant")>=0,"real SDK compiles all four-boolean Smith permission combinations");
            NativeScriptSystemUiRuntime runtime(game,address(manager.data()));NativeGameScriptCallbacks callbacks({});engine.SetCallbacks(&callbacks);NativeScriptFrameRunner runner(engine);
            runner.Queue("smith_grant","Grant0");check(runner.Tick(0.f)==NativeScriptFrameStatus::failed && runtime.Writes().empty(),"missing Smith script UI service fails without permission writes");
            NativeGameScriptServices endpoints;endpoints.system_ui=&runtime;callbacks.SetServices(endpoints);
            for(unsigned mask=0;mask<16u;++mask){const auto name="Grant"+std::to_string(mask);runner.Queue("smith_grant",name.c_str());check(runner.Tick(0.f)==NativeScriptFrameStatus::finished,"real SDK executes registered giGrantSmithSystem callback");
                for(unsigned i=0;i<4u;++i)check(game.GameDataEntry(15+static_cast<int>(i))==static_cast<unsigned>((mask&(1u<<i))!=0u),"actual GameData15-18 preserve four independent bool arguments");
                check(game.GameDataEntry(14)==values[0] && game.GameDataEntry(19)==values[5],"Smith grant leaves adjacent permissions unchanged");}
            check(runtime.Writes().size()==64u,"all SDK Smith grants reach existing native GameData writer");
            runner.Queue("smith_grant","Open4");check(runner.Tick(0.f)==NativeScriptFrameStatus::failed,
                "registered Open callback rejects an unbound state endpoint");
            std::vector<unsigned> script_calls;
            NativeScriptSystemUiServices open_services;
            open_services.enter_system=[&]{script_calls.push_back(0u);};
            open_services.event=[&](auto descriptor,auto choice,auto reserved){
                check(descriptor==0x8a80b8u && !reserved && script_calls.back()==0u,"SDK menu event follows state entry with original descriptor");
                script_calls.push_back(choice);
            };
            NativeScriptSystemUiRuntime open_runtime(game,address(manager.data()),nullptr,std::move(open_services));
            endpoints.system_ui=&open_runtime;callbacks.SetServices(endpoints);
            for(unsigned i=0;i<7u;++i){const auto name="Open"+std::to_string(i);runner.Queue("smith_grant",name.c_str());
                check(runner.Tick(0.f)==NativeScriptFrameStatus::finished && script_calls.size()==2u*(i+1u) && script_calls.back()==i+1u,
                    "real SDK maps every signed script menu index0-6 to original selector1-7");}
            runner.Queue("smith_grant","OpenInvalid");check(runner.Tick(0.f)==NativeScriptFrameStatus::finished && script_calls.size()==14u,
                "real SDK negative and upper-bound menu indices do not dispatch");
            engine.SetCallbacks(nullptr);
        }
        NativeCraftingRequirements requirements(smith_recipes,inventory);NativeInventoryPrescriptions learned(inventory,smith_recipes);
        unsigned recipe{};for(const auto& r:smith_recipes.Records())if(!inventory.ReadDword(smith_recipes.Address(r[0])+100u) && !requirements.HasEquipmentMaterial(r[0]) && !requirements.ProductAtLimit(requirements.Product(r[0]))){recipe=r[0];break;}
        check(recipe!=0u,"System Smith foundry fixture");(void)learned.Add(recipe);
        const auto record=smith_recipes.Address(recipe),product=requirements.Product(recipe),type=inventory.ReadDword(inventory.EquipmentDefinitionAddress(product)+0x94u);
        for(unsigned i=0;i<4u;++i){const auto id=inventory.ReadDword(record+0x98u+i*4u),amount=inventory.ReadDword(record+0xa8u+i*4u);if(static_cast<int>(amount)>0)(void)inventory.AddItem(id,static_cast<int>(amount));}
        const auto count=[&](){unsigned n{};for(unsigned i=0;i<inventory.EquipmentCount(type);++i){const auto entry=inventory.EquipmentAt(type,i);if(entry && inventory.ReadDword(entry+0x90u)==product)++n;}return n;};
        const auto parent=detail::Export<void* (__thiscall*)(const void*)>(module,"?getParent@Window@CEGUI@@QBEPAV12@XZ");
        const auto hover=detail::Export<void (__thiscall*)(void*,int)>(module,"?setHoverRowIndex@MultiColumnList@CEGUI@@QAEXH@Z");
        owner.Event(0x8a80acu,1u,0u);
        {
            NativeAngelScriptEngine engine;NativeGameScriptCallbacks script_callbacks({});engine.SetCallbacks(&script_callbacks);
            const char* source="void OpenSmith(){giOpenSystemUi(4);}";
            check(engine.Raw()->AddScriptSection("open_smith","open.cs",source,static_cast<int>(std::strlen(source)))>=0 && engine.Raw()->Build("open_smith")>=0,
                "real SDK compiles scripted Smith page selection");
            unsigned entered{};NativeScriptSystemUiServices endpoints;
            endpoints.enter_system=[&]{++entered;}; // State lifecycle is covered by the native arena fixture.
            endpoints.event=[&](auto descriptor,auto choice,auto reserved){check(entered==1u,"script enters before actual DLL menu selection");owner.Event(descriptor,choice,reserved);owner.RethrowError();};
            NativeScriptSystemUiRuntime runtime(game,address(manager.data()),nullptr,std::move(endpoints));
            NativeGameScriptServices script_services;script_services.system_ui=&runtime;script_callbacks.SetServices(script_services);
            NativeScriptFrameRunner runner(engine);runner.Queue("open_smith","OpenSmith");
            check(runner.Tick(0.f)==NativeScriptFrameStatus::finished && owner.Smith() && owner.SmithActive() && smith_created==1u,
                "real script giOpenSystemUi4 routes selector5 to actual DLL Smith factory");
            engine.SetCallbacks(nullptr);
        }
        (void)NativeCeguiWindowPointerEvents(module).HandleMouseAtCenter(ui.SystemToolbarButtons()[6]);owner.RethrowError();
        check(owner.Smith() && smith_created==1u && owner.SmithActive(),"actual toolbar constructs production Smith factory and sets shared original flag");auto* page=owner.Smith();const auto frame=page->FrameAddress();
        check(activity.GetLast()==frame+0xe8u && objects.Size()==2u && effects.UiActiveCount()==1u,"System global router owns Smith preview and input members");
        check(parent(reinterpret_cast<void*>(owner.FindFrameRoot("frameToolbar")))==reinterpret_cast<void*>(page->RootAddress()) &&
            parent(reinterpret_cast<void*>(owner.FindFrameRoot("gameInfo")))==reinterpret_cast<void*>(page->RootAddress()),"real registered toolbar/gameInfo attach to Smith root");
        const auto old_money=std::bit_cast<int>(inventory.ReadDword(inventory.Address()+0x134u));(void)inventory.UpdateCounter(123);owner.Event(0x8b13a8u,1u,0u);
        const auto info=owner.Decorations().InfoFrame(),money_window=*reinterpret_cast<const unsigned*>(info+0x2cu);
        const auto text=detail::Export<const void* (__thiscall*)(const void*)>(module,"?getText@Window@CEGUI@@QBEABVString@2@XZ")(reinterpret_cast<void*>(money_window));
        const auto text_bytes=detail::Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ")(text);
        if(std::string_view(text_bytes)!=std::to_string(old_money+123))std::cerr<<"Smith money refresh old="<<old_money<<" model="<<inventory.ReadDword(inventory.Address()+0x134u)<<" text="<<text_bytes<<std::endl;
        check(std::string_view(text_bytes)==std::to_string(old_money+123),"original Smith frame update event refreshes real DLL money text");(void)inventory.UpdateCounter(-123);
        unsigned recipe_row{};while(recipe_row<static_cast<unsigned>(learned.Count(0)) && learned.At(0,recipe_row)!=recipe)++recipe_row;
        hover(reinterpret_cast<void*>(page->FrameWord(0x44u)),static_cast<int>(recipe_row));owner.RethrowError();render();
        const auto before=count(),money=inventory.ReadDword(inventory.Address()+0x134u);
        check(key(28u),"System global key router opens Smith confirmation");ClickNativeCeguiWindow(ui,confirmation.Window("BtYes"));owner.RethrowError();
        check(count()==before+1u && inventory.ReadDword(inventory.Address()+0x134u)==money && smith_sounds,"System factory real foundry succeeds without normal fee");
        ClickNativeCeguiWindow(ui,confirmation.Window("BtOk"));owner.RethrowError();
        check(key(1u) && activity.GetLast()==frame+0x28u && key(1u),"System global key router retires Smith recipe then page");
        check(!parent(reinterpret_cast<void*>(page->RootAddress())) && !objects.Size() && !effects.UiActiveCount() && activity.GetLast()==owner.Toolbar().ActiveComponentAddress(),"Smith ESC returns actual System toolbar and releases preview");
        (void)NativeCeguiWindowPointerEvents(module).HandleMouseAtCenter(ui.SystemToolbarButtons()[6]);owner.RethrowError();check(owner.Smith()==page && smith_created==1u,"Smith factory preserves page identity while world remains live");
        (void)NativeCeguiWindowPointerEvents(module).HandleMouseAtCenter(ui.SystemToolbarButtons()[0]);owner.RethrowError();
        check(owner.Property() && !parent(reinterpret_cast<void*>(page->RootAddress())) && !page->FrameWord(0x9cu) && !page->FrameWord(0xa8u),"toolbar page switch retires Smith preview before opening Property");
        owner.ReleaseWorldPages();check(!owner.Smith() && !owner.Property() && !objects.Size() && !effects.UiActiveCount(),"world retirement releases Smith before borrowed UI world");
        owner.Event(0x8a80acu,1u,0u);(void)NativeCeguiWindowPointerEvents(module).HandleMouseAtCenter(ui.SystemToolbarButtons()[6]);owner.RethrowError();
        check(owner.Smith() && smith_created==2u,"retired Smith factory creates a fresh world-bound owner");owner.ReleaseWorldPages();
        check(!owner.Smith() && !objects.Size() && !effects.UiActiveCount(),"visible Smith retires models/effects before world release");
    }
    owner.Close();check(activity.Size()==0u,"factory Close removes global activity");objects.Clear();
    struct SkillCleanup final : IScriptSkillListCleanupRuntime {
        NativeBaseObjectMemory& memory;NativePlayerSkillCallbacks& callbacks;
        SkillCleanup(NativeBaseObjectMemory& m,NativePlayerSkillCallbacks& c):memory(m),callbacks(c){}
        unsigned ReadWord(unsigned a)override{return memory.ReadDword(a);}
        void WriteWord(unsigned a,unsigned v)override{memory.WriteDword(a,v);}
        unsigned ResolveSkillRecord(unsigned p)override{return callbacks.ResolveSkillRecord(p);}
        unsigned InvokeVirtual(unsigned a,unsigned s,std::span<const unsigned> args)override{return callbacks.InvokeVirtual(a,s,args);}
    } cleanup(memory,callbacks);
    (void)ScriptSkillCollection::CleanupObjectLists(script,cleanup);
    for(const auto offset:{680u,696u}){const auto buffer=memory.ReadDword(script+offset);if(buffer)(void)memory.Free(buffer);
        for(unsigned n=0;n<3u;++n)memory.WriteDword(script+offset+n*4u,0u);}
    refresh.BindSkills(nullptr);system.Destroy();check(!memory.OutstandingAllocations(),"factory skills balanced");
    std::cout<<"PASS: production System Property + Equipment factories, actual DLL toolbar/menu/role/cure, shared popup, original passive Skill="<<(*passive)[0]<<", equip/refresh/unequip, No/Yes exact-instance discard, live party mutation, both84-word restoration, global activity, persistent reopen and world retirement"<<std::endl;
}
}
