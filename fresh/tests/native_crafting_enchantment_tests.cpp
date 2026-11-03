#include "pal4/native_crafting_enchantment.h"
#include "pal4/native_inventory_equipment_mutation.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_prescription_record_catalog.h"
#include "pal4/native_inventory_save_runtime.h"
#include "pal4/native_inventory_load_runtime.h"
#include <windows.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>

namespace {
void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
unsigned Word(unsigned a){unsigned v;std::memcpy(&v,reinterpret_cast<const void*>(a),4);return v;}
std::string Text(unsigned descriptor){const auto data=Word(descriptor+4u),length=Word(descriptor+8u);return std::string(data?reinterpret_cast<const char*>(data):"",length);}
}
int main(int argc,char** argv)try{
    Check(argc==2,"original root required");pal4::NativeGameDatabase db;Check(db.Load(argv[1]),"original database");
    pal4::NativePrescriptionRecordCatalog recipes(db);pal4::NativeInventorySession inventory;inventory.BindDatabase(db);(void)inventory.Start();
    unsigned sounds{};inventory.BindItemObserver([](unsigned){});
    pal4::NativeCraftingEnchantment enchant(recipes,inventory,[&](std::string_view name,unsigned mode){Check(name=="wa09.wav" && mode==1u,"original enchant sound");++sounds;});
    pal4::NativeInventoryEquipmentMutation mutate(inventory);
    const auto& table=db.Tables().at("Prescription");
    auto expected=[&](std::size_t row,const std::string& name)->const pal4::NativeGameDbValue&{
        const auto f=std::find_if(table.fields.begin(),table.fields.end(),[&](const auto& field){return field.name==name;});
        Check(f!=table.fields.end(),"original expected Prescription field");return table.rows[row][f-table.fields.begin()];
    };
    std::array<unsigned,7> equipment{};const auto& equips=db.Tables().at("Equip");
    const auto id_field=std::find_if(equips.fields.begin(),equips.fields.end(),[](const auto& f){return f.name=="ID";});Check(id_field!=equips.fields.end(),"Equip ID");
    for(const auto& row:equips.rows){const auto id=std::get<unsigned>(row[id_field-equips.fields.begin()]);const auto def=inventory.EquipmentDefinitionAddress(id);
        const auto type=Word(def+0x94u);if(type<7u && !equipment[type])equipment[type]=id;}
    for(const auto id:equipment)Check(id!=0u,"actual equipment definition for all seven types");
    unsigned cases{},last_sequence{};std::array<unsigned,7> type_cases{};
    for(std::size_t row=0;row<table.rows.size();++row){const auto& recipe=recipes.Records()[row];if(recipe[25]!=2u)continue;
        for(const unsigned type:{3u,4u,5u,6u,0u,1u,2u})for(unsigned mode=0;mode<2u;++mode){
            inventory.Reset();(void)inventory.UpdateCounter(1000);
            const auto entry=inventory.ProcessEquipment(inventory.Address()+0x7cu,equipment[type]);const auto seq=last_sequence=Word(entry+0x190u);
            const auto other=inventory.ProcessEquipment(inventory.Address()+0x7cu,equipment[type]);
            const auto other_text=Text(other+0xb8u);
            const auto other_skill=Word(other+0x180u),definition=inventory.EquipmentDefinitionAddress(equipment[type]);
            const auto definition_text=Text(definition+0xb8u);
            mutate.AssignText(seq,0xf8u,"old-effect");
            std::map<unsigned,unsigned> materials;
            for(unsigned n=1;n<=4;++n){const auto prefix="P_MAT_"+std::to_string(n);const auto id=std::get<unsigned>(expected(row,prefix)),count=std::get<unsigned>(expected(row,prefix+"_NUMBER"));
                if(static_cast<int>(count)>0)materials.emplace(id,count);}
            for(const auto [id,count]:materials){Check(inventory.PropertyAddress(id)!=0u,"actual enchant ingredients are direct items");(void)inventory.AddItem(id,static_cast<int>(count+2u));}
            const auto sequence_order=inventory.RecentEquipmentSequences();
            Check(enchant.Apply(recipe[0],seq,mode!=0u)==0,"original enchant success");
            Check(Word(entry+0x180u)==std::get<unsigned>(expected(row,"PRA_SKILL_ID")),"actual instance skill write");
            Check(Text(entry+0xb8u)==std::get<std::string>(expected(row,"PRA_PROPERTY")) && Text(entry+0x108u)==std::get<std::string>(expected(row,"PRA_NAME")),"actual enchant property and name bytes");
            const auto effect=type<3u?std::get<std::string>(expected(row,"PRA_EF"+std::to_string(type+2u))):"old-effect";
            Check(Text(entry+0xf8u)==effect,"original type0/1/2 effect choice and other types untouched");
            Check(Word(entry+0x90u)==equipment[type] && Word(entry+0x94u)==type && Word(entry+0x190u)==seq && inventory.RecentEquipmentSequences()==sequence_order,"enchantment preserves instance and collection identity");
            Check(Text(other+0xb8u)==other_text && Word(other+0x180u)==other_skill && Text(definition+0xb8u)==definition_text,"other copy and shared definition stay independent");
            for(const auto [id,count]:materials)Check(inventory.ItemCount(id)==2u,"exact enchant material consumption");
            Check(inventory.ReadDword(inventory.Address()+0x134u)==(mode?1000u:920u),"enchant fee flag");
            // Repeat on the same native descriptors: no accumulating text owners.
            for(const auto [id,count]:materials)(void)inventory.AddItem(id,static_cast<int>(count));
            const auto allocations=inventory.OutstandingAllocations();
            Check(enchant.Apply(recipe[0],seq,true)==0 && inventory.OutstandingAllocations()==allocations,"repeat enchant releases replaced text buffers");
            for(const auto [id,count]:materials)Check(inventory.ItemCount(id)==2u,"repeat enchant retains exact material surplus");
            const auto old=Text(entry+0x108u);const auto data=Word(entry+0x10cu);
            mutate.AssignText(seq,0x108u,std::string_view(data?reinterpret_cast<const char*>(data):"",old.size()));
            Check(Text(entry+0x108u)==old,"aliased text assignment preserves source");
            ++cases;++type_cases[type];
        }
    }
    // Mutated instances survive the actual inventory save/load codecs.
    const auto current=inventory.FindInventoryEntry(inventory.Address()+0x7cu,last_sequence,7u);
    const auto saved_name=Text(current+0x108u),saved_effect=Text(current+0xf8u),saved_property=Text(current+0xb8u);const auto saved_skill=Word(current+0x180u);
    mutate.WriteScalar(last_sequence,0x184u,0xfffffffeu);mutate.WriteScalar(last_sequence,4u,12345u);
    pal4::NativeGameSaveArchiveBuffer buffer;pal4::NativeInventorySaveRuntime save(inventory,buffer);Check(save.Serialize()==1u,"serialize modified inventory");
    const auto root=std::filesystem::path("H:/PAL4/objs/enchantment-roundtrip")/std::to_string(GetCurrentProcessId());std::filesystem::create_directories(root/"save");
    {std::ofstream file(root/"save/0.dat",std::ios::binary);file.write(reinterpret_cast<const char*>(buffer.Bytes().data()),buffer.Bytes().size());Check(static_cast<bool>(file),"write generated inventory fixture");}
    pal4::NativeInventorySession restored;restored.BindDatabase(db);(void)restored.Start();pal4::NativeGameSaveArchiveReader input;
    Check(input.OpenSlot(root,0),"open mutated inventory fixture");pal4::NativeInventoryLoadRuntime load(restored,input);Check(load.Deserialize()==1u && !input.Remaining(),"restore full modified inventory");
    const auto actual=restored.FindInventoryEntry(restored.Address()+0x7cu,last_sequence,7u);
    Check(actual && Text(actual+0x108u)==saved_name && Text(actual+0xf8u)==saved_effect && Text(actual+0xb8u)==saved_property && Word(actual+0x180u)==saved_skill && Word(actual+0x184u)==0xfffffffeu && Word(actual+4u)==12345u,"text,skill,forge potential and scalar bits survive real save/load");
    restored.Close();Check(!restored.OutstandingAllocations(),"restored text released");
    bool forbidden{},missing{},bad_text{},bad_recipe{},bad_target{};
    try{mutate.WriteScalar(last_sequence,0x90u,1u);}catch(const std::invalid_argument&){forbidden=true;}
    try{mutate.AssignText(last_sequence,0x99u,"bad");}catch(const std::invalid_argument&){bad_text=true;}
    try{mutate.AssignText(0xfefefefeu,0x108u,"bad");}catch(const std::out_of_range&){missing=true;}
    const auto before_sound=sounds;
    try{(void)enchant.Apply(0xfefefefeu,last_sequence,true);}catch(const std::out_of_range&){bad_recipe=true;}
    try{(void)enchant.Apply(recipes.Records()[0][0],0xfefefefeu,true);}catch(const std::out_of_range&){bad_target=true;}
    Check(forbidden && missing && bad_text && bad_recipe && bad_target && before_sound==sounds,"mutation bounds and invalid host requests");
    mutate.AssignText(last_sequence,0x108u,"");Check(Text(current+0x108u).empty() && !Word(current+0x10cu),"empty assignment clears owned storage");
    Check(inventory.RemoveEquipment(last_sequence),"delete mutated instance");inventory.Close();Check(!inventory.OutstandingAllocations(),"all changed native text buffers released");
    Check(cases==728u && sounds==1456u,"all52 enchant recipes seven types two entry modes and repeat writes");
    std::cout<<"PASS original enchant cases="<<cases<<" repeats="<<cases<<" sound-calls="<<sounds<<"; 52 recipes,7 real equipment types,live mutation/alias/bounds/save-load/cleanup\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
