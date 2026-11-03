#pragma once
#include "native_cegui_test_click.h"
#include "pal4/native_crafting_requirements.h"
#include "pal4/native_inventory_prescriptions.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_prescription_record_catalog.h"
#include "pal4/native_smith_window.h"
#include "pal4/native_common_dialog_layout.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/native_dialog_settings_snapshot.h"
#include <map>
#include <iostream>
namespace pal4::test {
// Inventory reward/craft observers append real discovered item IDs. Restore
// only the fixture's discovery changes after acceptance, before later save tests.
class NativeSmithDiscoveryCheckpoint {
public:
    explicit NativeSmithDiscoveryCheckpoint(NativeDialogSettingsSnapshot& settings):settings_(settings){
        const auto& state=settings.State();const auto end=state.Word(148u+4u);
        for(auto node=settings.FirstNode(state,148u);node!=end;node=settings.NextNode(node,false))values_.push_back(settings.NodeValue(node));
    }
    ~NativeSmithDiscoveryCheckpoint(){settings_.ClearTree(settings_.State(),148u);for(auto id:values_)settings_.InsertTreeValue(settings_.State(),148u,id);}
private:
    NativeDialogSettingsSnapshot& settings_;std::vector<unsigned> values_;
};
// Supplies the live application's ordinary crafting acceptance. It adds test
// recipe/material rewards without resetting equipped instances or story state.
class NativeApplicationSmithCraftProbe {
public:
    NativeApplicationSmithCraftProbe(NativeInventorySession& inventory,const NativePrescriptionRecordCatalog& recipes)
        :inventory_(inventory){
        NativeCraftingRequirements requirements(recipes,inventory);
        for(const auto& row:recipes.Records()){
            const auto record=recipes.Address(row[0]);const auto product=requirements.Product(row[0]);
            if(inventory.ReadDword(record+100u) || requirements.HasEquipmentMaterial(row[0]) || !inventory.HasEquipment(product))continue;
            std::map<unsigned,unsigned> materials;bool valid=true;
            for(unsigned i=0u;i<4u;++i){const auto id=inventory.ReadDword(record+0x98u+i*4u),amount=inventory.ReadDword(record+0xa8u+i*4u);
                if(static_cast<int>(amount)<=0)continue;
                if(id==product || !inventory.PropertyAddress(id)){valid=false;break;}materials[id]+=amount;}
            if(!valid || materials.empty())continue;
            recipe_=row[0];product_=product;materials_=std::move(materials);break;
        }
        Require(recipe_ && product_,"application craft chooses original plain recipe with property materials");
        NativeInventoryPrescriptions learned(inventory,recipes);(void)learned.Add(recipe_);
        for(const auto& [id,count]:materials_)(void)inventory.AddItem(id,static_cast<int>(count*2u+2u));
        (void)inventory.UpdateCounter(1000);
        type_=inventory.ReadDword(inventory.EquipmentDefinitionAddress(product_)+0x94u);
    }
    template<class Enter,class Render>
    void Verify(NativeCeguiUiSession& ui,NativeSmithWindow& page,NativeCommonDialogLayout& dialog,
                NativeEventHandlerArray& activity,Enter enter,Render render){
        using detail::Export;const auto dll=GetModuleHandleA("CEGUIBase.dll");
        const auto list=page.FrameWord(0x44u);void* window=reinterpret_cast<void*>(list);
        const auto rows=Export<unsigned (__thiscall*)(const void*)>(dll,"?getRowCount@MultiColumnList@CEGUI@@QBEIXZ")(window);
        unsigned row=rows;
        for(unsigned i=0;i<rows;++i){const std::array<unsigned,2> grid{i,0u};
            const auto cell=Export<void* (__thiscall*)(const void*,const void*)>(dll,"?getItemAtGridReference@MultiColumnList@CEGUI@@QBEPAVListboxItem@2@ABUMCLGridRef@2@@Z")(window,grid.data());
            if(cell && Export<unsigned (__thiscall*)(const void*)>(dll,"?getID@ListboxItem@CEGUI@@QBEIXZ")(cell)==recipe_){row=i;break;}}
        Require(row<rows,"application Smith list includes actual learned probe recipe");
        Export<void (__thiscall*)(void*,int)>(dll,"?setHoverRowIndex@MultiColumnList@CEGUI@@QAEXH@Z")(window,static_cast<int>(row));page.RethrowEventError();
        const auto before=Snapshot();const auto products=Products();const auto money=Money();
        enter();dialog.RethrowEventError();page.RethrowEventError();render();
        ClickNativeCeguiWindow(ui,dialog.Window("BtNo"));dialog.RethrowEventError();page.RethrowEventError();
        Require(Snapshot()==before && Products()==products && Money()==money,"actual application crafting No preserves materials/product/money");
        enter();dialog.RethrowEventError();page.RethrowEventError();
        ClickNativeCeguiWindow(ui,dialog.Window("BtYes"));dialog.RethrowEventError();page.RethrowEventError();
        Require(Products()==products+1u && Money()==money-80u,"actual application Foundry Yes produces one item and charges original80 fee");
        for(const auto& [id,count]:materials_)Require(inventory_.ItemCount(id)==before.at(id)-count,"actual application craft consumes exact original recipe materials");
        render();ClickNativeCeguiWindow(ui,dialog.Window("BtOk"));dialog.RethrowEventError();page.RethrowEventError();
        Require(activity.GetLast()==page.FrameAddress()+0xe8u,"actual application result OK restores recipe navigation");
        std::cout<<"PASS: application Foundry real No/Yes/result, recipe="<<recipe_<<" product="<<product_<<" money="<<money<<"->"<<Money()<<"; exact material deltas\n";
    }
private:
    static void Require(bool value,const char* why){if(!value)throw std::runtime_error(why);}
    unsigned Money(){return inventory_.ReadDword(inventory_.Address()+0x134u);}
    unsigned Products(){unsigned count{};for(unsigned i=0;i<inventory_.EquipmentCount(type_);++i)count+=inventory_.ReadDword(inventory_.EquipmentAt(type_,i)+0x90u)==product_;return count;}
    std::map<unsigned,unsigned> Snapshot(){std::map<unsigned,unsigned> result;for(const auto& [id,count]:materials_)result[id]=inventory_.ItemCount(id);return result;}
    NativeInventorySession& inventory_;unsigned recipe_{},product_{},type_{};std::map<unsigned,unsigned> materials_;
};
}
