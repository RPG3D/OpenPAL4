#include "pal4/native_crafting_requirements.h"
#include "pal4/native_game_database.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_prescription_record_catalog.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>

namespace {
void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
unsigned Word(unsigned address){unsigned value;std::memcpy(&value,reinterpret_cast<const void*>(address),4);return value;}
}
int main(int argc,char** argv)try {
    Check(argc==2,"original game root required");
    pal4::NativeGameDatabase db;Check(db.Load(argv[1]),"original database");
    pal4::NativePrescriptionRecordCatalog recipes(db);
    pal4::NativeInventorySession inventory;inventory.BindDatabase(db);(void)inventory.Start();inventory.Reset();
    std::vector<unsigned> item_events;
    inventory.BindItemObserver([&](unsigned id){item_events.push_back(id);});
    pal4::NativeCraftingRequirements query(recipes,inventory);
    const auto& table=db.Tables().at("Prescription");
    auto field=[&](const auto& row,const std::string& name){
        const auto f=std::find_if(table.fields.begin(),table.fields.end(),[&](const auto& f){return f.name==name;});
        Check(f!=table.fields.end(),"original requirement field");return std::get<unsigned>(row[f-table.fields.begin()]);
    };
    unsigned rows{},direct{},equipment{},unknown{},partial{},distractors{};
    for(std::size_t r=0;r<table.rows.size();++r){
        inventory.Reset();const auto baseline=inventory.OutstandingAllocations();
        const auto& row=table.rows[r];const auto key=recipes.Records()[r][0];
        std::vector<pal4::NativeCraftingDeficit> expected;
        std::map<unsigned,unsigned> needed;std::int32_t first=-1;
        std::array<unsigned,4> ids{},counts{};
        for(unsigned slot=0;slot<4;++slot){
            const auto name="P_MAT_"+std::to_string(slot+1u);
            const auto id=ids[slot]=field(row,name);const auto count=counts[slot]=field(row,name+"_NUMBER");
            if(static_cast<int>(count)<=0)continue;
            const auto property=inventory.PropertyAddress(id),definition=inventory.EquipmentDefinitionAddress(id);
            if(!property && !definition){++unknown;continue;}
            expected.push_back({id,count});needed[id]=std::max(needed[id],count);
            if(property)++direct;
            else {++equipment;if(first==-1)first=static_cast<int>(Word(definition+0x90u));}
        }
        Check(query.Deficits(key)==expected && query.AreMet(key)==expected.empty(),"empty inventory exact ordered deficits and requirement state");
        Check(query.HasEquipmentMaterial(key)==(first!=-1) && query.FirstEquipmentValue(key)==first,"equipment material predicate and first definition value");
        Check(query.Product(key)==field(row,"PRA_PRODUCT_ID"),"actual recipe product");
        Check(inventory.OutstandingAllocations()==baseline,"queries release scratch and leave inventory untouched");
        // Other equipment of the same type must not satisfy a base-ID material.
        for(const auto [id,count]:needed)if(const auto def=inventory.EquipmentDefinitionAddress(id)){
            const auto& equips=db.Tables().at("Equip");
            const auto id_field=std::find_if(equips.fields.begin(),equips.fields.end(),[](const auto& f){return f.name=="ID";});
            Check(id_field!=equips.fields.end(),"Equip ID field");
            bool found{};
            for(const auto& candidate:equips.rows){
                const auto other=std::get<unsigned>(candidate[id_field-equips.fields.begin()]);
                const auto other_def=inventory.EquipmentDefinitionAddress(other);
                if(other_def && !needed.contains(other) && Word(other_def+0x94u)==Word(def+0x94u)){
                    Check(inventory.ProcessEquipment(inventory.Address()+0x7cu,other)!=0u,"create same-type unrelated equipment");
                    ++distractors;found=true;break;
                }
            }
            Check(found && query.Deficits(key)==expected && !query.AreMet(key),"same-type unrelated equipment is not a recipe material");
        }
        // Independently populate live storage, one below the requirement, then
        // finish it. Repeated material slots retain original per-slot semantics.
        for(const auto [id,count]:needed){
            const auto fill=count-1u;
            if(inventory.PropertyAddress(id)){if(fill)(void)inventory.AddItem(id,static_cast<int>(fill));}
            else for(unsigned n=0;n<fill;++n)Check(inventory.ProcessEquipment(inventory.Address()+0x7cu,id)!=0u,"create original equipment material instance");
        }
        std::vector<pal4::NativeCraftingDeficit> below;
        for(unsigned slot=0;slot<4;++slot)if(needed.contains(ids[slot]) && static_cast<int>(counts[slot])>0){
            const auto owned=needed.at(ids[slot])-1u;
            if(owned<counts[slot])below.push_back({ids[slot],counts[slot]-owned});
        }
        Check(query.Deficits(key)==below && query.AreMet(key)==below.empty(),"partial real inventory quantity deficits");
        if(!below.empty())++partial;
        for(const auto [id,count]:needed){
            if(inventory.PropertyAddress(id))(void)inventory.AddItem(id,1);
            else Check(inventory.ProcessEquipment(inventory.Address()+0x7cu,id)!=0u,"last equipment material instance");
        }
        const auto filled=inventory.OutstandingAllocations();const auto event_count=item_events.size();
        Check(query.AreMet(key) && query.Deficits(key).empty(),"fully populated original recipe requirements");
        Check(query.AreMet(key) && inventory.OutstandingAllocations()==filled,"repeated queries do not consume or allocate inventory");
        Check(item_events.size()==event_count,"requirements do not publish item mutation events");
        for(const auto [id,count]:needed)if(inventory.PropertyAddress(id))Check(inventory.ItemCount(id)==count,"read-only property counts preserved");
        Check(inventory.ReadDword(inventory.Address()+0x134u)==0u,"requirements do not debit money");
        ++rows;
    }
    bool missing{};try{(void)query.AreMet(0xfefefefeu);}catch(const std::out_of_range&){missing=true;}
    Check(missing,"missing recipe rejected before memory read");
    inventory.Close();bool closed{};try{(void)query.Product(recipes.Records()[0][0]);}catch(const std::logic_error&){closed=true;}
    Check(closed && inventory.OutstandingAllocations()==0u,"closed inventory rejected and storage balanced");
    Check(rows==150u && direct && equipment && partial && distractors,"all original recipes and both material kinds exercised");
    std::cout<<"PASS original crafting requirements rows="<<rows<<" direct="<<direct<<" equipment="<<equipment<<" unknown="<<unknown<<" partial="<<partial<<" distractors="<<distractors<<"; empty/partial/full live inventories, no mutations\n";
    return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
