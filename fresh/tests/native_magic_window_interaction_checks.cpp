#include "support/native_magic_window_presentation_probe.h"
#include "support/native_cegui_frame_session.h"
#include "support/native_cegui_test_click.h"
#include "pal4/native_magic_window_interaction.h"
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
#include "pal4/native_pal_game_iv_texture_storage.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/native_cegui_widget_images.h"
#include "pal4/indexed_sorted_value_arrays.h"
#include "../native/ui/cegui_exports_native.h"
#include <cstring>
#include <iostream>
namespace pal4::test {
void VerifyNativeMagicWindowInteraction(NativeCeguiUiSession& ui,const std::filesystem::path& root,const NativeGameDatabase& db,NativeFrameSession& frame){
    using detail::Export;const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};const auto ptr=[](unsigned a){return reinterpret_cast<void*>(a);};
    NativeMagicWindowRecords records(db);NativeSkillRecordCatalog skills(db);NativeBaseObjectMemory memory;
    PlayerManagerObjectState players{};NativeRoleRecordCatalog roles(db);NativePlayerSlotCopyRuntime copier;
    NativePalGameIVTextureStorage game([](unsigned){throw std::logic_error("magic input has no texture dictionaries");});check(game.Construct()!=0u,"magic input game core");
    game.SetGameDataEntry(19u,1u);game.SetGameDataEntry(20u,1u);
    struct Collections final : IIndexedSortedValueArrayRuntime{NativeBaseObjectMemory& memory;explicit Collections(NativeBaseObjectMemory& m):memory(m){}
        unsigned ReadDword(unsigned a)override{return memory.ReadDword(a);}void WriteDword(unsigned a,unsigned v)override{memory.WriteDword(a,v);}unsigned Allocate(unsigned n)override{return memory.Allocate(n);}void Free(unsigned a)override{(void)memory.Free(a);}} collections(memory);
    const auto& stunt=db.Tables().at("Stunt").rows;check(stunt.size()>=3u,"magic input original stunt keys");
    const std::array<unsigned,3> ids{std::get<unsigned>(stunt[0][0]),std::get<unsigned>(stunt[1][0]),std::get<unsigned>(stunt[2][0])};
    for(unsigned role=0u;role<4u;++role){const auto p=reinterpret_cast<unsigned>(&players.slots[role]);(void)PlayerManagerSlotDataCopier::Copy(p,roles.Address(role),copier);
        memory.WriteDword(p+0x654u,PlayerManagerSlot::kScriptVtable);(void)NativePlayerScriptPropertyRuntime::Set(p+0x654u,0x44u,4u,99u);
        players.Word((role*0xb14u+0xb08u)/4u)=role==0u || role==3u?1u:0u;
        FixedSortedValueArrayState values;std::memcpy(&values,ptr(p+0x938u),sizeof(values));
        for(const auto id:ids)FixedSortedValueArray::InsertUnique(values,id,collections);std::memcpy(ptr(p+0x938u),&values,sizeof(values));}
    NativeMagicWindowInteraction* current{};const unsigned parent=0xabcdu;
    NativeEventHandlerArray active([&](unsigned a,unsigned slot){if(current && current->OwnsActivity(a))return current->Callback(a,slot);check(a==parent,"magic input parent callback");return 1u;},
        [&](unsigned a,unsigned slot,unsigned e)->std::uint8_t{if(current && current->OwnsActivity(a))return current->Predicate(a,slot,e);check(a==parent,"magic input parent predicate");return 0u;});
    std::array<unsigned,0x224u/4u> manager{};const auto manager_address=reinterpret_cast<unsigned>(manager.data());
    NativeMagicWindowLayout layout(ui,root,db,manager_address,[&](auto o,auto h,auto e){return current?current->FromDll(o,h,e):false;});
    NativeMagicWindowPresentation view(layout,players,db,records,skills);
    unsigned points{},stunt_selects{},uses{},tutorials{},toolbar_returns{};std::vector<unsigned> events;
    NativeMagicInteractionServices services;
    services.event=[&](unsigned key,unsigned a,unsigned){events.push_back(key);if(key==0x8a80acu && a==1u)++toolbar_returns;};
    services.tutorial=[&](unsigned id){check(id==4u,"magic original tutorial4");++tutorials;};
    services.magic_point=[&](unsigned button){check(records.FindMagic(reinterpret_cast<unsigned>(Export<void* (__thiscall*)(const void*)>(GetModuleHandleA("CEGUIBase.dll"),"?getUserData@Window@CEGUI@@QBEPAXXZ")(ptr(button))))!=nullptr,"magic action endpoint receives actual record key");++points;};
    services.stunt_select=[&](unsigned payload,unsigned record){check(record && memory.ReadDword(record)==ids[1] && memory.ReadDword(payload+16u)==0u,"stunt action endpoint receives exact selected native record and zero-guard click payload");++stunt_selects;};
    services.use_stunt=[&](unsigned key){check(key==ids[1],"use_stunt command passes scalar key rather than dereferencing it as event");++uses;};
    NativeMagicWindowInteraction interaction(ui,layout,view,players,records,game,active,manager_address,std::move(services));current=&interaction;
    const auto module=GetModuleHandleA("CEGUIBase.dll");const auto& panel=layout.Panel();const auto object=layout.FrameAddress();
    const auto key=[&](unsigned code){std::array<unsigned,5> data{};data[4]=code;const auto event=reinterpret_cast<unsigned>(data.data());const auto result=active.Contains(event)?1u:active.DispatchKeyDown(active.GetLast(),event);interaction.RethrowEventError();return result;};
    const auto selected=[&](unsigned w){return Export<bool (__thiscall*)(const void*)>(module,"?isSelected@ButtonBase@CEGUI@@QBE_NXZ")(ptr(w));};
    const auto visible=[&](unsigned w){return Export<bool (__thiscall*)(const void*,bool)>(module,"?isVisible@Window@CEGUI@@QBE_N_N@Z")(ptr(w),false);};
    const auto render=[&]{check(frame.Begin() && ui.Render() && !frame.ReadPixels().empty(),"magic interaction real GPU frame");};
    const auto portrait=[&]{return reinterpret_cast<unsigned>(Export<const void* (__thiscall*)(const void*)>(module,"?getImage@StaticImage@CEGUI@@QBEPBVImage@2@XZ")(ptr(panel.At(0x7cu))));};
    NativeCeguiWidgetImages images(module);(void)active.Insert(parent);const auto snapshot=players;const auto allocations=memory.OutstandingAllocations();
    for(unsigned cycle=0u;cycle<2u;++cycle){
        interaction.SetVisible(true);check(layout.State().category_position==0u && layout.State().selected_character_index==0 && active.Size()==3u && active.GetLast()==object+0x4cu,
            "magic original show chooses active role and Stunt row member over category and parent");
        check(portrait()==images.Image("zi0","yuntianhe") && visible(panel.At(0x80u)) && visible(panel.At(0x84u)),"magic actual character header and two-player arrows");render();
        ClickNativeCeguiWindow(ui,panel.At(0x84u));interaction.RethrowEventError();check(layout.State().selected_character_index==3 && portrait()==images.Image("zi0","murongziying"),"actual Magic right arrow skips inactive roles and refreshes portrait");
        ClickNativeCeguiWindow(ui,panel.At(0x80u));interaction.RethrowEventError();check(layout.State().selected_character_index==0,"actual Magic left arrow returns active role");
        for(unsigned category=0u;category<6u;++category){render();ClickNativeCeguiWindow(ui,panel.At(0xa8u+category*4u));interaction.RethrowEventError();
            check(layout.State().category_position==category && selected(panel.At(0xa8u+category*4u)) && active.GetLast()==object+(category?0x40u:0x4cu),"actual DLL category click refreshes data and active row member");}
        check(key(3u) && layout.State().category_position==1u && active.GetLast()==object+0x40u,"global shortcut3 selects Water category with corrected navigation-subobject offset");
        render();ClickNativeCeguiWindow(ui,panel.At(0xc8u));interaction.RethrowEventError();
        check(selected(panel.At(0xc8u)) && points==cycle*2u,"actual magic-name mouse hover selects details without triggering point action");
        check(key(3u) && layout.State().category_position==1u && selected(panel.At(0xc8u)),
            "original selected category shortcut is handled without re-clicking or resetting hovered row");
        check(key(4u) && layout.State().category_position==2u && selected(panel.At(0xc8u)),
            "category shortcut refreshes data then original DLL capture release restores detail under current virtual cursor");
        const auto fire_key=reinterpret_cast<unsigned>(Export<void* (__thiscall*)(const void*)>(module,"?getUserData@Window@CEGUI@@QBEPAXXZ")(ptr(panel.At(0xf8u))));
        check(records.FindMagic(fire_key) && records.FindMagic(fire_key)->category==1u,
            "hovered row after shortcut contains the new Fire category record");
        (void)Export<bool (__thiscall*)(void*,float,float)>(module,"?injectMousePosition@System@CEGUI@@QAE_NMM@Z")(ptr(ui.SystemAddress()),10.f,10.f);
        interaction.RethrowEventError();
        check(key(3u) && layout.State().category_position==1u && selected(panel.At(0xc0u)),
            "keyboard category refresh selects first row when virtual cursor is outside skill rows");
        check(selected(panel.At(0xc0u)) && key(208u) && selected(panel.At(0xc4u)) && key(200u) && selected(panel.At(0xc0u)),"Magic keyboard row down/up updates actual DLL selection");
        check(key(200u) && key(205u) && layout.State().selected_character_index==3 && key(208u) && selected(panel.At(0xc0u)),"Magic keyboard header highlight/right changes active role and returns to first row");
        render();ClickNativeCeguiWindow(ui,panel.At(0x80u));interaction.RethrowEventError();check(layout.State().selected_character_index==0,"restore first role through actual header arrow");
        check(key(28u) && points==cycle*2u+1u,"Magic keyboard Enter reaches required real action endpoint once");
        render();ClickNativeCeguiWindow(ui,panel.At(0xf0u));interaction.RethrowEventError();check(points==cycle*2u+2u,"Magic actual point-button mouse click reaches same required action endpoint once");
        check(key(1u) && active.GetLast()==object+0x28u && key(1u) && active.Size()==1u && active.GetLast()==parent && toolbar_returns==cycle+1u,
            "Magic two Esc levels return category then System toolbar and preserve parent activity");
        interaction.SetVisible(true);check(key(208u) && selected(panel.At(0x10cu)) && key(28u) && stunt_selects==cycle*2u+1u,"Stunt keyboard down/Enter delivers selected native record through original synthetic click");
        render();ClickNativeCeguiWindow(ui,panel.At(0x10cu));interaction.RethrowEventError();check(stunt_selects==cycle*2u+2u,"Stunt actual mouse click and hover reach selected record action endpoint once");
        layout.DispatchCommand("use_stunt",ids[1]);check(uses==cycle+1u,"Magic owned use_stunt command routes scalar argument");
        interaction.SetVisible(false);check(active.Size()==1u && active.GetLast()==parent,"external page hide truncates all page-owned activities");
        check(!std::memcmp(&snapshot,&players,sizeof(players)) && memory.OutstandingAllocations()==allocations,"read-only Magic navigation and action boundary recording preserve entire party and allocations");
    }
    players.Word((3u*0xb14u+0xb08u)/4u)=0u;game.SetGameDataEntry(20u,0u);interaction.SetVisible(true);
    check(!visible(panel.At(0x80u)) && !visible(panel.At(0x84u)),"single active Magic role hides both header arrows");
    for(unsigned i=1u;i<6u;++i)check(Export<bool (__thiscall*)(const void*,bool)>(module,"?isDisabled@Window@CEGUI@@QBE_N_N@Z")(ptr(panel.At(0xa8u+i*4u)),false),"original GameData20 gate disables all five magic categories");
    interaction.Close();interaction.Close();check(active.Size()==1u && !interaction.OwnsActivity(object+0x28u),"Magic interaction Close retires borrowed routing and preserves parent");
    current=nullptr;active.ReleaseStorage();layout.Close();check(tutorials==5u,"Magic help lifecycle runs on each true show");
    std::cout<<"PASS: Magic actual category/party/portrait input, corrected global shortcuts, keyboard magic/stunt rows/header/Enter/two Esc, required action/command boundaries, gates and repeated cleanup\n";
}
}
