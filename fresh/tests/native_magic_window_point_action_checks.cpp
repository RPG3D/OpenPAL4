#include "support/native_magic_window_presentation_probe.h"
#include "support/native_cegui_frame_session.h"
#include "support/native_cegui_test_click.h"
#include "pal4/native_magic_window_point_action.h"
#include "pal4/native_magic_window_layout.h"
#include "pal4/native_magic_window_presentation.h"
#include "pal4/native_magic_window_records.h"
#include "pal4/native_game_database.h"
#include "pal4/native_skill_record_catalog.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_role_record_image.h"
#include "pal4/native_player_slot_copy_runtime.h"
#include "pal4/player_manager_slot_data_copier.h"
#include "pal4/player_manager_slot.h"
#include "pal4/native_player_script_property_runtime.h"
#include "pal4/player_manager_object.h"
#include "pal4/indexed_key_value_collection.h"
#include "pal4/indexed_sorted_value_arrays.h"
#include "../native/ui/cegui_exports_native.h"
#include <cstring>
#include <iostream>
namespace pal4::test {
void VerifyNativeMagicWindowPointAction(NativeCeguiUiSession& ui,const std::filesystem::path& root,const NativeGameDatabase& db,NativeFrameSession& frame){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};const auto ptr=[](unsigned a){return reinterpret_cast<void*>(a);};
    NativeMagicWindowRecords records(db);NativeSkillRecordCatalog skills(db);NativeBaseObjectMemory memory;
    PlayerManagerObjectState players{};NativeRoleRecordCatalog roles(db);NativePlayerSlotCopyRuntime copier;
    for(unsigned role=0;role<4;++role){const auto p=reinterpret_cast<unsigned>(&players.slots[role]);
        (void)PlayerManagerSlotDataCopier::Copy(p,roles.Address(role),copier);memory.WriteDword(p+0x654u,PlayerManagerSlot::kScriptVtable);
        (void)NativePlayerScriptPropertyRuntime::Set(p+0x654u,0x44u,4u,99u);memory.WriteDword(p+0xb08u,1u);}
    struct Collections final : IIndexedKeyValueCollectionRuntime, IIndexedSortedValueArrayRuntime {
        NativeBaseObjectMemory& m;explicit Collections(NativeBaseObjectMemory& memory):m(memory){}
        unsigned ReadDword(unsigned a)override{return m.ReadDword(a);}void WriteDword(unsigned a,unsigned v)override{m.WriteDword(a,v);}
        unsigned Allocate(unsigned n)override{return m.Allocate(n);}void Free(unsigned a)override{(void)m.Free(a);}
    } collections(memory);
    NativeMagicWindowPointAction* current{};unsigned hints{},sounds{},casts{};bool fail_cast{};
    NativeMagicWindowLayout layout(ui,root,db,0x1000u,[&](unsigned,unsigned handler,unsigned event){
        if(handler==0x459780u){check(current!=nullptr,"point callback owner");return current->Click(memory.ReadDword(event+8u));}return true;});
    NativeMagicWindowPresentation view(layout,players,db,records,skills);const MagicDbSkillRecord* expected{};
    NativeMagicPointServices services;
    services.hint=[&](std::string_view name,unsigned trigger,float x,float y){check(expected && name==records.Text(expected->name_address) && trigger==9u && x==0.5f && y==0.4f,"learned hint uses original record name and trigger9 position");++hints;};
    services.sound=[&](std::string_view sound){check(sound=="gamedata\\palSound\\wa08.wav","learning wa08 sound request");++sounds;};
    services.select_magic=[&](unsigned key){check(expected && key==expected->key,"learned click forwards exact magic key to cast controller");++casts;if(fail_cast)throw std::runtime_error("cast controller sentinel");};
    NativeMagicWindowPointAction action(layout,view,players,memory,records,std::move(services));current=&action;
    check(ui.BindDialogRootParent(layout.RootAddress(),true),"point original page visible");
    const auto module=GetModuleHandleA("CEGUIBase.dll");const auto& panel=layout.Panel();
    const auto text=[&](unsigned w){const auto s=detail::Export<const void* (__thiscall*)(const void*)>(module,"?getText@Window@CEGUI@@QBEABVString@2@XZ")(ptr(w));return std::string(detail::Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ")(s));};
    unsigned learned{};
    for(unsigned role=0;role<4;++role){const auto p=reinterpret_cast<unsigned>(&players.slots[role]);
        for(unsigned category=0;category<5;++category){layout.State().category_position=category+1u;view.UpdateCharacter(role);
            const auto selected=MagicWindowSkillCatalog::Select(records.Magic(),category);check(selected.VisibleCount()>0u,"original category rows");expected=&records.Magic()[selected.matching_indices[0]];
            check(expected->threshold_3c>0u && expected->threshold_3c<20u,"first original skill affordable with real level99 points");
            const auto before=players;const auto old_stat=memory.ReadDword(p+0x658u+0x7cu+4u*category);
            const auto old_base=memory.ReadDword(p+0x654u+(116u+category)*4u);
            const auto old_remaining=std::stoi(text(panel.At(0x94u)));
            check(frame.Begin() && ui.Render() && !frame.ReadPixels().empty(),"point original GPU frame");
            ClickNativeCeguiWindow(ui,panel.At(0xf0u));layout.RethrowEventError();++learned;
            check(hints==learned && sounds==learned && casts==learned-1u,"real mouse learns exactly once before cast route");
            IndexedKeyValueCollectionState kv;std::memcpy(&kv,ptr(p+0x938u),sizeof(kv));
            IndexedSortedValueArrayState sorted;std::memcpy(&sorted,ptr(p+0x938u),sizeof(sorted));
            check(IndexedKeyValueCollection::GetValue(kv,category,expected->key,collections)==expected->threshold_3c &&
                IndexedSortedValueArrays::Contains(sorted,category,expected->key,collections),"actual party owns learned key and exact spent points");
            check(memory.ReadDword(p+0x658u+0x7cu+4u*category)==old_stat+expected->threshold_3c &&
                memory.ReadDword(p+0x654u+(116u+category)*4u)==old_base+expected->threshold_3c,"both original five-element stat groups updated");
            check(std::stoi(text(panel.At(0x94u)))==old_remaining-static_cast<int>(expected->threshold_3c) &&
                text(panel.At(0x98u))==std::to_string(expected->threshold_3c),"display refreshes actual remaining and category point total");
            for(unsigned other=0;other<4;++other)if(other!=role)check(!std::memcmp(&before.slots[other],&players.slots[other],sizeof(players.slots[other])),"learning preserves other party slots");
            const auto after=players;const auto allocations=memory.OutstandingAllocations();
            check(action.Click(panel.At(0xf0u)) && casts==learned,"repeat learned click enters cast controller");
            check(!std::memcmp(&after,&players,sizeof(players)) && allocations==memory.OutstandingAllocations() && hints==learned && sounds==learned,"repeat click neither spends nor relearns");
        }
    }
    // A failed cast owner must not strand the descriptor allocated by the shared handler.
    fail_cast=true;bool caught{};try{(void)action.Click(panel.At(0xf0u));}catch(const std::runtime_error& e){caught=std::string_view(e.what())=="cast controller sentinel";}
    check(caught,"cast controller error propagated");fail_cast=false;check(action.Click(panel.At(0xf0u)),"point owner reusable after cast failure");
    // Use an unlearned expensive row with level0 to exercise the original signed budget gate.
    const auto p=reinterpret_cast<unsigned>(&players.slots[3]);(void)NativePlayerScriptPropertyRuntime::Set(p+0x654u,0x44u,4u,0u);
    layout.State().category_position=1u;view.UpdateCharacter(3);expected=&records.Magic()[MagicWindowSkillCatalog::Select(records.Magic(),0u).matching_indices[1]];
    const auto before=players;const auto allocated=memory.OutstandingAllocations();const auto counts=std::array{hints,sounds,casts};
    check(action.Click(panel.At(0xf4u)) && !std::memcmp(&before,&players,sizeof(players)) && allocated==memory.OutstandingAllocations() && counts==std::array{hints,sounds,casts},"insufficient points leave player collections and services untouched");
    // Adjacent current-stat delta dispatch retains existing MP clamp and health notification.
    (void)NativePlayerScriptPropertyRuntime::Set(p+0x654u,0x40u,2u,30u);
    (void)NativePlayerScriptPropertyRuntime::Set(p+0x654u,0x44u,9u,20u);
    check(NativePlayerScriptPropertyRuntime::Set(p+0x654u,0x54u,9u,0u-7u)==13u && NativePlayerScriptPropertyRuntime::Set(p+0x654u,0x54u,9u,100u)==30u &&
        NativePlayerScriptPropertyRuntime::Set(p+0x654u,0x54u,9u,0u-100u)==0u,"shared stat delta dispatch preserves signed MP clamps");
    layout.Close();
    for(unsigned role=0;role<4;++role){const auto q=reinterpret_cast<unsigned>(&players.slots[role]);
        for(unsigned category=0;category<5;++category)for(const auto offset:{0u,0x50u}){
            const auto descriptor=q+0x938u+offset+category*16u;const auto buffer=memory.ReadDword(descriptor+4u);
            if(buffer)(void)memory.Free(buffer);for(unsigned field=4u;field<=12u;field+=4u)memory.WriteDword(descriptor+field,0u);}}
    check(memory.OutstandingAllocations()==0u,"persistent learned collections release through their owning allocator");
    std::cout<<"Native Magic point actions: "<<learned<<" real DLL learns, exact spend/stats/unique keys, repeats, failure, insufficient budget and MP clamps pass\n";
}
}
