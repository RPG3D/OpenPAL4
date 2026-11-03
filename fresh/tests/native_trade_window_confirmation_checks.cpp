#include "support/native_system_property_probe.h"
#include "support/native_exception_trace.h"
#include "pal4/native_trade_window_confirmation.h"
#include "pal4/native_trade_window_layout.h"
#include "pal4/native_trade_window_input.h"
#include "pal4/native_trade_window_property_pages.h"
#include "pal4/native_trade_window_record_pages.h"
#include "pal4/native_player_inventory_state_runtime.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/player_manager_object.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_inventory_trade.h"
#include "pal4/native_inventory_property_reader.h"
#include "pal4/native_inventory_prescriptions.h"
#include "pal4/native_prescription_record_catalog.h"
#include "pal4/native_listbox_spinner_item.h"
#include "pal4/native_cegui_ui_session.h"
#include "../native/ui/cegui_exports_native.h"
#include <array>
#include <bit>
#include <iostream>
namespace pal4::test {
namespace {
void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
unsigned Address(const void* p){return reinterpret_cast<unsigned>(p);}
unsigned Word(unsigned p){return *reinterpret_cast<const unsigned*>(p);}
std::vector<unsigned> Ids(const NativeGameDatabase& db,std::string_view name){const auto& table=db.Tables().at(std::string(name));unsigned col{};while(table.fields.at(col).name!="ID")++col;std::vector<unsigned> ids;for(const auto& row:table.rows)ids.push_back(std::get<unsigned>(row[col]));return ids;}
}
void VerifyNativeTradeWindowConfirmation(const std::filesystem::path& root,NativeCeguiUiSession& ui,const NativeGameDatabase& db){
    NativeExceptionTrace trace;
    const auto module=GetModuleHandleA("CEGUIBase.dll");
    NativePrescriptionRecordCatalog recipes(db);NativeInventorySession inventory;inventory.BindDatabase(db);(void)inventory.Start();inventory.Reset();
    std::vector<unsigned> observed;inventory.BindItemObserver([&](auto id){observed.push_back(id);});NativeInventoryTrade trade(inventory,recipes);
    NativeCommonDialogLayout dialog(module,ui,root);std::array<unsigned,0x224u/4u> manager{};
    std::unique_ptr<NativeNumericSpinnerPool> pool;std::unique_ptr<NativeTradeWindowConfirmation> confirm;std::unique_ptr<NativeTradeWindowPropertyPages> property_pages;std::unique_ptr<NativeTradeWindowRecordPages> record_pages;
    PlayerManagerObjectState players{};NativeBaseObjectMemory player_memory;NativePlayerInventoryStateRuntime conditions(players,player_memory,inventory);
    std::vector<unsigned> equipment_definitions,recipe_definitions;
    std::vector<unsigned> property_definitions;unsigned resets{},clears{},quantity_changes{};
    const auto reset=detail::Export<void (__thiscall*)(void*)>(module,"?resetList@MultiColumnList@CEGUI@@QAEXXZ");
    NativeTradeWindowLayout layout(ui,root,Address(manager.data()),[](auto method,auto){Check(method==0x48d1d0u || method==0x48d1a0u,"only explicit hover/detail boundary in confirmation fixture");return true;},[&]{if(confirm)confirm->Close();if(property_pages)property_pages->Close();if(record_pages)record_pages->Close();for(auto field:{0x134u,0x138u,0x13cu,0x144u,0x148u,0x14cu,0x154u,0x158u,0x15cu})layout.Word(field)=0u;reset(reinterpret_cast<void*>(layout.Word(0x28u)));if(pool)pool->Close();});
    pool=std::make_unique<NativeNumericSpinnerPool>(ui,root,NativeSpinnerServices{[&](auto){++quantity_changes;if(property_pages)property_pages->QuantityChanged();},[](auto,auto,auto){throw std::logic_error("quantity button not routed in confirmation fixture");}});
    unsigned pages{},displays{};bool retire_during_refresh{},retirement_rejected{};std::vector<std::string> sounds;
    NativeTradeConfirmationServices services;
    services.sound=[&](std::string_view file,unsigned repeat){Check((file=="wa03.wav" || file=="wa04.wav") && repeat==1u,"Trade uses original dialog sound policy");sounds.emplace_back(file);};
    // All three page groups and player equipment checks use native owners.
    // Model refresh and muted sound remain explicit caller boundaries.
    services.refresh_page=[&](unsigned group,unsigned page,unsigned flag){Check(group<3u && page<7u && flag==1u,"Trade result refreshes current original page");++pages;if(group==1u){Check(bool(property_pages),"actual property page owner");property_pages->Update(page);}else if(group==0u)record_pages->Equipment(page);else record_pages->Recipes(page);if(retire_during_refresh){try{confirm->Close();}catch(const std::logic_error&){retirement_rejected=true;}}};
    services.refresh_equipment_display=[&](unsigned definition){Check(definition!=0u,"Trade equipment definition refresh");++displays;};
    services.equipped=[&](unsigned instance){return conditions.CheckPlayerCondition(instance)!=0u;};
    auto missing=services;missing.refresh_page={};bool missing_rejected{};
    try{NativeTradeWindowConfirmation bad(layout,inventory,trade,*pool,recipes,db,dialog,Address(manager.data()),std::move(missing));}catch(const std::invalid_argument&){missing_rejected=true;}
    Check(missing_rejected,"confirmation refuses missing page service");
    confirm=std::make_unique<NativeTradeWindowConfirmation>(layout,inventory,trade,*pool,recipes,db,dialog,Address(manager.data()),services);
    NativeTradeWindowInput input(layout,NativeTradeWindowInputServices{
        []{throw std::logic_error("Trade close state not composed in confirmation fixture");},
        [&]{Check(confirm->Prompt(),"Enter reaches actual Trade confirmation");},
        [&](auto cell){return pool->Value(cell);},[&](auto cell,auto value){pool->SetValue(cell,value);}});
    property_pages=std::make_unique<NativeTradeWindowPropertyPages>(layout,inventory,*pool,NativeTradePropertyPageServices{[&]{++resets;},[&]{++clears;}});
    record_pages=std::make_unique<NativeTradeWindowRecordPages>(layout,inventory,conditions,db,recipes,NativeTradeRecordPageServices{[&]{++resets;},[&]{++clears;}});
    unsigned equipment{},property{};
    for(auto id:Ids(db,"Equip")){const auto p=inventory.EquipmentDefinitionAddress(id);if(p && Word(p+376u)>0u && Word(p+376u)<100000u && Word(p+148u)<7u){equipment=id;break;}}
    for(auto id:Ids(db,"Property")){const auto p=inventory.PropertyAddress(id);if(p && Word(p+144u)>0u && Word(p+144u)<100000u){property=id;break;}}
    Check(equipment && property && !recipes.Records().empty(),"Trade confirmation original products");
    const auto balance=[&]{return std::bit_cast<int>(Word(inventory.Address()+308u));};
    const auto money=[&](int value){(void)inventory.UpdateCounter(value-balance());};
    const auto list=layout.Word(0x28u);layout.BindParent(true);
    const auto stage=[&](unsigned group,bool buy,unsigned id,int quantity){
        confirm->Cancel();property_pages->Clear();record_pages->Clear();reset(reinterpret_cast<void*>(list));layout.Word(0xe8u)=group;layout.Word(0xf8u)=buy;
        if(group==1u){
            unsigned definition{};if(buy)definition=inventory.PropertyAddress(id);else{const auto entry=NativeInventoryPropertyReader(inventory).FindSequence(id);Check(bool(entry),"sale page inventory sequence");definition=entry->definition;}
            property_definitions={definition};layout.Word(0x144u)=Address(property_definitions.data());layout.Word(0x148u)=Address(property_definitions.data()+property_definitions.size());layout.Word(0x14cu)=layout.Word(0x148u);
            property_pages->Update(Word(definition+4u));
            const std::array<unsigned,2> grid{0u,1u};const auto cell=Address(detail::Export<void* (__thiscall*)(const void*,const void*)>(module,"?getItemAtGridReference@MultiColumnList@CEGUI@@QBEPAVListboxItem@2@ABUMCLGridRef@2@@Z")(reinterpret_cast<void*>(list),grid.data()));
            pool->SetValue(cell,quantity);Check(Word(cell+0x138u)==id && Word(cell+0x134u)==0u,"real page writes product/sequence separately from row ID");return;
        }
        if(group==0u){const auto definition=buy?inventory.EquipmentDefinitionAddress(id):inventory.FindInventoryEntry(inventory.Address()+124u,id,7u);Check(definition!=0u,"equipment page original definition/instance");
            equipment_definitions={buy?definition:inventory.EquipmentDefinitionAddress(Word(definition+144u))};layout.Word(0x134u)=Address(equipment_definitions.data());layout.Word(0x138u)=Address(equipment_definitions.data()+1u);layout.Word(0x13cu)=layout.Word(0x138u);
            record_pages->Equipment(Word(definition+148u));return;
        }
        Check(group==2u,"recipe page group");recipe_definitions={recipes.Address(id)};layout.Word(0x154u)=Address(recipe_definitions.data());layout.Word(0x158u)=Address(recipe_definitions.data()+1u);layout.Word(0x15cu)=layout.Word(0x158u);record_pages->Recipes(Word(recipe_definitions[0]+100u));
    };
    const auto visible=[&](const char* suffix){return detail::Export<bool (__thiscall*)(const void*,bool)>(module,"?isVisible@Window@CEGUI@@QBE_N_N@Z")(reinterpret_cast<void*>(dialog.Window(suffix)),true);};
    const auto choose=[&](const char* button){std::array<unsigned,5> event{};const auto target=dialog.Window(button);
        detail::Export<void (__thiscall*)(void*,void*)>(module,"??0WindowEventArgs@CEGUI@@QAE@PAVWindow@1@@Z")(event.data(),reinterpret_cast<void*>(target));detail::NativeString empty(module,"");
        detail::Export<void (__thiscall*)(void*,const void*,void*,const void*)>(module,"?fireEvent@EventSet@CEGUI@@UAEXABVString@2@AAVEventArgs@2@0@Z")(reinterpret_cast<void*>(target),detail::Export<const void*>(module,"?EventClicked@PushButton@CEGUI@@2VString@2@B"),event.data(),empty.Data());
        detail::Export<void (__thiscall*)(void*)>(module,"??1WindowEventArgs@CEGUI@@UAE@XZ")(event.data());dialog.RethrowEventError();};
    stage(1u,true,property,0);Check(confirm->Prompt() && visible("BtOk") && !visible("BtYes"),"zero property quantity uses original informational prompt");choose("BtOk");
    const auto property_price=Word(inventory.PropertyAddress(property)+144u);money(1000000);stage(1u,true,property,3);
    const auto total_text=[&]{const auto text=detail::Export<const void* (__thiscall*)(const void*)>(module,"?getText@Window@CEGUI@@QBEABVString@2@XZ")(reinterpret_cast<void*>(layout.Word(0x38u)));return std::string(detail::Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ")(text));};
    Check(total_text()==std::to_string(3u*property_price),"actual quantity change updates original total-price label");
    Check(confirm->Prompt() && visible("BtYes") && visible("BtNo"),"actual original confirmation offers Yes/No");choose("BtNo");
    Check(!inventory.ItemCount(property) && balance()==1000000 && !pages,"No selection leaves actual inventory untouched");
    retire_during_refresh=true;std::array<unsigned,5> enter{};enter[4]=28u;
    Check(input.Predicate(layout.FrameAddress()+0x1a8u,0u,Address(enter.data())) && visible("BtYes"),"actual Trade Enter input opens confirmation after No");choose("BtYes");retire_during_refresh=false;
    Check(inventory.ItemCount(property)==3u && balance()==1000000-static_cast<int>(3u*property_price) && pages==1u && retirement_rejected && observed==std::vector<unsigned>{property} && visible("BtOk"),"Yes reads column1, buys actual items and shows reentrant success dialog");choose("BtOk");
    const auto after_purchase=balance();choose("BtYes");Check(inventory.ItemCount(property)==3u && balance()==after_purchase,"replaced confirmation callback cannot purchase twice");
    const auto entry=NativeInventoryPropertyReader(inventory).FindId(property);Check(bool(entry) && entry->sequence!=property,"actual property sequence key");
    stage(1u,false,entry->sequence,2);const auto before_sale=balance();Check(total_text()==std::to_string(2u*(property_price/4u)),"actual sell quantity uses quarter unit price in total label");Check(confirm->Prompt(),"property sale prompt");choose("BtYes");
    Check(inventory.ItemCount(property)==1u && balance()==before_sale+static_cast<int>(2u*(property_price/4u)) && pages==2u,"confirmed batch sale resolves inventory sequence and credits actual proceeds");choose("BtOk");
    stage(1u,true,property,2);money(0);Check(confirm->Prompt(),"insufficient-funds confirmation");choose("BtYes");
    Check(!balance() && inventory.ItemCount(property)==1u && pages==2u && visible("BtOk"),"failed purchase shows error without list refresh or inventory mutation");choose("BtOk");
    const auto type=Word(inventory.EquipmentDefinitionAddress(equipment)+148u),gear_price=Word(inventory.EquipmentDefinitionAddress(equipment)+376u);
    stage(0u,true,equipment,0);money(1000000);Check(confirm->Prompt(),"equipment buy prompt");choose("BtYes");
    Check(inventory.EquipmentCount(type)==1u && balance()==1000000-static_cast<int>(gear_price) && displays==1u && pages==3u,"confirmed equipment purchase creates original instance and refreshes detail/page");choose("BtOk");
    const auto instance=inventory.EquipmentAt(type,0u),sequence=Word(instance+400u);const auto player=Address(&players.slots[0]);*reinterpret_cast<unsigned*>(player+0xb08u)=1u;*reinterpret_cast<unsigned*>(player+0x920u)=sequence;
    stage(0u,false,sequence,0);Check(confirm->Prompt() && visible("BtOk") && !visible("BtYes"),"equipped item warning blocks sale confirmation");choose("BtOk");*reinterpret_cast<unsigned*>(player+0x920u)=0u;record_pages->Equipment(type);
    Check(confirm->Prompt(),"unequipped instance sale prompt");const auto gear_balance=balance();choose("BtYes");
    Check(!inventory.EquipmentCount(type) && balance()==gear_balance+static_cast<int>(gear_price/4u) && pages==4u,"confirmed equipment sale removes selected original instance");choose("BtOk");
    const auto recipe=recipes.Records().front()[0];stage(2u,true,recipe,0);money(1000000);Check(confirm->Prompt(),"recipe confirmation");choose("BtYes");
    Check(NativeInventoryPrescriptions(inventory,recipes).Contains(std::bit_cast<int>(recipe)) && pages==5u,"confirmed recipe purchase reaches existing learned collection");choose("BtOk");
    Check(confirm->Prompt() && visible("BtOk") && !visible("BtYes"),"known recipe uses original warning");choose("BtOk");
    stage(1u,true,property,1);Check(confirm->Prompt(),"prompt before cancellation");const auto before_cancel=balance();confirm->Cancel();choose("BtYes");
    Check(balance()==before_cancel && inventory.ItemCount(property)==1u,"cancelled callback cannot execute stale purchase");
    Check(confirm->Prompt(),"prompt before foreign replacement");unsigned foreign{};dialog.Show(U"foreign prompt",3u,[&](auto choice){foreign=choice;},services.sound);confirm->Close();choose("BtYes");Check(foreign==1u,"Trade retirement preserves another owner's replacement prompt");
    bool retired{};try{(void)confirm->Prompt();}catch(const std::logic_error&){retired=true;}Check(retired,"retired confirmation rejects a new prompt");
    dialog.Close();dialog.ReleaseSelectionCallback();
    // Exercise the actual page algorithm over every original property category,
    // rather than only the one product used by the confirmation cases.
    record_pages->Clear();property_definitions.clear();for(auto id:Ids(db,"Property")){const auto definition=inventory.PropertyAddress(id);if(definition)property_definitions.push_back(definition);}
    layout.Word(0x144u)=Address(property_definitions.data());layout.Word(0x148u)=Address(property_definitions.data()+property_definitions.size());layout.Word(0x14cu)=layout.Word(0x148u);
    layout.Word(0xe8u)=1u;layout.Word(0xf8u)=1u;money(1000000);
    const auto count_rows=[&]{return detail::Export<unsigned (__thiscall*)(const void*)>(module,"?getRowCount@MultiColumnList@CEGUI@@QBEIXZ")(reinterpret_cast<void*>(list));};
    const auto cell_at=[&](unsigned row,unsigned column){const std::array grid{row,column};return Address(detail::Export<void* (__thiscall*)(const void*,const void*)>(module,"?getItemAtGridReference@MultiColumnList@CEGUI@@QBEPAVListboxItem@2@ABUMCLGridRef@2@@Z")(reinterpret_cast<void*>(list),grid.data()));};
    const auto text_at=[&](unsigned row,unsigned column){const auto item=cell_at(row,column);const auto text=detail::Export<const void* (__thiscall*)(const void*)>(module,"?getText@ListboxItem@CEGUI@@QBEABVString@2@XZ")(reinterpret_cast<void*>(item));return std::string(detail::Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ")(text));};
    unsigned original_rows{};
    for(unsigned category=0;category<5u;++category){std::vector<unsigned> expected;for(auto definition:property_definitions)if(Word(definition+4u)==category)expected.push_back(definition);
        property_pages->Update(category);Check(count_rows()==expected.size() && detail::Export<unsigned (__thiscall*)(const void*)>(module,"?getColumnCount@MultiColumnList@CEGUI@@QBEIXZ")(reinterpret_cast<void*>(list))==4u,"all original property categories generate exact row count and four columns");
        for(unsigned row=0;row<expected.size();++row){const auto definition=expected[row],id=Word(definition),cell=cell_at(row,1u);const auto owned=inventory.ItemCount(id);
            Check(Word(cell+0x138u)==id && Word(cell+0x134u)==row && Word(Word(cell+424u)+472u)==99u-owned && text_at(row,2u)==std::to_string(owned) && text_at(row,3u)==std::to_string(std::bit_cast<int>(Word(definition+144u))),"real buy rows preserve product/order/row ID, owned stock, signed price and numeric limit");
            if(id==property){pool->SetValue(cell,999);Check(pool->Value(cell)==static_cast<int>(99u-owned),"actual buy page clamps to99 minus owned quantity");}
            ++original_rows;
        }
        if(!expected.empty()){money(0);property_pages->Update(category);for(unsigned row=0;row<expected.size();++row)Check((*reinterpret_cast<const unsigned char*>(cell_at(row,1u)+421u)!=0u)==(std::bit_cast<int>(Word(expected[row]+144u))>0),"unaffordable original quantity rows have original disabled appearance");money(1000000);}
    }
    std::array<bool,5> seeded{};for(auto definition:property_definitions){const auto category=Word(definition+4u);if(category<5u && !seeded[category] && Word(definition+160u)){(void)inventory.AddItem(Word(definition),7);seeded[category]=true;}}
    layout.Word(0xf8u)=0u;
    for(unsigned category=0;category<5u;++category){std::vector<NativeInventoryProperty> expected;NativeInventoryPropertyReader reader(inventory);
        for(int index=0;index<reader.Count(category);++index){const auto item=reader.AtReverse(category,index);Check(bool(item),"original reverse inventory row");if(Word(item->definition+160u))expected.push_back(*item);}
        property_pages->Update(category);Check(count_rows()==expected.size(),"sell page filters original unsellable records");
        for(unsigned row=0;row<expected.size();++row){const auto& item=expected[row];const auto cell=cell_at(row,1u);Check(Word(cell+0x138u)==item.sequence && Word(Word(cell+424u)+472u)==item.count && text_at(row,2u)==std::to_string(item.count) && text_at(row,3u)==std::to_string(std::bit_cast<int>(Word(item.definition+144u))/4),"sell page preserves reverse order, sequence key, stock limit and quarter-price text");}
    }
    inventory.Reset();const auto before_empty=clears;property_pages->Update(0u);Check(!count_rows() && clears==before_empty+1u && original_rows>0u && total_text()=="0","empty live inventory page clears details");
    property_pages->Clear();equipment_definitions.clear();for(auto id:Ids(db,"Equip")){const auto definition=inventory.EquipmentDefinitionAddress(id);if(definition)equipment_definitions.push_back(definition);}
    layout.Word(0x134u)=Address(equipment_definitions.data());layout.Word(0x138u)=Address(equipment_definitions.data()+equipment_definitions.size());layout.Word(0x13cu)=layout.Word(0x138u);
    layout.Word(0xe8u)=0u;layout.Word(0xf8u)=1u;money(1000000);unsigned equipment_rows{};
    const auto level=[&](unsigned definition,bool positive){int value{};for(unsigned i=0;i<4u;++i){value=std::bit_cast<int>(Word(definition+360u+4u*i));if(positive?value>0:value!=0)break;}return value;};
    std::array<unsigned,7> representatives{};
    for(unsigned category=0;category<7u;++category){std::vector<unsigned> expected;for(auto definition:equipment_definitions)if(Word(definition+148u)==category)expected.push_back(definition);
        record_pages->Equipment(category);Check(count_rows()==expected.size() && detail::Export<unsigned (__thiscall*)(const void*)>(module,"?getColumnCount@MultiColumnList@CEGUI@@QBEIXZ")(reinterpret_cast<void*>(list))==3u,"all original equipment categories build exact three-column rows");
        for(unsigned row=0;row<expected.size();++row){const auto definition=expected[row];Check(Word(cell_at(row,0u)+312u)==Word(definition+144u) && text_at(row,1u)==std::to_string(level(definition,true)) && text_at(row,2u)==std::to_string(std::bit_cast<int>(Word(definition+376u))),"real equipment buy key, level and full signed price");++equipment_rows;}
        if(!expected.empty()){representatives[category]=Word(expected.front()+144u);money(0);record_pages->Equipment(category);for(unsigned row=0;row<expected.size();++row)Check((*reinterpret_cast<const unsigned char*>(cell_at(row,0u)+421u)!=0u)==(std::bit_cast<int>(Word(expected[row]+376u))>0),"equipment buy affordability uses actual money");money(1000000);}
    }
    record_pages->Equipment(7u);unsigned category_zero{};for(auto definition:equipment_definitions)category_zero+=Word(definition+148u)==0u;
    Check(layout.Word(0xecu)==0u && count_rows()==category_zero,"original buy page7 remaps to equipment category0");
    for(auto id:representatives)if(id)Check(!trade.BuyEquipment(id),"seed actual equipment sale inventory");
    layout.Word(0xf8u)=0u;
    record_pages->Equipment(7u);Check(!count_rows(),"original out-of-range sale slot7 yields an empty page");
    for(unsigned category=0;category<7u;++category){record_pages->Equipment(category);Check(count_rows()==inventory.EquipmentCount(category),"unequipped sale page enumerates actual category instances");for(unsigned row=0;row<count_rows();++row){const auto instance=inventory.EquipmentAt(category,row);Check(Word(cell_at(row,0u)+312u)==Word(instance+400u) && text_at(row,2u)==std::to_string(std::bit_cast<int>(Word(instance+376u))/4),"actual equipment sale uses instance sequence and quarter price");}}
    const auto equipped_type=Word(inventory.EquipmentDefinitionAddress(equipment)+148u),equipped_record=inventory.EquipmentAt(equipped_type,0u),equipped_sequence=Word(equipped_record+400u);
    *reinterpret_cast<unsigned*>(player+0xb08u)=1u;*reinterpret_cast<unsigned*>(player+0x920u)=equipped_sequence;record_pages->Equipment(equipped_type);
    Check(count_rows()==1u && *reinterpret_cast<const unsigned char*>(cell_at(0u,0u)+421u) && Word(cell_at(0u,0u)+528u),"active player's equipment has real equipped icon and disabled sale row");
    *reinterpret_cast<unsigned*>(player+0xb08u)=0u;record_pages->Equipment(equipped_type);Check(!count_rows(),"inactive player's equipment is filtered from actual sale page");
    *reinterpret_cast<unsigned*>(player+0x920u)=0u;record_pages->Equipment(equipped_type);Check(count_rows()==1u,"removing inactive player's slot restores sale availability");
    recipe_definitions.clear();for(const auto& record:recipes.Records())recipe_definitions.push_back(Address(record.data()));
    layout.Word(0x154u)=Address(recipe_definitions.data());layout.Word(0x158u)=Address(recipe_definitions.data()+recipe_definitions.size());layout.Word(0x15cu)=layout.Word(0x158u);
    layout.Word(0xe8u)=2u;layout.Word(0xf8u)=1u;money(1000000);Check(!trade.BuyRecipe(recipe),"seed actual learned recipe state");unsigned recipe_rows{};
    for(unsigned mode=0;mode<3u;++mode){std::vector<unsigned> expected;for(auto definition:recipe_definitions)if(Word(definition+100u)==mode && (mode || inventory.EquipmentDefinitionAddress(Word(definition+104u))))expected.push_back(definition);
        record_pages->Recipes(mode);Check(count_rows()==expected.size() && detail::Export<unsigned (__thiscall*)(const void*)>(module,"?getColumnCount@MultiColumnList@CEGUI@@QBEIXZ")(reinterpret_cast<void*>(list))==(mode?3u:4u),"all original recipe modes build exact layout and product filter");
        for(unsigned row=0;row<expected.size();++row){const auto definition=expected[row],id=Word(definition);Check(Word(cell_at(row,0u)+312u)==id && text_at(row,mode?2u:3u)==std::to_string(std::bit_cast<int>(Word(definition+120u))) && (*reinterpret_cast<const unsigned char*>(cell_at(row,0u)+421u)!=0u)==(id==recipe),"recipe rows preserve game key, price and actual learned-state disabled flag");
            if(mode)Check(text_at(row,1u)==std::to_string(std::bit_cast<int>(Word(definition+(mode==1u?108u:112u)))),"recipe requirement text matches original mode");else Check(text_at(row,2u)==std::to_string(level(inventory.EquipmentDefinitionAddress(Word(definition+104u)),false)),"crafting recipe product level matches original equipment");++recipe_rows;}
        money(0);record_pages->Recipes(mode);for(unsigned row=0;row<expected.size();++row)Check((*reinterpret_cast<const unsigned char*>(cell_at(row,0u)+421u)!=0u)==(Word(expected[row])==recipe || std::bit_cast<int>(Word(expected[row]+120u))>0),"recipe affordability combines actual funds and learned state");money(1000000);
    }
    layout.Close();inventory.Close();Check(!pool->OutstandingAllocations() && !inventory.OutstandingAllocations() && !player_memory.OutstandingAllocations() && !sounds.empty() && resets>0u && quantity_changes>0u,"confirmation/page/spinner/inventory/player ownership balances");
    std::cout<<"PASS: native equipment/recipe pages "<<equipment_rows<<" equipment records/seven types and "<<recipe_rows<<" recipes/three modes, actual prices/levels/requirements, player equipped icon/active-disable/inactive-filter and learned/money gates\n";
    std::cout<<"PASS: native property pages "<<original_rows<<" original buy rows/five categories, actual stock/price/disabled/limits, reverse sale keys/filter and empty-page clear\n";
    std::cout<<"PASS: original Trade confirmation Yes/No, quantity column1, actual property/equipment/recipe transactions, error/warning dialogs, reentrant result, cancellation and foreign dialog retirement; actual equipment/property/recipe page rebuild after transactions and player equipped gates; model/audio policies explicit\n";
}
}

