#include "pal4/native_trade_window_input.h"
#include "pal4/native_trade_window_navigation.h"
#include "pal4/native_trade_window_property_pages.h"
#include "pal4/native_trade_window_record_pages.h"
#include "pal4/native_trade_window_repeat.h"
#include "pal4/native_listbox_spinner_item.h"
#include "pal4/native_game_timer_queue.h"
#include "pal4/native_player_inventory_state_runtime.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/player_manager_object.h"
#include "pal4/native_cegui_window_pointer_events.h"
#include "support/native_system_property_probe.h"
#include "pal4/native_trade_window_data.h"
#include "pal4/native_trade_window_layout.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_inventory_property_reader.h"
#include "pal4/native_inventory_prescriptions.h"
#include "pal4/native_shop_record_catalog.h"
#include "pal4/native_prescription_record_catalog.h"
#include "pal4/native_pal_game_iv_texture_storage.h"
#include "pal4/native_event_handler_array.h"
#include "../native/ui/cegui_exports_native.h"
#include <array>
#include <bit>
#include <iostream>
#include <memory>
#include <set>
#include <vector>
namespace pal4::test {
namespace {
void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
unsigned Address(const void* p){return reinterpret_cast<unsigned>(p);}
unsigned Word(unsigned p){return *reinterpret_cast<const unsigned*>(p);}
std::vector<unsigned> Ids(const NativeGameDatabase& db,const char* name){
    const auto& table=db.Tables().at(name);unsigned id{};while(table.fields.at(id).name!="ID")++id;
    std::vector<unsigned> values;for(const auto& row:table.rows)values.push_back(std::get<unsigned>(row.at(id)));return values;
}
}
void VerifyNativeTradeWindowData(const std::filesystem::path& root,NativeCeguiUiSession& ui,const NativeGameDatabase& database,NativePalGameIVTextureStorage& game){
    NativeGameDatabase shop_db;Check(shop_db.Load(root,"shopdb.db"),"Trade data original shop database");NativeShopRecordCatalog shops(shop_db);
    NativePrescriptionRecordCatalog recipes(database);NativeInventorySession inventory;inventory.BindDatabase(database);(void)inventory.Start();inventory.Reset();
    std::vector<unsigned> observed;inventory.BindItemObserver([&](auto id){observed.push_back(id);});
    std::array<unsigned,0x224u/4u> manager{};unsigned pages{};bool fail_page{};std::unique_ptr<NativeTradeWindowData> data;
    std::unique_ptr<NativeTradeWindowNavigation> navigation;std::unique_ptr<NativeTradeWindowPropertyPages> property_pages;std::unique_ptr<NativeTradeWindowRecordPages> record_pages;
    std::unique_ptr<NativeNumericSpinnerPool> pool;std::unique_ptr<NativeTradeWindowRepeat> repeat;
    PlayerManagerObjectState players{};NativeBaseObjectMemory player_memory;NativePlayerInventoryStateRuntime conditions(players,player_memory,inventory);
    unsigned resets{},details{};NativeGameTimerQueue timers([](auto,auto){throw std::logic_error("unexpected basic Trade callback");},[&](auto owner,auto method,auto row,auto flag){Check(bool(repeat),"repeat timer owner");repeat->Callback(owner,method,row,flag);});manager[0x1a0u/4u]=timers.Construct();
    NativeTradeWindowLayout* active_layout{};std::unique_ptr<NativeTradeWindowInput> input;std::vector<std::array<unsigned,2>> status_calls;
    // Actual input/status, data, pages, category and quantity/repeat owners are
    // composed. Confirmation and close actions reject when not bound here.
    NativeEventHandlerArray activity([&](unsigned component,unsigned slot){
        Check(active_layout && (component==active_layout->FrameAddress()+0x19cu || component==active_layout->FrameAddress()+0x1a8u) &&
            (slot==0xcu || slot==0x10u),"Trade activity invokes only original input activation/status endpoints");
        Check(input && input->OwnsActivity(component),"Trade data routes actual input owner");
        status_calls.push_back({component,slot});return input->Callback(component,slot);
    },[&](auto component,auto slot,auto event)->std::uint8_t{Check(input && input->OwnsActivity(component),"Trade data predicate input owner");return input->Predicate(component,slot,event);});
    NativeTradeWindowLayout layout(ui,root,Address(manager.data()),[&](auto method,auto event){
        Check(data!=nullptr,"Trade data callback after construction");
        const auto* payload=reinterpret_cast<const unsigned*>(event);
        if(method==0x48bea0u){data->Refresh(payload[0],payload[1]);return true;}
        if(method==0x48c5c0u){data->SetActive(payload[0]);return true;}
        if(method==0x48d290u || method==0x48d310u){Check(bool(navigation),"category navigation owner");return navigation->Event(method);}
        if(method==0x48cc20u || method==0x48cc80u || method==0x48cce0u){Check(bool(repeat),"quantity repeat owner");return repeat->Event(method,event);}
        if(method==0x48d1d0u || method==0x48d1a0u){++details;return true;} // Explicit detail/model boundary.
        throw std::logic_error("Trade confirmation/close action not bound in data fixture");
    },[&]{if(navigation)navigation->Close();if(data)data->Close();if(property_pages)property_pages->Close();if(record_pages)record_pages->Close();if(repeat)repeat->Close();if(pool)pool->Close();});
    const auto page=[&](auto group,auto selected,auto flag){
        Check(layout.Word(0xe8u)==group && !selected && flag==1u && !activity.Size(),"Trade page follows data/group and removal of both activity components");
        ++pages;if(fail_page)throw std::runtime_error("page failure");Check(bool(navigation),"actual category/page owner");navigation->Update(group,selected,flag!=0u);
    };
    active_layout=&layout;
    NativeTradeWindowInputServices input_services;
    input_services.close=[](){throw std::logic_error("data fixture close action not bound");};
    input_services.prompt=[](){throw std::logic_error("data fixture prompt action not bound");};
    input_services.spinner_value=[&](auto item){return pool->Value(item);};
    input_services.set_spinner_value=[&](auto item,auto value){pool->SetValue(item,value);};
    pool=std::make_unique<NativeNumericSpinnerPool>(ui,root,NativeSpinnerServices{[&](auto){if(property_pages)property_pages->QuantityChanged();},[&](auto key,auto row,auto col){Check(layout.Event(key,{row,col}),"quantity press/release routes to original frame");}});
    repeat=std::make_unique<NativeTradeWindowRepeat>(layout,*pool,timers,Address(manager.data()));
    const auto cancel=[&]{repeat->Cancel();++resets;};
    property_pages=std::make_unique<NativeTradeWindowPropertyPages>(layout,inventory,*pool,NativeTradePropertyPageServices{cancel,[&]{++details;}});
    record_pages=std::make_unique<NativeTradeWindowRecordPages>(layout,inventory,conditions,database,recipes,NativeTradeRecordPageServices{cancel,[&]{++details;}});
    input=std::make_unique<NativeTradeWindowInput>(layout,std::move(input_services));
    data=std::make_unique<NativeTradeWindowData>(layout,inventory,game,shops,recipes,activity,Address(manager.data()),page);
    navigation=std::make_unique<NativeTradeWindowNavigation>(layout,*data,*property_pages,*record_pages,cancel);
    const auto visible=[&](unsigned w){return detail::Export<bool (__thiscall*)(const void*,bool)>(GetModuleHandleA("CEGUIBase.dll"),"?isVisible@Window@CEGUI@@QBE_N_N@Z")(reinterpret_cast<void*>(w),false);};
    const auto row_count=[&]{return detail::Export<unsigned (__thiscall*)(const void*)>(GetModuleHandleA("CEGUIBase.dll"),"?getRowCount@MultiColumnList@CEGUI@@QBEIXZ")(reinterpret_cast<void*>(layout.Word(0x28u)));};
    const auto selected=[&](unsigned group){return layout.Word(group==0u?0xecu:group==1u?0xf0u:0xf4u);};
    const auto key_at=[&](unsigned row){const std::array grid{row,0u};const auto item=Address(detail::Export<void* (__thiscall*)(const void*,const void*)>(GetModuleHandleA("CEGUIBase.dll"),"?getItemAtGridReference@MultiColumnList@CEGUI@@QBEPAVListboxItem@2@ABUMCLGridRef@2@@Z")(reinterpret_cast<void*>(layout.Word(0x28u)),grid.data()));return Word(item+312u);};
    layout.BindParent(true);Check(layout.Event(0x8b0a4cu,{1u,0u}) && visible(layout.Word(0xdcu)) && !visible(layout.Word(0xe0u)),"actual selector event enables buy label in original DLL");
    unsigned cases{};
    for(const auto& shop:shops.Shops())for(unsigned group=0;group<3u;++group){
        const auto owner=Address(&shop);const auto name=shops.Text(owner+4u);std::vector<unsigned> expected;std::set<unsigned> categories;
        for(const auto& good:shops.Goods())if(shops.Text(Address(&good)+20u)==name && good[1]==group &&
            game.PointerEntryValue(std::bit_cast<int>(good[3]))>=std::bit_cast<int>(good[4])){
            const auto definition=group==0u?inventory.EquipmentDefinitionAddress(good[2]):group==1u?inventory.PropertyAddress(good[2]):recipes.Address(good[2]);
            if(definition){expected.push_back(definition);categories.insert(Word(definition+(group==0u?148u:group==1u?4u:100u)));}
        }
        Check(layout.Event(0x8b0a48u,{group,owner}),"original goods event dispatches native Trade data");
        const auto actual=data->Records(group);Check(std::vector<unsigned>(actual.begin(),actual.end())==expected && data->HasItems()==!expected.empty() && data->CanChangePage()==(categories.size()>1u),"all original shops resolve exact ordered definitions and category availability");
        const auto expected_keys=[&](unsigned category){std::vector<unsigned> ids;for(auto definition:expected)if(Word(definition+(group==0u?148u:group==1u?4u:100u))==category && (group!=2u || category || inventory.EquipmentDefinitionAddress(Word(definition+104u))))ids.push_back(Word(definition+(group==0u?144u:0u)));return ids;};
        const auto verify_page=[&]{const auto keys=expected_keys(selected(group));Check(row_count()==keys.size(),"actual native shop category has exact row count");for(unsigned row=0;row<keys.size();++row)Check(key_at(row)==keys[row],"actual native shop category preserves product keys/order");};verify_page();
        Check(activity.Size()==1u && activity.GetLast()==layout.FrameAddress()+(group==1u?0x1a8u:0x19cu) &&
            visible(layout.Word(0xd4u))==(group==0u) && visible(layout.Word(0xd8u))==(group!=0u) &&
            visible(layout.Word(0x44u))==(categories.size()>1u) && visible(layout.Word(0x48u))==(categories.size()>1u),"Trade original category panel, arrows and live activity identity");++cases;
        if(categories.size()>1u){const auto limit=group==0u?7u:group==1u?4u:3u;
            for(bool forward:{true,false}){unsigned candidate=(selected(group)+(forward?1u:limit-1u))%limit,wanted=candidate;
                for(unsigned step=0;step<limit;++step){wanted=candidate;if(!expected_keys(candidate).empty())break;candidate=(candidate+(forward?1u:limit-1u))%limit;}
                if(expected.empty())wanted=0u;
                const auto before=resets;(void)NativeCeguiWindowPointerEvents(GetModuleHandleA("CEGUIBase.dll")).HandleMouseAtCenter(layout.Word(forward?0x48u:0x44u));layout.RethrowEventError();
                Check(selected(group)==wanted && resets>before,"original DLL category click invokes real skip-empty navigation and cancellation");verify_page();}
        }
    }
    Check(status_calls.size()>=cases,"real activity storage exercises native input activation/status endpoints");
    const auto before_bad=layout.Word(0xe8u);bool foreign{};
    try{data->Refresh(0u,Address(&shops.Goods()[0]));}catch(const std::invalid_argument&){foreign=true;}
    Check(foreign && layout.Word(0xe8u)==before_bad,"foreign shop pointer rejects before original group mutation");
    fail_page=true;bool failed{};try{data->Refresh(0u,Address(&shops.Shops()[0]));}catch(const std::runtime_error& e){failed=std::string_view(e.what())=="page failure";}fail_page=false;
    Check(failed && !activity.Size(),"page failure retains original data/removed activity and does not leak temporary shop text");data->Refresh(0u,Address(&shops.Shops()[0]));
    std::array<unsigned,2> equipment{},properties{},recipe_ids{};unsigned equipment_type=7u,property_type=5u,recipe_type=3u;
    for(const auto id:Ids(database,"Equip")){const auto row=inventory.EquipmentDefinitionAddress(id);if(row && Word(row+148u)<7u){if(!equipment[0]){equipment[0]=id;equipment_type=Word(row+148u);}else if(Word(row+148u)!=equipment_type){equipment[1]=id;break;}}}
    for(const auto id:Ids(database,"Property")){const auto row=inventory.PropertyAddress(id);if(row && Word(row+4u)<5u){if(!properties[0]){properties[0]=id;property_type=Word(row+4u);}else if(Word(row+4u)!=property_type){properties[1]=id;break;}}}
    for(const auto& row:recipes.Records()){if(!recipe_ids[0]){recipe_ids[0]=row[0];recipe_type=row[25];}else if(row[25]!=recipe_type){recipe_ids[1]=row[0];break;}}
    Check(equipment[1] && properties[1] && recipe_ids[1],"original records cover distinct inventory categories");
    data->SetActive(0xffffffffu);Check(!visible(layout.Word(0xdcu)) && visible(layout.Word(0xe0u)),"negative selector uses original signed buy/sell comparison");
    for(unsigned group=0;group<3u;++group){data->Refresh(group,0u);Check(!data->HasItems() && !data->CanChangePage(),"empty real sell inventory has no items or category arrows");}
    for(const auto id:equipment)(void)inventory.ProcessEquipment(inventory.Address()+124u,id);
    for(const auto id:properties)(void)inventory.AddItem(id,1);
    NativeInventoryPrescriptions learned(inventory,recipes);for(const auto id:recipe_ids)(void)learned.Add(id);
    for(unsigned group=0;group<3u;++group){const auto retained=data->Records(group).size();data->Refresh(group,0u);Check(data->HasItems() && data->CanChangePage() && data->Records(group).size()==retained,"sell mode reads actual equipment/property/recipe categories and retains buy arrays");}
    inventory.Reset();(void)inventory.ProcessEquipment(inventory.Address()+124u,equipment[0]);(void)inventory.AddItem(properties[0],1);(void)learned.Add(recipe_ids[0]);
    for(unsigned group=0;group<3u;++group){data->Refresh(group,0u);Check(data->HasItems() && !data->CanChangePage(),"one real sell category hides arrows while items remain available");}
    const auto before_invalid=pages;data->Refresh(3u,0u);Check(!data->HasItems() && !data->CanChangePage() && !activity.Size() && pages==before_invalid,"invalid group removes old activity and hides arrows without page update");
    navigation->Close();navigation.reset();data->Close();data->Close();
    // Controlled original-schema records prove OPEN_* uses a signed game-data
    // threshold, independently of inventory money, with exact equality allowed.
    NativeGameDbTable shop_table,goods_table;shop_table.fields={{"ID",2u},{"NAME",2u},{"POSITION",2u},{"TYPE",1u}};
    shop_table.rows={{std::string("conditional"),std::string("shop"),std::string("Q01"),0u}};
    goods_table.fields={{"ID",1u},{"SHOP_ID",2u},{"GOODS_ID",1u},{"GOODS_TYPE",1u},{"OPEN_CONDITION",1u},{"OPEN_NUM",1u}};
    goods_table.rows={{1u,std::string("conditional"),equipment[0],0u,15u,2u},{2u,std::string("conditional"),equipment[1],0u,15u,1u},{3u,std::string("conditional"),properties[0],1u,15u,0xffffffffu}};
    NativeShopRecordCatalog conditional(shop_table,goods_table);
    const auto story=game.StoryObjects();Check(story.size()>15u && game.PointerEntryValue(-1)==-1 && game.PointerEntryValue(static_cast<int>(story.size()))==-1,"native Story value reuses original pointer-table bounds");
    auto& condition=*reinterpret_cast<unsigned*>(story[15]);
    struct Restore{unsigned& value;unsigned old;~Restore(){value=old;}} restore{condition,condition};
    data=std::make_unique<NativeTradeWindowData>(layout,inventory,game,conditional,recipes,activity,Address(manager.data()),page);
    navigation=std::make_unique<NativeTradeWindowNavigation>(layout,*data,*property_pages,*record_pages,cancel);data->SetActive(1u);
    Check(!inventory.ReadDword(inventory.Address()+308u),"condition fixture has zero money");condition=1u;data->Refresh(0u,Address(&conditional.Shops()[0]));
    Check(data->Records(0u).size()==1u && data->Records(0u)[0]==inventory.EquipmentDefinitionAddress(equipment[1]),"game variable below threshold excludes only unopened goods despite zero money");
    condition=2u;data->Refresh(0u,Address(&conditional.Shops()[0]));Check(data->Records(0u).size()==2u && data->CanChangePage(),"threshold equality opens second equipment category");
    const auto key=[&](unsigned code){std::array<unsigned,5> event{};event[4]=code;return activity.DispatchKeyDown(activity.GetLast(),Address(event.data()));};
    const auto first_equipment_category=selected(0u);
    Check(key(200u) && (layout.Word(0x1a4u)&0xffu),"actual Up input enters category focus from first real shop row");
    Check(key(205u) && selected(0u)!=first_equipment_category && row_count()>0u,"actual Right input clicks original arrow and skips to next populated shop category");
    Check(key(208u) && !(layout.Word(0x1a4u)&0xffu),"actual Down input returns from category focus to real rows");
    bool foreign_method{};try{(void)navigation->Event(0x1234u);}catch(const std::invalid_argument&){foreign_method=true;}Check(foreign_method,"native category adapter rejects foreign event methods");
    condition=0xffffffffu;data->Refresh(0u,Address(&conditional.Shops()[0]));Check(data->Records(0u).empty(),"signed negative game value remains below positive thresholds");
    data->Refresh(1u,Address(&conditional.Shops()[0]));Check(data->Records(1u).size()==1u,"signed negative threshold equality is accepted");
    Check(row_count()!=0u,"conditional native property page contains original quantity row");
    const std::array<unsigned,2> quantity_grid{0u,1u};
    Check(layout.Event(0x8b1860u,{0u,0u}),"actual quantity down schedules repeat through frame descriptor");
    const auto callbacks_before=timers.EventCount();Check(callbacks_before>0u,"actual repeat delay is queued");
    (void)navigation->Event(0x48d310u);timers.Update(1.f);
    // A row address can be recycled immediately; cancellation is proved by the
    // empty real queue and the replacement row's reset value rather than address.
    const auto replacement=Address(detail::Export<void* (__thiscall*)(const void*,const void*)>(GetModuleHandleA("CEGUIBase.dll"),"?getItemAtGridReference@MultiColumnList@CEGUI@@QBEPAVListboxItem@2@ABUMCLGridRef@2@@Z")(reinterpret_cast<void*>(layout.Word(0x28u)),quantity_grid.data()));
    Check(!timers.EventCount() && !timers.Count() && pool->Value(replacement)==0,"category rebuild cancels real repeat before retiring/recycling quantity rows");
    data->Close();layout.BindParent(false);layout.Close();Check(!activity.Size(),"Trade data retires active components and host vectors before layout");activity.ReleaseStorage();timers.Destroy();inventory.Close();Check(!timers.OutstandingAllocations() && !pool->OutstandingAllocations() && !player_memory.OutstandingAllocations() && !inventory.OutstandingAllocations(),"Trade data never owns or leaks a replacement inventory");
    std::cout<<"PASS: Trade native data "<<cases<<" original shop/group combinations, actual definition arrays/condition variable/category panels/activity, sell inventory categories, threshold/failure/retirement; actual page/category/quantity/repeat composition; detail/model/confirmation/close remain explicit\n";
}
}
