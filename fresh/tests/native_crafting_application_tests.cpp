#include "pal4/native_crafting_application.h"
#include "pal4/native_crafting_requirements.h"
#include "pal4/native_prescription_record_catalog.h"
#include "pal4/native_game_database.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_player_script_refresh_runtime.h"
#include "pal4/player_manager_object.h"
#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>

namespace {
void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
unsigned Address(const void* p){return reinterpret_cast<unsigned>(p);}
unsigned Word(unsigned a){unsigned v;std::memcpy(&v,reinterpret_cast<const void*>(a),4);return v;}
}
int main(int argc,char** argv)try {
    Check(argc==2,"original root required");pal4::NativeGameDatabase db;Check(db.Load(argv[1]),"original database");
    pal4::NativePrescriptionRecordCatalog recipes(db);pal4::NativeInventorySession inventory;
    inventory.BindDatabase(db);(void)inventory.Start();
    pal4::NativeBaseObjectMemory memory;pal4::PlayerManagerObjectState players{};
    pal4::NativePlayerScriptRefreshRuntime refresh(memory,inventory);
    unsigned sounds{},events{};inventory.BindItemObserver([&](unsigned){++events;});
    pal4::NativeCraftingApplication craft(recipes,inventory,players,memory,refresh,[&](std::string_view name,unsigned mode){
        Check(name=="wa09.wav" && mode==1u,"original crafting sound ABI");++sounds;
    });
    pal4::NativeCraftingRequirements requirements(recipes,inventory);
    auto materials=[](const auto& row){std::map<unsigned,unsigned> result;
        for(unsigned i=0;i<4;++i)if(static_cast<int>(row[0xa8u/4u+i])>0)result.emplace(row[0x98u/4u+i],row[0xa8u/4u+i]);return result;};
    auto prepare=[&](const auto& row){
        inventory.Reset();players.Fill(0u);(void)inventory.UpdateCounter(1000);
        unsigned selected=0xffffffffu;
        for(const auto [id,count]:materials(row)){
            if(inventory.PropertyAddress(id))(void)inventory.AddItem(id,static_cast<int>(count+2u));
            else {Check(inventory.HasEquipment(id) && count==1u,"original single equipment ingredient");
                const auto entry=inventory.ProcessEquipment(inventory.Address()+0x7cu,id);selected=Word(entry+0x190u);
                (void)inventory.ProcessEquipment(inventory.Address()+0x7cu,id); // another copy must survive
            }
        }
        Check(requirements.AreMet(row[0]),"populated original crafting requirements");return selected;
    };
    unsigned cases{},equipped{},failed{},first_key{};
    for(const auto& row:recipes.Records())if(row[25]==0u){
        Check(inventory.HasEquipment(row[0x68u/4u]),"original foundry output is equipment");if(!first_key)first_key=row[0];
        for(unsigned mode=0;mode<2;++mode){const auto selected=prepare(row);const auto before_events=events,before_sounds=sounds;
            const auto before_sequences=inventory.RecentEquipmentSequences();const auto before=before_sequences.size();
            Check(craft.Apply(row[0],selected,mode!=0u)==0,"original foundry success");
            Check(sounds==before_sounds+1u,"one sound per creation");
            Check(inventory.ReadDword(inventory.Address()+0x134u)==(mode?1000u:920u),"original system flag controls80-money charge");
            const auto recent=inventory.RecentEquipmentSequences();Check(recent.size()==before+(selected==0xffffffffu?1u:0u),"one output and exactly one selected removal");
            const auto product=inventory.FindInventoryEntry(inventory.Address()+0x7cu,recent.back(),7u);
            Check(product && Word(product+0x90u)==row[0x68u/4u],"real produced equipment instance definition");
            if(selected!=0xffffffffu)Check(!inventory.FindInventoryEntry(inventory.Address()+0x7cu,selected,7u),"old selected instance removed");
            for(const auto sequence:before_sequences)if(sequence!=selected)
                Check(inventory.FindInventoryEntry(inventory.Address()+0x7cu,sequence,7u)!=0u,"unselected material copies retain their exact instance sequence");
            for(const auto [id,count]:materials(row))if(inventory.PropertyAddress(id))Check(inventory.ItemCount(id)==2u,"exact real material decrement");
            Check(events==before_events,"original RemoveItem does not emit AddItem observer callbacks");++cases;
        }
        if(requirements.HasEquipmentMaterial(row[0])){
            // Each real equipment recipe replaces the owner's reference through
            // shared590AE0 and actually refreshes the native player component.
            for(unsigned owner=0;owner<4;++owner){const auto selected=prepare(row);
                const auto p=Address(&players.slots[owner]);memory.WriteDword(p+0xb08u,1u);
                const auto entry=inventory.FindInventoryEntry(inventory.Address()+0x7cu,selected,7u);
                pal4::InventoryItemSlotsState slots{};const auto slot=pal4::InventoryItemSlots::FindSlotOrClassify(slots,entry,inventory);
                Check(slot<6u,"original equipment slot classification");memory.WriteDword(p+0x920u+slot*4u,selected);
                memory.WriteDword(p+0x654u+8u,100u);memory.WriteDword(p+0x654u+16u,50u);
                memory.WriteDword(p+0x654u+0x220u+9u*4u,80u);
                Check(craft.Apply(row[0],selected,true)==0,"worn equipment crafting success");
                const auto sequence=inventory.RecentEquipmentSequences().back();
                Check(memory.ReadDword(p+0x920u+slot*4u)==sequence && sequence!=selected,"real player slot replaced before old instance removal");
                Check(memory.ReadDword(p+0x654u+0x220u+9u*4u)<=50u,"real native refresh clamps current MP");
                Check(!inventory.FindInventoryEntry(inventory.Address()+0x7cu,selected,7u),"worn material no longer exists in inventory");++equipped;
            }
            const auto selected=prepare(row);const auto before=inventory.RecentEquipmentSequences();
            Check(craft.Apply(row[0],0xffffffffu,false)==-1,"missing selected material fails original primitive");
            Check(inventory.RecentEquipmentSequences()==before && inventory.FindInventoryEntry(inventory.Address()+0x7cu,selected,7u),"failed craft creates no output and retains equipment");
            for(const auto [id,count]:materials(row))if(inventory.PropertyAddress(id))Check(inventory.ItemCount(id)==2u,"failure preserves original partial material consumption");
            Check(inventory.ReadDword(inventory.Address()+0x134u)==1000u,"failure does not charge money");++failed;
        }
    }
    bool missing{},bad_selected{},closed{};const auto before_sound=sounds;
    try{(void)craft.Apply(0xfefefefeu,0xffffffffu,true);}catch(const std::out_of_range&){missing=true;}
    try{(void)craft.Apply(first_key,0xfefefefeu,true);}catch(const std::out_of_range&){bad_selected=true;}
    Check(missing && bad_selected && sounds==before_sound,"invalid host requests rejected before sound or mutation");
    inventory.Close();try{(void)craft.Apply(first_key,0xffffffffu,true);}catch(const std::logic_error&){closed=true;}
    Check(closed && !inventory.OutstandingAllocations() && !memory.OutstandingAllocations(),"native lifetimes balanced");
    Check(cases==108u && equipped && failed,"all54 original foundry recipes in both entry modes");
    std::cout<<"PASS original foundry cases="<<cases<<" worn-replacements="<<equipped<<" partial-failures="<<failed<<" sound-calls="<<sounds<<"; actual material/output/money/player mutations\n";
    return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
