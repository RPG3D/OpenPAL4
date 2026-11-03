#include "pal4/native_inventory_trade.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_inventory_prescriptions.h"
#include "pal4/native_inventory_property_reader.h"
#include "pal4/native_prescription_record_catalog.h"
#include <array>
#include <bit>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace {
void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
unsigned Word(unsigned address){return *reinterpret_cast<const unsigned*>(address);}
std::vector<unsigned> Ids(const pal4::NativeGameDatabase& db,std::string_view name){
    std::vector<unsigned> ids;
    for(const auto& [table_name,table]:db.Tables())if(table_name==name){
        std::size_t column=0;while(column<table.fields.size() && table.fields[column].name!="ID")++column;
        Check(column<table.fields.size(),"original ID column");for(const auto& row:table.rows)ids.push_back(std::get<unsigned>(row[column]));
    }
    Check(!ids.empty(),"original table records");return ids;
}
}
int main(int argc,char** argv)try{
    Check(argc==2,"original root required");pal4::NativeGameDatabase db;Check(db.Load(argv[1]),"original database load");
    pal4::NativePrescriptionRecordCatalog recipes(db);pal4::NativeInventorySession inventory;
    inventory.BindDatabase(db);(void)inventory.Start();inventory.Reset();
    std::vector<unsigned> observed;inventory.BindItemObserver([&](auto id){observed.push_back(id);});
    pal4::NativeInventoryTrade trade(inventory,recipes);pal4::NativeInventoryPrescriptions learned(inventory,recipes);
    const auto balance=[&]{return std::bit_cast<int>(inventory.ReadDword(inventory.Address()+308u));};
    const auto money=[&](int value){(void)inventory.UpdateCounter(value-balance());};
    unsigned equipment{},property{},other{};
    for(const auto id:Ids(db,"Equip")){const auto row=inventory.EquipmentDefinitionAddress(id);if(row && Word(row+376u)>0u && Word(row+376u)<100000u && Word(row+148u)<7u){equipment=id;break;}}
    for(const auto id:Ids(db,"Property")){const auto row=inventory.PropertyAddress(id);if(row && Word(row+144u)>0u && Word(row+144u)<100000u){if(!property)property=id;else{other=id;break;}}}
    Check(equipment && property && other,"original positive-price equipment and two properties");
    const auto equipment_row=inventory.EquipmentDefinitionAddress(equipment),type=Word(equipment_row+148u),price=Word(equipment_row+376u);
    Check(trade.BuyEquipment(equipment)==45 && !inventory.EquipmentCount(type) && !balance(),"insufficient equipment funds preserves inventory");
    money(1000000);const auto before_gear=balance();
    Check(!trade.BuyEquipment(equipment) && !trade.BuyEquipment(equipment) && balance()==before_gear-static_cast<int>(2u*price) && inventory.EquipmentCount(type)==2u,
        "two actual equipment purchases debit definition price and create two owned instances");
    const auto first=inventory.EquipmentAt(type,0u),second=inventory.EquipmentAt(type,1u);
    const auto first_sequence=Word(first+400u),second_sequence=Word(second+400u);
    Check(first_sequence!=second_sequence,"equipment sale keys are unique instance sequences");
    // Explicit fixture mutation proves sale price comes from this instance,
    // not the immutable definition; both copies otherwise share the same ID.
    *reinterpret_cast<unsigned*>(first+376u)=7u;
    const auto before_sale=balance();
    Check(trade.SellEquipment(first_sequence)==1u && balance()==before_sale+1 && inventory.EquipmentCount(type)==1u &&
        inventory.FindInventoryEntry(inventory.Address()+124u,second_sequence,7u)==second &&
        !inventory.FindInventoryEntry(inventory.Address()+124u,first_sequence,7u),
        "sale truncates instance price/4, removes only its sequence and retains duplicate definition");
    Check(!trade.SellEquipment(first_sequence) && balance()==before_sale+1,"selling an absent sequence is a no-op");
    const auto before_second_sale=balance();Check(trade.SellEquipment(second_sequence)==1u && balance()==before_second_sale+static_cast<int>(price/4u) && !inventory.EquipmentCount(type),"second instance remains independently sellable");
    money(0);const auto item_price=Word(inventory.PropertyAddress(property)+144u),other_price=Word(inventory.PropertyAddress(other)+144u);
    Check(trade.BuyProperty(property,1u)==45 && !inventory.ItemCount(property) && observed.empty(),"insufficient property funds leaves item/observer untouched");
    money(1000000);const auto before_property=balance();
    Check(!trade.BuyProperty(property,3u) && inventory.ItemCount(property)==3u && balance()==before_property-static_cast<int>(3u*item_price) && observed==std::vector<unsigned>{property},"property purchase uses actual collection and observer");
    const auto property_entry=pal4::NativeInventoryPropertyReader(inventory).FindId(property);
    Check(property_entry && property_entry->sequence!=property,"property sequence is distinct from definition ID");
    const auto property_sequence=property_entry->sequence;
    const auto before_short=balance();Check(trade.SellProperty(property_sequence,4u)==46 && inventory.ItemCount(property)==3u && balance()==before_short,"overselling refuses before payment/removal");
    Check(!trade.SellProperty(property_sequence,2u) && inventory.ItemCount(property)==1u && balance()==before_short+static_cast<int>(2u*(item_price/4u)),"property sale credits truncated unit proceeds and removes quantity");
    const std::array<pal4::ItemQuantity,2> batch{{{property,2u},{other,3u}}};
    money(static_cast<int>(item_price*2u));const auto before_batch=balance();const auto count=inventory.ItemCount(property);const auto observers=observed.size();
    Check(trade.BuyProperties(batch)==45 && balance()==before_batch && inventory.ItemCount(property)==count && !inventory.ItemCount(other) && observed.size()==observers,"batch purchase preflight prevents partial mutation when later item is unaffordable");
    money(1000000);const auto before_batch_success=balance();Check(!trade.BuyProperties(batch) && inventory.ItemCount(property)==3u && inventory.ItemCount(other)==3u && balance()==before_batch_success-static_cast<int>(2u*item_price+3u*other_price),"batch purchase mutates both actual item groups after preflight");
    const std::array<pal4::ItemQuantity,2> duplicate{{{property_sequence,2u},{property_sequence,2u}}};const auto before_duplicate=balance();
    Check(!trade.SellProperties(duplicate) && inventory.ItemCount(property)==1u && balance()==before_duplicate+static_cast<int>(2u*(item_price/4u)),"duplicate batch preserves original per-entry preflight and ignored later insufficient-quantity result");
    const auto before_missing=balance();Check(!trade.BuyEquipment(0xfefefefeu) && !trade.BuyProperty(0xfefefefeu,1u) && !trade.SellProperty(0xfefefefeu,1u) && !trade.BuyRecipe(0xfefefefeu) && balance()==before_missing,"unknown original record IDs preserve missing-record behavior");
    unsigned learned_count{};std::array<unsigned,3> modes{};
    for(const auto& row:recipes.Records()){
        const auto id=row[0],recipe_price=row[30],mode=row[25];Check(mode<3u,"original recipe mode");
        if(recipe_price){money(0);Check(trade.BuyRecipe(id)==45 && !learned.Contains(std::bit_cast<int>(id)),"insufficient recipe funds preserves learned collection");}
        money(1000000);const auto before_recipe=balance();
        Check(!trade.BuyRecipe(id) && learned.Contains(std::bit_cast<int>(id)) && balance()==before_recipe-static_cast<int>(recipe_price),"recipe purchase debits actual price and stores original mode/ID");
        const auto known_balance=balance();Check(!trade.BuyRecipe(id) && balance()==known_balance,"known recipe never charges twice");++learned_count;++modes[mode];
    }
    Check(learned_count==recipes.Records().size(),"all original recipes bought");for(int mode=0;mode<3;++mode)Check(learned.Count(mode)==modes[mode],"all recipe modes share existing inventory collection");
    inventory.Reset();Check(!balance() && !inventory.ItemCount(property) && !learned.Count(0) && !learned.Count(1) && !learned.Count(2),"NewGame reset clears the same traded inventory");
    inventory.Close();Check(!inventory.OutstandingAllocations(),"traded inventory releases all actual owned records and text");
    bool closed{};try{(void)trade.BuyProperty(property,1u);}catch(const std::logic_error&){closed=true;}Check(closed,"trade rejects retired inventory");
    std::cout<<"PASS: native equipment/property/batch/recipe trade, actual money and inventory mutation, per-instance sale, observer/preflight/duplicate behavior, all "<<learned_count<<" recipes, Reset/Close\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
