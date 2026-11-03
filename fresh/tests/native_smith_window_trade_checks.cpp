#include "support/native_smith_trade_checks.h"
#include "support/native_cegui_test_click.h"
#include "pal4/native_smith_window_layout.h"
#include "pal4/native_smith_window_lists.h"
#include "pal4/native_smith_window_confirmation.h"
#include "pal4/native_smith_window_craft_actions.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_inventory_prescriptions.h"
#include "pal4/native_inventory_equipment_mutation.h"
#include "pal4/native_prescription_record_catalog.h"
#include "pal4/native_game_database.h"
#include "pal4/native_crafting_requirements.h"
#include "pal4/native_common_dialog_layout.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/native_cegui_text_conversion.h"
#include "pal4/player_manager_object.h"
#include "pal4/game_text_format.h"
#include "../native/ui/cegui_exports_native.h"
#include <bit>
#include <optional>
namespace pal4::test {
namespace {
using detail::Export;void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}void* Ptr(unsigned a){return reinterpret_cast<void*>(a);}
}
void CheckNativeSmithTrade(NativeSmithTradeFixture f){
    const auto module=GetModuleHandleA("CEGUIBase.dll");const auto owner=f.layout.FrameAddress();
    const auto left=f.layout.Word(0x40u),right=f.layout.Word(0x44u);NativeInventoryPrescriptions learned(f.inventory,f.recipes);
    NativeCraftingRequirements requirements(f.recipes,f.inventory);
    auto text=[&](unsigned w){const auto s=Export<const void* (__thiscall*)(const void*)>(module,"?getText@Window@CEGUI@@QBEABVString@2@XZ")(Ptr(w));return std::string(Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ")(s));};
    auto hover=[&](unsigned list,unsigned row){Export<void (__thiscall*)(void*,int)>(module,"?setHoverRowIndex@MultiColumnList@CEGUI@@QAEXH@Z")(Ptr(list),static_cast<int>(row));};
    auto expect=[&](unsigned key,std::optional<std::string_view> name=std::nullopt){const auto source=f.db.FindTableTextById("GameString",key);Check(source.has_value(),"trade original GameString exists");
        const auto expected=name?FormatGameTextArgument(*source,*name):std::string(*source);
        Check(text(f.dialog.Window("Message"))==*ConvertGameTextToUtf8(expected),"exact original branch message and item name reaches DLL");};
    auto click=[&](const char* name){ClickNativeCeguiWindow(f.ui,f.dialog.Window(name));f.dialog.RethrowEventError();f.layout.RethrowEventError();};
    auto dismiss=[&](){click("BtOk");Check(f.activity.GetLast()==owner+0xe8u,"trade result restores original effect activity");(void)f.activity.Remove(owner+0xe8u);};
    auto reset=[&](unsigned mode,unsigned row,bool active){f.dialog.Close();f.confirmation.HideSelection();(void)f.activity.Remove(owner+0xe8u);(void)f.activity.Remove(owner+0xf0u);
        f.lists.Reset();f.inventory.Reset();f.players.Fill(0u);f.smith_active=active;for(const auto& r:f.recipes.Records())(void)learned.Add(r[0]);
        (void)f.inventory.UpdateCounter(1000);f.lists.Load(mode);hover(right,row);};
    auto fill=[&](unsigned record){for(unsigned n=0;n<4u;++n){const auto amount=f.inventory.ReadDword(record+0xa8u+n*4u),item=f.inventory.ReadDword(record+0x98u+n*4u);
        if(static_cast<int>(amount)>0){if(f.inventory.PropertyAddress(item))(void)f.inventory.AddItem(item,static_cast<int>(amount));else for(unsigned i=0;i<amount;++i)(void)f.inventory.ProcessEquipment(f.inventory.Address()+0x7cu,item);}}};
    auto target=[&](unsigned record,unsigned potential,unsigned copies=1u){unsigned selected{};for(const auto& r:f.db.Tables().at("Equip").rows){const auto id=std::get<unsigned>(r[0]),definition=f.inventory.EquipmentDefinitionAddress(id);
        if(definition && static_cast<int>(f.inventory.ReadDword(record+0x7cu+f.inventory.ReadDword(definition+0x94u)*4u))>0){
            for(unsigned i=0;i<copies;++i){const auto entry=f.inventory.ProcessEquipment(f.inventory.Address()+0x7cu,id);if(!selected)selected=entry;const auto sequence=f.inventory.ReadDword(entry+0x190u);
                f.mutation.WriteScalar(sequence,0x184u,potential);f.mutation.WriteScalar(sequence,0x180u,0u);f.memory.WriteDword(entry+0x17cu,f.inventory.ReadDword(record+0x70u));}break;}}
        Check(selected!=0u,"real trade eligible definition");return selected;};
    auto item_name=[&](unsigned entry){return std::string(reinterpret_cast<const char*>(f.inventory.ReadDword(entry+0x9cu)));};
    auto no_effect=[&](unsigned before,unsigned money){Check(f.mutation_sounds==before && f.inventory.ReadDword(f.inventory.Address()+0x134u)==money,"trade rejection/cancellation has no mutation or fee");};
    unsigned low_row{};while(low_row<static_cast<unsigned>(learned.Count(1)) && f.inventory.ReadDword(f.recipes.Address(learned.At(1,low_row))+0x6cu)<=1u)++low_row;
    Check(low_row<static_cast<unsigned>(learned.Count(1)),"original risky strengthening recipe");
    unsigned cases{};
    for(const auto active:{false,true})for(const auto low:{false,true}){
        reset(1u,low?low_row:0u,active);const auto record=f.recipes.Address(learned.At(1,low?low_row:0u));fill(record);
        const auto potential=low?1u:100u;const auto entry=target(record,potential,(!active && !low)?8u:1u);const auto name=item_name(entry);
        Check(f.confirmation.Confirm(),"original initial trade gates");const auto before=f.mutation_sounds;
        Check(f.actions.TradeSelected(),"strengthening original trade gate");expect(low?0x37u:(active?0x36u:0x5cu),(low || active)?std::optional<std::string_view>(name):std::nullopt);
        click("BtNo");no_effect(before,1000u);Check(f.inventory.ReadDword(entry+0x184u)==potential,"risk cancellation preserves actual potential");
        if(!active && !low){Check(f.ui.BindDialogRootParent(f.layout.RootAddress(),true),"trade list input fixture root");ClickNativeCeguiWindow(f.ui,left);f.layout.RethrowEventError();
            expect(0x5cu);}
        else Check(f.actions.TradeSelected(),"strengthening reconfirm");
        if(low && !active){click("BtYes");expect(0x5cu);no_effect(before,1000u);click("BtNo");no_effect(before,1000u);
            Check(f.actions.TradeSelected(),"risk warning restart after second cancellation");click("BtYes");expect(0x5cu);}
        click("BtYes");Check(f.mutation_sounds==before+1u && f.inventory.ReadDword(f.inventory.Address()+0x134u)==(active?1000u:920u),"real complete strengthening warning/fee/Yes chain");
        if(low)expect(0x38u);else expect(0x39u,name);dismiss();++cases;
    }
    // Trade rechecks a depleted potential and post-selection money, not just
    // the initial confirmation gate.
    reset(1u,0u,false);const auto smith_record=f.recipes.Address(learned.At(1,0));fill(smith_record);const auto exhausted=target(smith_record,0u);Check(f.confirmation.Confirm(),"exhausted target initial selection");
    auto before=f.mutation_sounds;Check(f.actions.TradeSelected(),"exhausted potential trade gate");expect(0xbeu);no_effect(before,1000u);f.dialog.Close();
    f.mutation.WriteScalar(f.inventory.ReadDword(exhausted+0x190u),0x184u,100u);(void)f.inventory.UpdateCounter(-921);
    Check(f.actions.TradeSelected(),"post-selection money gate");expect(0x2du);no_effect(before,79u);f.dialog.Close();
    // Six enchantment branches: new/same/different Skill, normal/System.
    unsigned enchant_row{};while(enchant_row<static_cast<unsigned>(learned.Count(2)) && !f.inventory.ReadDword(f.recipes.Address(learned.At(2,enchant_row))+0x74u))++enchant_row;
    Check(enchant_row<static_cast<unsigned>(learned.Count(2)),"original nonzero enchantment Skill");
    for(const auto active:{false,true})for(unsigned previous=0;previous<3u;++previous){reset(2u,enchant_row,active);const auto key=learned.At(2,enchant_row),record=f.recipes.Address(key),skill=f.inventory.ReadDword(record+0x74u);
        fill(record);const auto entry=target(record,100u),sequence=f.inventory.ReadDword(entry+0x190u);const auto name=item_name(entry);
        const auto old_skill=previous==0u?0u:previous==1u?skill:skill+1u;f.mutation.WriteScalar(sequence,0x180u,old_skill);Check(f.confirmation.Confirm(),"enchantment initial selection");before=f.mutation_sounds;
        Check(f.actions.TradeSelected(),"enchantment original trade gate");if(previous==1u){expect(0xccu,name);no_effect(before,1000u);f.dialog.Close();++cases;continue;}
        expect(previous==2u?0x3eu:(active?0x3cu:0x5eu),(active || previous==2u)?std::optional<std::string_view>(name):std::nullopt);
        click("BtNo");no_effect(before,1000u);Check(f.inventory.ReadDword(entry+0x180u)==old_skill,"enchantment cancel preserves old Skill");
        Check(f.actions.TradeSelected(),"enchantment reconfirm");
        if(previous==2u && !active){click("BtYes");expect(0x5eu);no_effect(before,1000u);click("BtNo");no_effect(before,1000u);
            Check(f.actions.TradeSelected(),"overwrite restart after second cancellation");click("BtYes");expect(0x5eu);}
        click("BtYes");expect(0x3fu,name);Check(f.inventory.ReadDword(entry+0x180u)==skill && f.mutation_sounds==before+1u &&
            f.inventory.ReadDword(f.inventory.Address()+0x134u)==(active?1000u:920u),"real complete enchantment overwrite/fee/Yes chain writes actual Skill");dismiss();++cases;
    }
    reset(2u,enchant_row,false);const auto record=f.recipes.Address(learned.At(2,enchant_row));fill(record);const auto short_entry=target(record,100u);Check(f.confirmation.Confirm(),"insufficient lingyun initial selection");
    const auto name=item_name(short_entry);f.memory.WriteDword(short_entry+0x17cu,f.inventory.ReadDword(record+0x70u)-1u);before=f.mutation_sounds;
    Check(f.actions.TradeSelected(),"lingyun recheck");expect(0x3du,name);no_effect(before,1000u);f.dialog.Close();
    f.memory.WriteDword(short_entry+0x17cu,f.inventory.ReadDword(record+0x70u));unsigned material{};
    for(unsigned i=0;i<4u;++i){const auto id=f.inventory.ReadDword(record+0x98u+i*4u);if(f.inventory.PropertyAddress(id) && f.inventory.ItemCount(id)){material=id;break;}}
    Check(material!=0u,"enchantment ordinary material fixture");(void)f.inventory.RemoveItem(material,static_cast<int>(f.inventory.ItemCount(material)));
    Check(f.actions.TradeSelected(),"materials rechecked after selection");Check(text(f.dialog.Window("Message")).find("%s")==std::string::npos && text(f.dialog.Window("Message")).find("%d")==std::string::npos,"real missing-material text through equipment trade");no_effect(before,1000u);f.dialog.Close();
    // Foundry with equipment material: real selection, confirmation and final
    // callback replace the selected instance through the existing mutation owner.
    unsigned gear_row{};while(gear_row<static_cast<unsigned>(learned.Count(0)) && !requirements.HasEquipmentMaterial(learned.At(0,gear_row)))++gear_row;
    Check(gear_row<static_cast<unsigned>(learned.Count(0)),"original equipment material foundry recipe");
    for(const auto active:{false,true}){reset(0u,gear_row,active);const auto key=learned.At(0,gear_row),recipe=f.recipes.Address(key);fill(recipe);Check(f.confirmation.Confirm(),"foundry equipment selection");
        const auto definition=static_cast<unsigned>(requirements.FirstEquipmentValue(key)),type=f.inventory.ReadDword(f.inventory.EquipmentDefinitionAddress(definition)+0x94u);
        const auto old=f.inventory.EquipmentAt(type,0u),sequence=f.inventory.ReadDword(old+0x190u);const auto product=requirements.Product(key);before=f.mutation_sounds;
        Check(f.actions.TradeSelected(),"foundry selected instance trade confirmation");expect(active?0x32u:0x5bu,active?std::optional<std::string_view>(reinterpret_cast<const char*>(f.inventory.ReadDword(f.inventory.EquipmentDefinitionAddress(product)+0x9cu))):std::nullopt);
        click("BtYes");Check(!f.inventory.FindInventoryEntry(f.inventory.Address()+0x7cu,sequence,7u) && f.mutation_sounds==before+1u && f.inventory.ReadDword(f.inventory.Address()+0x134u)==(active?1000u:920u),"complete selected-instance foundry removes old copy and applies original fee");dismiss();++cases;
    }
    Check(cases==12u,"all trade branch families exercised");
    f.confirmation.HideSelection();(void)f.activity.Remove(owner+0xe8u);(void)f.activity.Remove(owner+0xf0u);f.lists.Reset();f.inventory.Reset();f.players.Fill(0u);
    for(const auto& r:f.recipes.Records())(void)learned.Add(r[0]);f.lists.Load(0u);
    std::cout<<"Native Smith complete trade branches12: normal/System risk/fee/new/same/overwrite/foundry, real list click and DLL Yes/No/result/OK; late money/material/potential/lingyun gates verified\n";
}
}
