#include "support/native_magic_window_presentation_probe.h"
#include "support/native_cegui_frame_session.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/native_magic_window_layout.h"
#include "pal4/native_magic_window_records.h"
#include "pal4/native_magic_window_presentation.h"
#include "pal4/native_game_database.h"
#include "pal4/native_skill_record_catalog.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_role_record_image.h"
#include "pal4/native_player_slot_copy_runtime.h"
#include "pal4/player_manager_slot_data_copier.h"
#include "pal4/native_player_script_property_runtime.h"
#include "pal4/player_manager_object.h"
#include "pal4/player_manager_slot.h"
#include "pal4/native_cegui_widget_images.h"
#include "pal4/native_cegui_text_conversion.h"
#include "pal4/indexed_key_value_collection.h"
#include "pal4/indexed_sorted_value_arrays.h"
#include "../native/ui/cegui_exports_native.h"
#include <algorithm>
#include <bit>
#include <cstring>
#include <iostream>
namespace pal4::test {
void VerifyNativeMagicWindowPresentation(NativeCeguiUiSession& ui,const std::filesystem::path& root,const NativeGameDatabase& db,NativeFrameSession& frame){
    using detail::Export;const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto ptr=[](unsigned a){return reinterpret_cast<void*>(a);};
    const auto utf8=[](std::string_view text){return ConvertGameTextToUtf8(text).value();};
    const auto column=[&](const NativeGameDbTable& table,std::string_view name){const auto it=std::find_if(table.fields.begin(),table.fields.end(),[&](const auto& c){return c.name==name;});check(it!=table.fields.end(),"Magic/Stunt source column");return static_cast<std::size_t>(it-table.fields.begin());};
    NativeMagicWindowRecords records(db);NativeSkillRecordCatalog skills(db);
    const auto& magic_table=db.Tables().at("Magic");
    check(records.Magic().size()==magic_table.rows.size() && !records.FindMagic(0xffffffffu) && !records.FindStunt(0xffffffffu),"Magic/Stunt original record count and absent ID");
    for(const auto& source:magic_table.rows){const auto id=std::get<unsigned>(source[column(magic_table,"ID")]);const auto* r=records.FindMagic(id);
        check(r && r->category==std::get<unsigned>(source[column(magic_table,"FELEMENTS")]) && r->effect_id==std::get<unsigned>(source[column(magic_table,"ATTACHED_SKILL")]) &&
            r->threshold_3c==std::get<unsigned>(source[column(magic_table,"NEED_POINT")]) && r->minimum_total_40==std::get<unsigned>(source[column(magic_table,"NEED_TOTAL_USED_POINT")]) &&
            r->cast_cost==std::bit_cast<int>(std::get<unsigned>(source[column(magic_table,"CONSUMED_MP")])) &&
            records.Text(r->name_address)==std::get<std::string>(source[column(magic_table,"NAME")]),"all original Magic IDs/categories/Skill links and owned names");
        const auto detail=records.MagicDetails(id);check(detail && detail->title==std::get<std::string>(source[column(magic_table,"DESCRIPTION")]) &&
            detail->description==std::get<std::string>(source[column(magic_table,"PROP")]),"all Magic details preserve original DESCRIPTION/PROP strings");}
    const auto& stunt_table=db.Tables().at("Stunt");std::vector<unsigned> stunt_keys;
    for(const auto& source:stunt_table.rows){const auto id=std::get<unsigned>(source[column(stunt_table,"ID")]);const auto r=records.FindStunt(id);
        check(r && records.Text(r->name_address)==std::get<std::string>(source[column(stunt_table,"NAME")]) && r->effect_id==std::get<unsigned>(source[column(stunt_table,"ATTACHED_SKILL")]) &&
            r->required_stat==std::bit_cast<int>(std::get<unsigned>(source[column(stunt_table,"CONSUMED_RAGE")])) && r->counter_cost==std::bit_cast<int>(std::get<unsigned>(source[column(stunt_table,"CONSUMED_MONEY")])),"all original Stunt IDs/Skill links/costs and names");
        const auto detail=records.StuntDetails(id);check(detail && detail->title==std::get<std::string>(source[column(stunt_table,"DESCRIPTION")]) &&
            detail->description==std::get<std::string>(source[column(stunt_table,"PROP")]),"all Stunt details preserve original DESCRIPTION/PROP strings");stunt_keys.push_back(id);}
    bool foreign{};try{(void)records.Text(0xdeadbeefu);}catch(const std::invalid_argument&){foreign=true;}check(foreign,"Magic/Stunt foreign text rejects before dereference");
    NativeBaseObjectMemory memory;
    struct Collections final : IIndexedKeyValueCollectionRuntime,IIndexedSortedValueArrayRuntime {
        NativeBaseObjectMemory& memory;explicit Collections(NativeBaseObjectMemory& m):memory(m){}
        unsigned ReadDword(unsigned a)override{return memory.ReadDword(a);}void WriteDword(unsigned a,unsigned v)override{memory.WriteDword(a,v);}
        unsigned Allocate(unsigned n)override{return memory.Allocate(n);}void Free(unsigned a)override{(void)memory.Free(a);}
    } collections(memory);
    PlayerManagerObjectState players{};NativeRoleRecordCatalog roles(db);NativePlayerSlotCopyRuntime copier;
    std::array<unsigned,4> player_addresses{};
    for(unsigned i=0u;i<4u;++i){const auto p=player_addresses[i]=reinterpret_cast<unsigned>(&players.slots[i]);(void)PlayerManagerSlotDataCopier::Copy(p,roles.Address(i),copier);
        memory.WriteDword(p+0x654u,PlayerManagerSlot::kScriptVtable);(void)NativePlayerScriptPropertyRuntime::Set(p+0x654u,0x44u,4u,1u);}
    const auto module=GetModuleHandleA("CEGUIBase.dll");NativeCeguiWidgetImages images(module);
    const auto text=[&](unsigned w){const auto s=Export<const void* (__thiscall*)(const void*)>(module,"?getText@Window@CEGUI@@QBEABVString@2@XZ")(ptr(w));return std::string(Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ")(s));};
    const auto visible=[&](unsigned w){return Export<bool (__thiscall*)(const void*,bool)>(module,"?isVisible@Window@CEGUI@@QBE_N_N@Z")(ptr(w),false);};
    const auto disabled=[&](unsigned w){return Export<bool (__thiscall*)(const void*,bool)>(module,"?isDisabled@Window@CEGUI@@QBE_N_N@Z")(ptr(w),false);};
    const auto key=[&](unsigned w){return reinterpret_cast<unsigned>(Export<void* (__thiscall*)(const void*)>(module,"?getUserData@Window@CEGUI@@QBEPAXXZ")(ptr(w)));};
    unsigned locked{},learnable{},learned_use{},learned_disabled{};
    for(unsigned cycle=0u;cycle<2u;++cycle){
        NativeMagicWindowLayout layout(ui,root,db,0x1000u,[](auto,auto,auto){return true;});
        NativeMagicWindowPresentation view(layout,players,db,records,skills);const auto& panel=layout.Panel();
        check(ui.BindDialogRootParent(layout.RootAddress(),true),"magic presentation binds actual original page");
        for(unsigned role=0u;role<4u;++role){const auto player=player_addresses[role];layout.State().selected_character_index=role;
            (void)NativePlayerScriptPropertyRuntime::Set(player+0x654u,0x44u,4u,cycle?99u:1u);
            const auto snapshot=players;
            const auto base=(cycle?99:1)+(memory.ReadDword(player+0xa48u)==2u?9:4);
            for(unsigned category=0u;category<5u;++category){layout.State().category_position=category+1u;
                const auto selected=MagicWindowSkillCatalog::Select(records.Magic(),category);
                check(view.UpdateMagic()==selected.VisibleCount() && !std::memcmp(&snapshot,&players,sizeof(players)),"magic refresh reads live party without mutation");
                if(text(panel.At(0x90u))!=std::to_string(base) || text(panel.At(0x94u))!=std::to_string(base) || text(panel.At(0x98u))!="0")
                    std::cout<<"Magic counters role="<<role<<" cycle="<<cycle<<" category="<<category<<" role_id="<<memory.ReadDword(player+0xa48u)<<" expected_base="<<base
                        <<" actual="<<text(panel.At(0x90u))<<','<<text(panel.At(0x94u))<<','<<text(panel.At(0x98u))<<" level="<<memory.ReadDword(player+0x654u+140u*4u)<<std::endl;
                check(text(panel.At(0x90u))==std::to_string(base) && text(panel.At(0x94u))==std::to_string(base) && text(panel.At(0x98u))=="0","actual Magic base/remaining/category counters match original role/level query");
                for(unsigned row=0u;row<selected.VisibleCount();++row){const auto& r=records.Magic()[selected.matching_indices[row]];const auto button=panel.At(0xf0u+row*4u);
                    check(text(panel.At(0xc0u+row*4u))==utf8(records.Text(r.name_address)) && key(button)==r.key && visible(button),"actual magic row name/key and visibility");
                    check(images.ButtonImage(button,"Self",MinimapButtonImageState::Normal)==images.Image("duihuakuang0","bottonskilllearn"),"unlearned Magic actual original learn-button image");
                    check(disabled(button)==!(base>=std::bit_cast<int>(r.threshold_3c) && std::bit_cast<int>(r.minimum_total_40)<=0),"actual learn enable checks remaining points and total prerequisites independently");
                    if(disabled(button))++locked;else ++learnable;
                    view.MagicDetails(row);const auto detail=records.MagicDetails(r.key).value();check(text(panel.At(0x88u))==utf8(detail.title) && text(panel.At(0x8cu))==utf8(detail.description),"all visible magic details reach actual DLL controls");
                }
                check(frame.Begin() && ui.Render() && !frame.ReadPixels().empty(),"original skill data draws through actual DLL GPU path");
            }
        }
    }
    check(locked && learnable,"original Magic data exercises both locked and affordable actions");
    {
        NativeMagicWindowLayout layout(ui,root,db,0x1000u,[](auto,auto,auto){return true;});NativeMagicWindowPresentation view(layout,players,db,records,skills);
        const auto& panel=layout.Panel();layout.State().selected_character_index=0;const auto p=player_addresses[0];
        IndexedKeyValueCollectionState collection;std::memcpy(&collection,ptr(p+0x938u),sizeof(collection));unsigned used{};
        for(const auto& r:records.Magic()){if(r.category<5u){IndexedKeyValueCollection::AddDelta(collection,r.category,r.key,r.threshold_3c,collections);used+=r.threshold_3c;}}
        std::memcpy(ptr(p+0x938u),&collection,sizeof(collection));const auto snapshot=players;const auto allocations=memory.OutstandingAllocations();
        for(unsigned category=0u;category<5u;++category){layout.State().category_position=category+1u;const auto selected=MagicWindowSkillCatalog::Select(records.Magic(),category);(void)view.UpdateMagic();unsigned total{};
            for(const auto& r:records.Magic())if(r.category==category)total+=r.threshold_3c;
            check(text(panel.At(0x94u))==std::to_string(std::bit_cast<int>(103u-used)) && text(panel.At(0x98u))==std::to_string(total),"learned Magic counters read actual stored points including signed remaining value");
            for(unsigned row=0u;row<selected.VisibleCount();++row){const auto& r=records.Magic()[selected.matching_indices[row]];const auto button=panel.At(0xf0u+row*4u);
                const auto skill=skills.Address(r.effect_id);const bool usable=skill && memory.ReadDword(skill+4u)==3u;
                check(disabled(button)!=usable && images.ButtonImage(button,"Self",MinimapButtonImageState::Normal)==images.Image("duihuakuang0","bottonskill"),"learned Magic uses actual Skill type to enable cast and swaps button images");
                if(usable)++learned_use;else ++learned_disabled;}
        }
        check(!std::memcmp(&snapshot,&players,sizeof(players)) && memory.OutstandingAllocations()==allocations,"learned refresh leaves party and allocation ownership unchanged");
        // Stunt is the existing fixed sorted array at the same collection's final
        // slot; include one missing key so compacting rows is exercised.
        FixedSortedValueArrayState stunts;std::memcpy(&stunts,ptr(p+0x938u),sizeof(stunts));
        FixedSortedValueArray::InsertUnique(stunts,-1,collections);
        for(unsigned i=0u;i<std::min<std::size_t>(stunt_keys.size(),10u);++i)FixedSortedValueArray::InsertUnique(stunts,stunt_keys[i],collections);
        std::memcpy(ptr(p+0x938u),&stunts,sizeof(stunts));const auto stunt_snapshot=players;
        const auto count=view.UpdateStunts();check(count==7u,"real Stunt collection skips missing ID and limits display to seven");
        for(unsigned row=0u;row<count;++row){const auto button=panel.At(0x108u+row*4u);const auto record=records.FindStunt(key(button));
            check(record && visible(button) && text(button)==utf8(records.Text(record->name_address)),"actual Stunt names/keys from compacted owned records");
            view.StuntDetails(row);const auto detail=records.StuntDetails(record->key).value();check(text(panel.At(0x88u))==utf8(detail.title) && text(panel.At(0x8cu))==utf8(detail.description),"all seven Stunt details reach actual DLL controls");}
        check(!visible(panel.At(0x90u)) && !visible(panel.At(0x15cu)) && visible(panel.At(0x164u)) && !std::memcmp(&stunt_snapshot,&players,sizeof(players)),"Stunt view hides point counters, changes panel and preserves live players");
        layout.State().selected_character_index=4;check(!view.UpdateMagic() && text(panel.At(0x88u)).empty() && text(panel.At(0x8cu)).empty(),"missing player returns and clears details without dereference");
    }
    check(learned_use && learned_disabled,"original linked Skills exercise both learned-use and learned-disabled branches");
    std::cout<<"PASS: original Magic="<<records.Magic().size()<<" Stunt="<<stunt_keys.size()<<" records, four roles/five categories, locked="<<locked<<" affordable="<<learnable
        <<" learned-use="<<learned_use<<" learned-disabled="<<learned_disabled<<", actual DLL rows/images/text/counters/details, Stunt collection, GPU and read-only party lifetime\n";
}
}
