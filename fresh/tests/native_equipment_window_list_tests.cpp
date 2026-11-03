#include "pal4/native_equipment_window_list.h"
#include "pal4/native_equipment_window_layout.h"
#include "pal4/native_equipment_slot_navigation.h"
#include "pal4/native_equipment_window_interaction.h"
#include "pal4/native_equipment_window_presentation.h"
#include "pal4/equipment_level_formatter.h"
#include "pal4/native_equipment_character_display.h"
#include "pal4/native_equipment_item_details.h"
#include "pal4/native_equipment_window_actions.h"
#include "pal4/native_equipment_window_selection.h"
#include "pal4/native_menu_window.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/native_player_script_refresh_runtime.h"
#include "pal4/native_player_script_property_runtime.h"
#include "pal4/native_role_record_image.h"
#include "pal4/native_player_slot_copy_runtime.h"
#include "pal4/player_manager_slot_data_copier.h"
#include "pal4/equipment_item_details.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_game_database.h"
#include "pal4/native_player_hud_stats_data.h"
#include "pal4/player_manager_slot.h"
#include "pal4/native_cegui_widget_images.h"
#include "pal4/native_cegui_text_conversion.h"
#include "support/native_cegui_frame_session.h"
#include "support/native_desktop_observation.h"
#include "support/native_property_window_layout.h"
#include "support/native_cegui_test_click.h"
#include "../native/ui/cegui_exports_native.h"
#include <rtfsyst.h>
#include <algorithm>
#include <iostream>
#include <set>
#include <cstring>
#include <cmath>
namespace {
using pal4::detail::Export;
void Check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void* Ptr(std::uint32_t a){return reinterpret_cast<void*>(a);}
struct Files {bool ready{};~Files(){if(ready)RtFSManagerClose();}};
void Run(const std::filesystem::path& path){
    pal4::test::NativeFrameSession frame;Check(frame.Start(800,600) && !IsWindowVisible(frame.window),"hidden equipment frame");
    Files files;files.ready=RtFSManagerOpen(8)!=FALSE;Check(files.ready,"equipment filesystem");
    pal4::NativeCeguiUiSession ui;Check(ui.Start(path,800,600),"equipment UI");
    pal4::test::NativePropertyWindowLayout resources(ui,path);
    Check(ui.LoadOriginalMinimapLayout(path) && ui.SetMinimapLayoutVisible(false),"equipment original systemBold startup font");
    Check(ui.BindDialogRootParent(resources.state.Read(36u),false),"hide bootstrap Property fixture");
    std::vector<std::array<unsigned,3>> layout_events;std::vector<unsigned> clicked_slots;
    std::function<bool(unsigned,unsigned,unsigned)> page_route;
    pal4::NativeEquipmentWindowLayout layout(ui,0x1000u,[&](auto owner,auto handler,auto argument){layout_events.push_back({owner,handler,argument});
        if(handler==0x452e40u){unsigned source;std::memcpy(&source,Ptr(argument+8u),4u);clicked_slots.push_back(source);}return page_route?page_route(owner,handler,argument):true;});
    auto& state=layout.State();const auto m=resources.module;const auto list=Ptr(state.Read(0x40u));
    Check(ui.BindDialogRootParent(state.Read(36u),true),"equipment attachment");
    for(unsigned i=0;i<6u;++i)Export<void (__thiscall*)(void*,unsigned)>(m,"?setID@Window@CEGUI@@QAEXI@Z")(Ptr(state.Read(0x98u+i*4u)),900u+i);
    Check(state.Read(0xf0u)==5u && !state.Read(0xf4u) && state.Read(0x110u)==layout.FrameAddress() && state.Read(0x130u)==layout.FrameAddress(),"native constructor original initial state and owners");
    constexpr std::array<unsigned,6> primary{0x64u,0x50u,0x54u,0x58u,0x5cu,0x60u},secondary{0x98u,0x9cu,0xa8u,0xa4u,0xa0u,0xacu};
    for(unsigned i=0;i<6u;++i){Check(reinterpret_cast<const unsigned*>(state.Read(0x30u))[i]==state.Read(primary[i]),"original primary navigation order");
        Check(reinterpret_cast<const unsigned*>(state.Read(0x120u))[i]==state.Read(secondary[i]),"original secondary navigation order");}
    pal4::test::ClickNativeCeguiWindow(ui,state.Read(0x50u));layout.RethrowEventError();
    Check(std::any_of(layout_events.begin(),layout_events.end(),[&](const auto& e){return e[0]==layout.FrameAddress() && e[1]==0x452c20u;}),"actual DLL category click reaches rewritten page handler");
    layout.DispatchCommand("use_equipment",12345u);Check(layout_events.back()==std::array<unsigned,3>{layout.FrameAddress(),0x4502b0u,12345u},"native command descriptor preserves instance argument");
    layout_events.clear();
    {
        pal4::NativeEquipmentSlotNavigation* current{};std::vector<bool> panels;
        const auto member=layout.FrameAddress()+0x118u,list_member=layout.FrameAddress()+0x10cu;
        pal4::NativeEventHandlerArray navigation_activity([&](unsigned owner,unsigned slot){
            if(owner==member && slot==12u)return current->SelectFirstEnabled()?1u:0u;return 1u;
        },[&](unsigned owner,unsigned slot,unsigned e)->std::uint8_t{Check(owner==member && slot==0u,"equipment slot typed activity predicate");return current->HandleKey(e)?1u:0u;});
        pal4::NativeEquipmentSlotNavigation slots(state,navigation_activity,[&](bool v){panels.push_back(v);});current=&slots;
        std::array<bool,6> enabled_before{};
        for(unsigned i=0;i<6u;++i){const auto w=state.Read(0x98u+i*4u);enabled_before[i]=!Export<bool (__thiscall*)(const void*,bool)>(m,"?isDisabled@Window@CEGUI@@QBE_N_N@Z")(Ptr(w),false);
            Export<void (__thiscall*)(void*,bool)>(m,"?setEnabled@Window@CEGUI@@QAEX_N@Z")(Ptr(w),true);}
        std::array<unsigned,5> key{};const auto press=[&](unsigned code){key[4]=code;return navigation_activity.DispatchKeyDown(member,reinterpret_cast<unsigned>(key.data()));};
        const auto hover=[&](unsigned index){return *reinterpret_cast<const unsigned char*>(state.Read(0x98u+index*4u)+1201u)!=0u;};
        (void)navigation_activity.Insert(0xabcdu);(void)navigation_activity.Insert(list_member);(void)navigation_activity.Insert(member);
        state.Write(0x12cu,2u);Check(press(205u) && state.Read(0x12cu)==5u && !hover(4u) && hover(5u),"right key reads actual permuted navigation vector source");
        Check(press(203u) && state.Read(0x12cu)==2u && hover(4u) && !hover(5u),"left key highlights navigation entry2 which is page slot4");
        state.Write(0x12cu,1u);Export<void (__thiscall*)(void*,bool)>(m,"?setEnabled@Window@CEGUI@@QAEX_N@Z")(Ptr(state.Read(0xa0u)),false);
        Check(press(205u) && state.Read(0x12cu)==1u,"right key tests disabled destination through permuted table rather than page slot4");
        Export<void (__thiscall*)(void*,bool)>(m,"?setEnabled@Window@CEGUI@@QAEX_N@Z")(Ptr(state.Read(0xa0u)),true);
        state.Write(0x12cu,0u);Check(press(200u) && state.Read(0x12cu)==5u,"up key wraps through existing shared navigation code");
        state.Write(0x12cu,2u);const auto before_click=clicked_slots.size();Check(frame.Begin() && ui.Render(),"equipment navigation GPU frame");(void)frame.ReadPixels();
        Check(press(28u),"equipment Enter handled");layout.RethrowEventError();
        Check(clicked_slots.size()==before_click+1u && clicked_slots.back()==state.Read(0xa0u),"Enter activates original page slot2 rather than permuted navigation slot2");
        for(unsigned i=0;i<6u;++i)*reinterpret_cast<unsigned char*>(state.Read(0x98u+i*4u)+1201u)=1u;
        Export<void (__thiscall*)(void*,bool)>(m,"?setEnabled@Window@CEGUI@@QAEX_N@Z")(Ptr(state.Read(0xa8u)),false);
        std::array<unsigned,3> mouse{0u,0u,state.Read(0xa0u)};Check(slots.HandleMouseMove(reinterpret_cast<unsigned>(mouse.data())),"equipment registered mouse virtual reaches shared hover handler");
        for(unsigned i=0;i<6u;++i)Check(hover(i)==(i==2u || i==4u),"mouse move preserves source/disabled slots and clears other hover flags");
        Check(press(15u) && navigation_activity.GetLast()==list_member,"Tab releases slot activity and returns to equipment list");
        (void)navigation_activity.Insert(member);Check(press(1u) && navigation_activity.Size()==1u && navigation_activity.GetLast()==0xabcdu && panels==std::vector<bool>{false},"Esc removes slot/list activities and hides panels while preserving parent");
        navigation_activity.ReleaseStorage();(void)slots.ClearHighlight();
        for(unsigned i=0;i<6u;++i)Export<void (__thiscall*)(void*,bool)>(m,"?setEnabled@Window@CEGUI@@QAEX_N@Z")(Ptr(state.Read(0x98u+i*4u)),enabled_before[i]);
        state.Write(0x12cu,0u);
    }
    std::cout<<"Equipment original layout ready\n";
    const auto count=[&]{return Export<unsigned (__thiscall*)(const void*)>(m,"?getRowCount@MultiColumnList@CEGUI@@QBEIXZ")(list);};
    const auto cell=[&](unsigned row,unsigned column){const std::array<unsigned,2> grid{row,column};return reinterpret_cast<unsigned>(Export<void* (__thiscall*)(const void*,const void*)>(m,
        "?getItemAtGridReference@MultiColumnList@CEGUI@@QBEPAVListboxItem@2@ABUMCLGridRef@2@@Z")(list,grid.data()));};
    const auto render=[&]{Check(frame.Begin() && ui.Render(),"equipment real GPU draw");return frame.ReadPixels();};
    const auto baseline=render();
    std::cout<<"Equipment empty GPU baseline ready\n";
    pal4::NativeGameDatabase db;Check(db.Load(path),"equipment original database");
    pal4::NativeInventorySession inventory;(void)inventory.Start();inventory.BindDatabase(db);
    pal4::PlayerManagerObjectState players;
    for(unsigned p=0;p<4u;++p){players.Word((p*0xb14u+0x654u)/4u)=pal4::PlayerManagerSlot::kScriptVtable;
        players.Word((p*0xb14u+0x654u)/4u+140u)=20u;}
    players.Word(0xb08u/4u)=1u;
    // Two instances of each real record prove sequence identity is independent
    // of database ID and category iteration is based on inventory TYPE slots.
    for(const auto& row:db.Tables().at("Equip").rows){const auto id=std::get<unsigned>(row[0]);
        (void)inventory.ProcessEquipment(inventory.Address()+0x7cu,id);(void)inventory.ProcessEquipment(inventory.Address()+0x7cu,id);}
    std::vector<unsigned> sequences;
    for(unsigned type=0;type<7u;++type)for(unsigned index=0,n=inventory.EquipmentCount(type);index<n;++index)
        sequences.push_back(inventory.ReadDword(inventory.EquipmentAt(type,index)+400u));
    Check(sequences.size()==db.Tables().at("Equip").rows.size()*2u,"all original equipment instances in TYPE slots");
    const auto recent=inventory.RecentEquipmentSequences();
    Check(recent.size()==6u,"original equipment recent queue retains six instances");
    auto sorted_sequences=sequences;std::sort(sorted_sequences.begin(),sorted_sequences.end());
    Check(std::equal(recent.begin(),recent.end(),sorted_sequences.end()-6),"recent queue contains the last six acquired instances in original order");
    unsigned equipped{},hidden{};
    for(auto sequence:sequences){const auto item=inventory.FindInventoryEntry(inventory.Address()+0x7cu,sequence,7u);
        if(inventory.ReadDword(item+0x168u)>0u && !equipped){equipped=sequence;players.Word(0x920u/4u)=sequence;}
        else if(!hidden){hidden=sequence;players.Word((0xb14u+0x920u)/4u)=sequence;}}
    Check(equipped && hidden,"equipment fixture has equipped and inactive-owner instances");
    const auto allocations=inventory.OutstandingAllocations();
    pal4::NativeEquipmentWindowList page(state,inventory,players,db);
    std::cout<<"Equipment instances="<<sequences.size()<<" recent="<<recent.size()<<" native controller ready\n";
    for(unsigned repeat=0;repeat<2u;++repeat)for(unsigned character=0;character<4u;++character)for(unsigned category=0;category<6u;++category){
        page.Populate(category,character);std::set<unsigned> expected;
        for(auto sequence:sequences){const auto item=inventory.FindInventoryEntry(inventory.Address()+0x7cu,sequence,7u);
            if(sequence==hidden)continue;
            const auto type=inventory.ReadDword(item+148u);unsigned actual_category=99u;
            for(const auto& relation:db.Tables().at("EquipRelation").rows)if(std::get<unsigned>(relation[2])==type)actual_category=std::get<unsigned>(relation[1]);
            if((category==5u && std::find(recent.begin(),recent.end(),sequence)!=recent.end()) || (category<5u && actual_category==category))expected.insert(sequence);}
        Check(count()==expected.size() && page.OwnedItemCount()==expected.size()*2u,"native equipment rows and owned cells");
        std::set<unsigned> actual;
        for(unsigned row=0;row<count();++row){const auto name=cell(row,0),level=cell(row,1);Check(name && level,"both original equipment cells");
            const auto get_id=Export<unsigned (__thiscall*)(const void*)>(m,"?getID@ListboxItem@CEGUI@@QBEIXZ");
            const auto sequence=get_id(Ptr(name));actual.insert(sequence);
            Check(get_id(Ptr(level))==sequence,"both cells retain inventory sequence");
            const auto item=inventory.FindInventoryEntry(inventory.Address()+0x7cu,sequence,7u);
            Check(item!=0u,"native cell ID resolves original equipment instance");
            const auto required=inventory.ReadDword(item+0x168u+character*4u);
            const auto text=[&](unsigned address){const auto native=Export<const void* (__thiscall*)(const void*)>(m,"?getText@ListboxItem@CEGUI@@QBEABVString@2@XZ")(Ptr(address));
                return std::string(Export<const char* (__thiscall*)(const void*)>(m,"?c_str@String@CEGUI@@QBEPBDXZ")(native));};
            const auto name_pointer=inventory.ReadDword(item+0x9cu);
            Check(pal4::ConvertGameTextToUtf8(name_pointer?reinterpret_cast<const char*>(name_pointer):"")==std::optional<std::string>(text(name)) &&
                text(level)==(required?std::to_string(required):"--"),"original Chinese name and required-level text reach DLL cells");
            const bool disabled=!required || required>20u;
            Check(static_cast<unsigned char>(*reinterpret_cast<const std::byte*>(name+421u))==static_cast<unsigned char>(disabled) &&
                static_cast<unsigned char>(*reinterpret_cast<const std::byte*>(level+421u))==static_cast<unsigned char>(disabled),"original disabled appearance in both columns");
            Check((*reinterpret_cast<const unsigned*>(name+528u)!=0u)==(sequence==equipped),"actual equipped badge follows active inventory sequence");}
        Check(actual==expected,"every native row is an original eligible inventory instance");
        Check(inventory.OutstandingAllocations()==allocations,"equipment list does not allocate/mutate inventory");
    }
    unsigned equipped_category=99u;
    const auto equipped_record=inventory.FindInventoryEntry(inventory.Address()+0x7cu,equipped,7u);
    for(const auto& row:db.Tables().at("EquipRelation").rows)if(std::get<unsigned>(row[2])==inventory.ReadDword(equipped_record+148u))equipped_category=std::get<unsigned>(row[1]);
    page.Populate(equipped_category,0u);const auto image=render();Check(image.size()==baseline.size(),"equipment GPU raster size");
    unsigned pixels{};for(int y=35;y<190;++y)for(int x=340;x<710;++x){const auto i=static_cast<std::size_t>(y*800+x)*4u;
        pixels+=(image[i]!=baseline[i] || image[i+1]!=baseline[i+1] || image[i+2]!=baseline[i+2]);}
    Check(pixels>100u,"actual equipment rows change list-region GPU pixels");
    const auto first=cell(0u,0u);auto* badge=reinterpret_cast<unsigned*>(first+528u);const auto badge_image=*badge;
    Check(badge_image!=0u,"equipped entry is first and has actual badge image");
    *badge=0u;Export<void (__thiscall*)(void*)>(m,"?handleUpdatedItemData@MultiColumnList@CEGUI@@QAEXXZ")(list);const auto no_badge=render();
    unsigned badge_pixels{};for(std::size_t i=0;i<image.size();i+=4u)badge_pixels+=(image[i]!=no_badge[i] || image[i+1]!=no_badge[i+1] || image[i+2]!=no_badge[i+2]);
    Check(badge_pixels>5u,"equipped marker produces distinct actual GPU output");
    *badge=badge_image;pal4::NativeCeguiWidgetImages(m).RequestRedraw(state.Read(0x40u));
    page.Populate(5u,4u);Check(count()==0u && !page.OwnedItemCount(),"character sentinel leaves no rows");
    pal4::NativeEquipmentCharacterDisplay character_display(state,inventory,players);
    const auto saved_players=players;
    constexpr std::array<unsigned,6> slots{0u,2u,3u,1u,4u,5u};
    const auto window_text=[&](unsigned w){const auto native=Export<const void* (__thiscall*)(const void*)>(m,"?getText@Window@CEGUI@@QBEABVString@2@XZ")(Ptr(w));
        return std::string(Export<const char* (__thiscall*)(const void*)>(m,"?c_str@String@CEGUI@@QBEPBDXZ")(native));};
    const auto disabled=[&](unsigned w){return Export<bool (__thiscall*)(const void*,bool)>(m,"?isDisabled@Window@CEGUI@@QBE_N_N@Z")(Ptr(w),true);};
    const auto expected_name=[&](unsigned sequence){const auto item=inventory.FindInventoryEntry(inventory.Address()+0x7cu,sequence,7u);
        Check(item!=0u,"character equipment fixture record");const auto name=inventory.ReadDword(item+0x9cu);
        return *pal4::ConvertGameTextToUtf8(name?reinterpret_cast<const char*>(name):"");};
    const auto character_image=[&]{return reinterpret_cast<unsigned>(Export<const void* (__thiscall*)(const void*)>(m,"?getImage@StaticImage@CEGUI@@QBEPBVImage@2@XZ")(Ptr(state.Read(0x44u))));};
    const std::array<const char*,4> images{"yuntianhe","hanlingsha","liumengli","murongziying"};
    for(unsigned c=0;c<4u;++c){for(unsigned slot=0;slot<6u;++slot)players.Word((c*0xb14u+0x920u+slot*4u)/4u)=sequences[c*6u+slot];
        character_display.SetCharacterImage(c);character_display.UpdateEquipment(c);
        Check(character_image()==pal4::NativeCeguiWidgetImages(m).Image("zi0",images[c]),"original character nameplate image");
        for(unsigned i=0;i<6u;++i){const auto w=state.Read(0x98u+i*4u);
            Check(window_text(w)==expected_name(sequences[c*6u+slots[i]]) && disabled(w)==(i==0u),"six original controls use userData slot permutation, Chinese names and first-control disable rule");}}
    character_display.SetCharacterImage(0u);const auto name0=render();character_display.SetCharacterImage(1u);const auto name1=render();
    unsigned nameplate_pixels{};for(std::size_t i=0;i<name0.size();i+=4u)nameplate_pixels+=(name0[i]!=name1[i] || name0[i+1]!=name1[i+1] || name0[i+2]!=name1[i+2]);
    Check(nameplate_pixels>5u,"actual character nameplate change reaches GPU");
    const auto selected_image=character_image();character_display.SetCharacterImage(4u);character_display.SetCharacterImage(99u);character_display.UpdateEquipment(4u);
    Check(character_image()==selected_image,"sentinel and invalid image indices retain original nameplate");
    for(unsigned i=0;i<6u;++i)Check(window_text(state.Read(0x98u+i*4u)).empty() && disabled(state.Read(0x98u+i*4u)),"sentinel clears and disables equipment controls");
    const auto direct_window=state.Read(0x9cu);
    Export<void (__thiscall*)(void*,void*)>(m,"?setUserData@Window@CEGUI@@QAEXPAX@Z")(Ptr(direct_window),Ptr(sequences.back()));
    character_display.UpdateEquipment(0u);Check(window_text(direct_window)==expected_name(sequences.back()) && !disabled(direct_window),"userData above five is direct inventory sequence");
    Export<void (__thiscall*)(void*,void*)>(m,"?setUserData@Window@CEGUI@@QAEXPAX@Z")(Ptr(direct_window),Ptr(0xffffffffu));
    character_display.UpdateEquipment(0u);Check(window_text(direct_window).empty() && disabled(direct_window),"missing equipment record clears previous label");
    Export<void (__thiscall*)(void*,void*)>(m,"?setUserData@Window@CEGUI@@QAEXPAX@Z")(Ptr(direct_window),Ptr(2u));
    const auto no_name_item=inventory.FindInventoryEntry(inventory.Address()+0x7cu,sequences[2u],7u);
    auto* no_name_pointer=reinterpret_cast<unsigned*>(no_name_item+0x9cu);const auto saved_name=*no_name_pointer;*no_name_pointer=0u;
    character_display.UpdateEquipment(0u);Check(window_text(direct_window).empty() && !disabled(direct_window),"null name clears text but retains populated-slot availability");*no_name_pointer=saved_name;
    players=saved_players;
    {
        pal4::PlayerManagerObjectState detail_players{};pal4::NativeRoleRecordCatalog roles(db);pal4::NativePlayerSlotCopyRuntime copier;
        for(unsigned c=0;c<4u;++c){const auto p=reinterpret_cast<unsigned>(&detail_players.slots[c]);
            (void)pal4::PlayerManagerSlotDataCopier::Copy(p,roles.Address(c),copier);
            detail_players.Word((c*0xb14u+0x654u)/4u)=pal4::PlayerManagerSlot::kScriptVtable;
            (void)pal4::NativePlayerScriptPropertyRuntime::Set(p+0x654u,0x44u,4u,40u);}
        pal4::NativeBaseObjectMemory memory;pal4::NativePlayerScriptRefreshRuntime refresh(memory,inventory);
        unsigned retired{};state.Write(0xf8u,0x1234u);
        pal4::NativeEquipmentItemDetails details(state,inventory,detail_players,db,refresh,[&](unsigned id){Check(id==0x1234u,"details Clear forwards actual preview identity");++retired;return id+1u;});
        Check(details.Clear()==0x1235u && retired==1u,"details Clear dispatches preview removal endpoint");const auto empty_details=render();
        unsigned previews{},ineligible{},descriptions{},eligible_record{};
        std::array<pal4::EquipmentStatSnapshot,4> original;
        for(unsigned c=0;c<4u;++c){const auto p=reinterpret_cast<unsigned>(&detail_players.slots[c]);(void)refresh.Refresh(p+0x654u);
            std::memcpy(original[c].data(),Ptr(p+0x7a8u),sizeof(original[c]));}
        for(unsigned c=0;c<4u;++c){state.Write(0xf4u,c);const auto p=reinterpret_cast<unsigned>(&detail_players.slots[c]);
            for(unsigned n=0;n<sequences.size();n+=2u){const auto record=inventory.FindInventoryEntry(inventory.Address()+0x7cu,sequences[n],7u);
                const auto required=inventory.ReadDword(record+0x168u+c*4u);const bool eligible=required>0u && required<=40u;
                const auto before=inventory.OutstandingAllocations();details.Update(record);++descriptions;
                const auto description_pointer=inventory.ReadDword(record+0xacu);
                Check(window_text(state.Read(0x68u))==*pal4::ConvertGameTextToUtf8(description_pointer?reinterpret_cast<const char*>(description_pointer):""),"original EA_DESCRIPTION reaches actual detail control");
                const auto attributes=window_text(state.Read(0x6cu));
                for(unsigned id:{0x59u,0x58u})Check(attributes.find(*pal4::ConvertGameTextToUtf8(*db.FindTableTextById("GameString",id)))!=std::string::npos,"original GameString attribute labels reach DLL text");
                const auto extra=inventory.ReadDword(record+0xbcu);if(extra && *reinterpret_cast<const char*>(extra))
                    Check(attributes.find(*pal4::ConvertGameTextToUtf8(reinterpret_cast<const char*>(extra)))!=std::string::npos,"original EA_LING_INFO appended to attributes");
                alignas(4) std::array<std::byte,96> colours{};
                Export<void* (__thiscall*)(const void*,void*)>(m,"?getImageColours@StaticImage@CEGUI@@QBE?AVColourRect@2@XZ")(Ptr(state.Read(0xe0u+c*4u)),colours.data());
                const auto red=Export<float (__thiscall*)(const void*)>(m,"?getRed@colour@CEGUI@@QBEMXZ")(colours.data());
                Check(std::abs(red-(eligible?1.0f:0.57647061f))<0.00001f,"actual character image tint follows equipment qualification");
                for(unsigned stat=0;stat<10u;++stat){const auto index=stat<5u?stat+4u:stat+26u;
                    const auto delta=eligible?static_cast<std::int32_t>(inventory.ReadDword(record+index*4u)):0;
                    const auto actual=window_text(state.Read(0x70u+stat*4u)),expected=pal4::EquipmentItemDetails::FormatStat(original[c][index],original[c][index]+delta);
                    if(actual!=expected)std::cerr<<"Projection mismatch character="<<c<<" sequence="<<sequences[n]<<" stat="<<index<<" baseline="<<original[c][index]<<" delta="<<delta<<" required="<<required<<" actual=["<<actual<<"] expected=["<<expected<<"]\n";
                    Check(actual==expected,"actual refreshed projection matches original equipment integer bonus");}
                pal4::EquipmentStatSnapshot restored;std::memcpy(restored.data(),Ptr(p+0x7a8u),sizeof(restored));
                Check(restored==original[c] && inventory.OutstandingAllocations()==before,"preview restores all84 live stats without inventory allocations");
                for(unsigned slot=0;slot<6u;++slot)Check(detail_players.Word((c*0xb14u+0x920u+slot*4u)/4u)==0u,"preview restores all six originally empty slots");
                if(eligible){++previews;if(!eligible_record && c==0u)eligible_record=record;}else ++ineligible;
            }}
        Check(previews && ineligible && descriptions==db.Tables().at("Equip").rows.size()*4u,"all original equipment/four-role eligible and locked projections");
        // Preview replacement must restore a real nonzero old instance, not
        // only the zero-slot case exercised by the complete catalog sweep.
        state.Write(0xf4u,0u);const auto p=reinterpret_cast<unsigned>(&detail_players.slots[0]);
        pal4::InventoryItemSlotsState empty_slots;
        const auto replacement_slot=pal4::InventoryItemSlots::FindSlotOrClassify(empty_slots,eligible_record,inventory);
        Check(replacement_slot<6u,"eligible replacement slot");
        const auto old_record=inventory.EquipmentAt(inventory.ReadDword(eligible_record+148u),1u);
        const auto old_sequence=inventory.ReadDword(old_record+400u);
        detail_players.Word((0x920u+replacement_slot*4u)/4u)=old_sequence;(void)refresh.Refresh(p+0x654u);
        pal4::EquipmentStatSnapshot before_replacement;std::memcpy(before_replacement.data(),Ptr(p+0x7a8u),sizeof(before_replacement));
        details.Update(eligible_record);pal4::EquipmentStatSnapshot after_replacement;std::memcpy(after_replacement.data(),Ptr(p+0x7a8u),sizeof(after_replacement));
        Check(before_replacement==after_replacement && detail_players.Word((0x920u+replacement_slot*4u)/4u)==old_sequence,"preview restores real previous equipment instance and all derived stats");
        detail_players.Word((0x920u+replacement_slot*4u)/4u)=0u;(void)refresh.Refresh(p+0x654u);
        state.Write(0xf4u,0u);Check(eligible_record!=0u,"real eligible detail record");details.Update(eligible_record);const auto full_details=render();
        unsigned detail_pixels{};for(std::size_t i=0;i<full_details.size();i+=4u)detail_pixels+=(full_details[i]!=empty_details[i] || full_details[i+1]!=empty_details[i+1] || full_details[i+2]!=empty_details[i+2]);
        Check(detail_pixels>100u,"actual equipment descriptions/stat labels render to GPU");
        details.Update(0u);Check(window_text(state.Read(0x68u)).empty() && window_text(state.Read(0x6cu)).empty(),"empty hover clears both detail controls");
        state.Write(0xf4u,4u);details.Update(0u);for(unsigned stat=0;stat<10u;++stat)Check(window_text(state.Read(0x70u+stat*4u)).empty(),"character sentinel clears all ten stats");
        state.Write(0xf4u,0u);bool stale{};try{details.Update(0xdeadbeefu);}catch(const std::invalid_argument&){stale=true;}Check(stale,"stale inventory pointer rejects before dereference");
        Check(details.Clear()==0x1235u && retired==2u && !memory.OutstandingAllocations(),"details cleanup dispatch and native refresh ownership");
        {
            pal4::NativeCommonDialogLayout confirmation(m,ui,path);
            pal4::NativeEquipmentWindowList action_list(state,inventory,detail_players,db);
            pal4::NativeEquipmentCharacterDisplay action_character(state,inventory,detail_players);
            std::vector<bool> model_visibility;
            pal4::NativeEquipmentWindowPresentation presentation(state,detail_players,action_list,details,action_character,[&](bool v){model_visibility.push_back(v);});
            const auto visible=[&](unsigned w){return Export<bool (__thiscall*)(const void*,bool)>(m,"?isVisible@Window@CEGUI@@QBE_N_N@Z")(Ptr(w),false);};
            for(unsigned c=0;c<=4u;++c)for(unsigned category=0;category<6u;++category){
                presentation.UpdateList(category,c);layout.RethrowEventError();
                Check(state.Read(0xf0u)==category && state.Read(0xf4u)==c && model_visibility.back(),"original presentation commits category/character and shows model endpoint");
                for(auto offset:{0xbcu,0xb8u,0xc0u,0xc4u,0xb4u})Check(visible(state.Read(offset)),"presentation shows all five original panels");
                if(count())Check(Export<int (__thiscall*)(void*)>(m,"?getHoverRowIndex@MultiColumnList@CEGUI@@QAEHXZ")(list)==0,"presentation selects first actual DLL row after population");
                for(unsigned n=0;n<6u;++n)Check(Export<bool (__thiscall*)(const void*)>(m,"?isSelected@ButtonBase@CEGUI@@QBE_NXZ")(Ptr(state.Read(0x50u+n*4u)))==(n==category),"presentation selects exactly current category");
                for(unsigned n=0;n<4u;++n)Check(visible(state.Read(0xe0u+n*4u))==(n==c),"presentation exposes exactly selected role marker");
                if(c<4u)Check(window_text(state.Read(0xb4u))==pal4::EquipmentLevelFormatter::Format(40),"presentation uses original Level_font text format");
                else Check(!count() && window_text(state.Read(0xb4u)).empty(),"sentinel presentation clears level and inventory list");
            }
            presentation.SetPanelsVisible(false);Check(!model_visibility.back(),"panel hide forwards original model cleanup endpoint");
            for(auto offset:{0xbcu,0xb8u,0xc0u,0xc4u,0xb4u})Check(!visible(state.Read(offset)),"all five original equipment panels hide");
            Check(Export<const void* (__thiscall*)(const void*)>(m,"?getImage@StaticImage@CEGUI@@QBEPBVImage@2@XZ")(Ptr(state.Read(0xb0u)))==Ptr(pal4::NativeCeguiWidgetImages(m).Image("zi0","zhuangbei")),"panel hide restores original equipment category image");
            unsigned list_updates{},display_updates{};std::vector<std::string> sounds;
            pal4::NativeEquipmentActionServices endpoints;
            endpoints.sound=[&](std::string_view file,unsigned mode){Check(mode==1u,"equipment action sound mode");sounds.emplace_back(file);};
            endpoints.refresh_list=[&](unsigned category,unsigned character){++list_updates;presentation.UpdateList(category,character);};
            endpoints.refresh_display=[&](unsigned record,unsigned character){Check(character==state.Read(0xf4u),"equipment action display character");++display_updates;presentation.UpdateDisplay(record,character);};
            pal4::NativeEquipmentWindowActions actions(state,inventory,detail_players,db,refresh,confirmation,std::move(endpoints));
            unsigned category=99u;for(const auto& relation:db.Tables().at("EquipRelation").rows)if(std::get<unsigned>(relation[2])==inventory.ReadDword(eligible_record+148u))category=std::get<unsigned>(relation[1]);
            action_list.Populate(category,0u);const auto sequence=inventory.ReadDword(eligible_record+400u),definition_id=inventory.ReadDword(eligible_record+144u);
            pal4::NativeEventHandlerArray activity([](auto,auto)->unsigned{throw std::logic_error("unexpected menu global virtual");},
                [](auto,auto,auto)->std::uint8_t{throw std::logic_error("unexpected menu global predicate");});
            unsigned commands{};
            pal4::NativeEquipmentWindowSelection selection(ui,activity,state,inventory,detail_players,db,[&](auto name,auto id){
                ++commands;Check(id==sequence,"equipment menu dispatches exact sequence");
                if(name=="use_equipment")(void)actions.Equip(id);else if(name=="discard_equipment")(void)actions.ShowDiscard(id);
                else throw std::logic_error("equipment menu unexpected command");});
            const auto hover_sequence=[&](unsigned wanted){for(unsigned row=0;row<count();++row){
                if(Export<unsigned (__thiscall*)(const void*)>(m,"?getID@ListboxItem@CEGUI@@QBEIXZ")(Ptr(cell(row,0u)))==wanted){
                    Export<void (__thiscall*)(void*,int)>(m,"?setHoverRowIndex@MultiColumnList@CEGUI@@QAEXH@Z")(list,row);return;}}
                throw std::logic_error("equipment menu target row missing");};
            hover_sequence(old_sequence);Check(selection.Show({400.f,220.f}),"menu captures another live equipment instance");
            Check(inventory.RemoveEquipment(old_sequence),"remove instance while its menu remains open");
            action_list.Populate(category,0u);
            pal4::test::ClickNativeCeguiWindow(ui,selection.Menu().Button(1u));selection.RethrowEventError();
            Check(!commands && !selection.Menu().Visible(),"stale menu click closes without dispatching freed equipment");
            const auto allocated_before=inventory.OutstandingAllocations();
            Export<void (__thiscall*)(void*,int)>(m,"?setHoverRowIndex@MultiColumnList@CEGUI@@QAEXH@Z")(list,-1);
            Check(selection.Show({400.f,220.f}) && !selection.Menu().Visible(),"missing hover is original no-op");
            hover_sequence(sequence);
            Check(selection.Show({400.f,220.f}) && selection.Menu().ButtonCount()==2u && activity.Size()==0u,"actual equipment menu opens two choices without replacing parent activity");
            Check(!disabled(selection.Menu().Button(0u)) && !disabled(selection.Menu().Button(1u)),"eligible equipment menu enables both choices");
            Check(window_text(selection.Menu().Button(0u))==*pal4::ConvertGameTextToUtf8(*db.FindTableTextById("GameString",0x18u)) &&
                window_text(selection.Menu().Button(1u))==*pal4::ConvertGameTextToUtf8(*db.FindTableTextById("GameString",0x17u)),"equipment menu uses original labels/order");
            const auto level_word=detail_players.Word((0x654u+140u*4u)/4u);
            detail_players.Word((0x654u+140u*4u)/4u)=0u;
            Check(selection.Show({400.f,220.f}) && disabled(selection.Menu().Button(0u)) && !disabled(selection.Menu().Button(1u)),"low level only disables equipment use");
            detail_players.Word((0x654u+140u*4u)/4u)=level_word;
            bool zero_requirement{};for(unsigned c=0;c<4u;++c)if(!inventory.ReadDword(eligible_record+0x168u+c*4u)){
                state.Write(0xf4u,c);Check(selection.Show({400.f,220.f}) && disabled(selection.Menu().Button(0u)) && !disabled(selection.Menu().Button(1u)),"zero role requirement forbids use but permits discard");
                zero_requirement=true;break;}
            Check(zero_requirement,"real selected equipment has an unsupported role");state.Write(0xf4u,0u);
            const auto owner_active=detail_players.Word((0xb14u+0xb08u)/4u);
            const auto owner_slot=detail_players.Word((0xb14u+0x920u)/4u);
            detail_players.Word((0xb14u+0xb08u)/4u)=1u;detail_players.Word((0xb14u+0x920u)/4u)=sequence;
            Check(selection.Show({400.f,220.f}) && disabled(selection.Menu().Button(0u)) && disabled(selection.Menu().Button(1u)),"active equipment owner disables both menu choices");
            detail_players.Word((0xb14u+0xb08u)/4u)=0u;
            Check(selection.Show({400.f,220.f}) && !disabled(selection.Menu().Button(0u)) && !disabled(selection.Menu().Button(1u)),"inactive owner does not block original menu gates");
            detail_players.Word((0xb14u+0xb08u)/4u)=owner_active;detail_players.Word((0xb14u+0x920u)/4u)=owner_slot;
            state.Write(0xf4u,4u);Check(selection.Show({400.f,220.f}) && selection.Menu().Visible(),"character sentinel preserves original no-op menu");state.Write(0xf4u,0u);
            pal4::test::ClickNativeCeguiWindow(ui,selection.Menu().Button(0u));selection.RethrowEventError();
            Check(commands==1u && !selection.Menu().Visible() && activity.Size()==0u && detail_players.Word((0x920u+replacement_slot*4u)/4u)==sequence,"actual DLL menu click equips selected instance and releases modal activity");
            selection.Close();activity.ReleaseStorage();
            Check(sounds==std::vector<std::string>{"wa13.wav"} && list_updates==1u && display_updates==1u && inventory.OutstandingAllocations()==allocated_before,"Equip actual refresh/list/display and exact sound");
            for(unsigned stat=4u;stat<9u;++stat)Check(*reinterpret_cast<const int*>(p+0x7a8u+stat*4u)==original[0][stat]+static_cast<int>(inventory.ReadDword(eligible_record+stat*4u)),"Equip persists real stat bonus");
            unsigned control{};for(unsigned i=0;i<6u;++i){const auto w=state.Read(0x98u+i*4u);
                if(reinterpret_cast<unsigned>(Export<void* (__thiscall*)(const void*)>(m,"?getUserData@Window@CEGUI@@QBEPAXXZ")(Ptr(w)))==replacement_slot)control=w;}
            Export<void (__thiscall*)(void*,int)>(m,"?setHoverRowIndex@MultiColumnList@CEGUI@@QAEXH@Z")(list,1);
            const auto row_before=Export<int (__thiscall*)(void*)>(m,"?getHoverRowIndex@MultiColumnList@CEGUI@@QAEHXZ")(list);
            Check(actions.Unequip(control) && !detail_players.Word((0x920u+replacement_slot*4u)/4u) && inventory.FindInventoryEntry(inventory.Address()+0x7cu,sequence,7u)==eligible_record,"Unequip clears slot and retains exact inventory record");
            Check(Export<int (__thiscall*)(void*)>(m,"?getHoverRowIndex@MultiColumnList@CEGUI@@QAEHXZ")(list)==row_before && sounds.back()=="wa13.wav","Unequip restores hover and uses equipment sound");
            {
                std::array<unsigned,4> active_before{};for(unsigned c=0;c<4u;++c){auto& value=detail_players.Word((c*0xb14u+0xb08u)/4u);active_before[c]=value;value=c<2u?1u:0u;}
                pal4::NativeEquipmentWindowInteraction* current{};
                pal4::NativeEventHandlerArray page_activity([&](unsigned member,unsigned slot){
                    if(current && current->OwnsActivity(member))return current->Callback(member,slot);
                    Check(member==0xabcd,"equipment page preserves known parent callback");return 1u;
                },[&](unsigned member,unsigned slot,unsigned event)->std::uint8_t{
                    if(current && current->OwnsActivity(member))return current->Predicate(member,slot,event);
                    Check(member==0xabcd,"equipment page known parent predicate");return 0u;});
                std::vector<std::string> page_events;std::vector<unsigned> model_records;std::vector<bool> model_states;unsigned tutorials{};
                pal4::NativeEquipmentInteractionServices service;
                service.event=[&](std::string_view name,unsigned,unsigned){page_events.emplace_back(name);};
                service.tutorial=[&](unsigned id){Check(id==3u,"original equipment tutorial3");++tutorials;};
                service.model_update=[&](unsigned record){Check(record!=0u,"page model callback only receives live nonempty hover");model_records.push_back(record);};
                service.model_visible=[&](bool v){model_states.push_back(v);};service.sound=[&](auto file,auto mode){Check(mode==1u,"page sound mode");sounds.emplace_back(file);};
                pal4::NativeEquipmentWindowInteraction interaction(ui,layout,inventory,detail_players,db,presentation,details,actions,refresh,page_activity,0x1000u,std::move(service));current=&interaction;
                page_route=[&](auto owner,auto handler,auto event){return interaction.FromDll(owner,handler,event);};
                (void)page_activity.Insert(0xabcd);interaction.SetVisible(true);interaction.RethrowEventError();
                Check(state.Read(0xf0u)==5u && state.Read(0xf4u)==0u && tutorials==1u && page_activity.GetLast()==layout.FrameAddress()+0x10cu && !model_records.empty(),"page entry selects first active role/recent category and real DLL hover reaches details/model");
                pal4::test::ClickNativeCeguiWindow(ui,state.Read(0x4cu));interaction.RethrowEventError();Check(state.Read(0xf4u)==1u,"actual right arrow switches to next active character");
                pal4::test::ClickNativeCeguiWindow(ui,state.Read(0x48u));interaction.RethrowEventError();Check(state.Read(0xf4u)==0u,"actual left arrow restores previous active character");
                unsigned wearable{},wearable_slot{},wearable_category{};
                for(unsigned type=3u;type<7u && !wearable;++type)for(unsigned n=0;n<inventory.EquipmentCount(type);++n){const auto item=inventory.EquipmentAt(type,n),required=inventory.ReadDword(item+0x168u);
                    if(required && required<=40u){wearable=item;wearable_slot=pal4::InventoryItemSlots::FindSlotOrClassify(empty_slots,item,inventory);
                        for(const auto& relation:db.Tables().at("EquipRelation").rows)if(std::get<unsigned>(relation[2])==type)wearable_category=std::get<unsigned>(relation[1]);break;}}
                Check(wearable && wearable_slot>0u && wearable_slot<6u,"page has an original wearable with an enabled unequip control");
                pal4::test::ClickNativeCeguiWindow(ui,state.Read(0x50u+wearable_category*4u));interaction.RethrowEventError();
                const auto wearable_sequence=inventory.ReadDword(wearable+400u);hover_sequence(wearable_sequence);interaction.RethrowEventError();
                std::array<unsigned,5> key{};const auto press=[&](unsigned code){key[4]=code;const auto member=page_activity.GetLast();
                    Check(interaction.OwnsActivity(member),"equipment key dispatch targets live page activity");const auto result=page_activity.DispatchKeyDown(member,reinterpret_cast<unsigned>(key.data()));interaction.RethrowEventError();return result;};
                Check(press(28u) && interaction.Menu().Visible(),"list Enter opens actual guarded equipment menu");
                pal4::test::ClickNativeCeguiWindow(ui,interaction.Menu().Button(0u));interaction.RethrowEventError();
                Check(detail_players.Word((0x920u+wearable_slot*4u)/4u)==wearable_sequence && !interaction.Menu().Visible(),"actual menu click routes layout command to persistent equipment mutation");
                Check(press(15u) && page_activity.GetLast()==layout.FrameAddress()+0x118u,"list Tab transfers to populated equipment slots");
                Check(press(28u) && !detail_players.Word((0x920u+wearable_slot*4u)/4u),"slot Enter uses actual DLL click to unequip through page routing");
                Check(press(1u) && page_activity.GetLast()==layout.FrameAddress()+0x28u,"first Esc leaves equipment details and returns to category activity");
                Check(press(1u) && page_activity.GetLast()==0xabcd && page_events.back()=="show_sys_tool_bar" && !model_states.back(),"second Esc hides page, releases activities/model and returns System toolbar command");
                const auto modal=[&]{return Export<bool (__thiscall*)(const void*)>(m,"?getModalState@Window@CEGUI@@QBE_NXZ")(Ptr(confirmation.RootAddress()));};
                interaction.SetVisible(true);
                Check(actions.ShowDiscard(wearable_sequence) && modal(),"equipment pending discard before page hide");
                interaction.SetVisible(false);
                Check(!modal() && inventory.FindInventoryEntry(inventory.Address()+0x7cu,wearable_sequence,7u),"page hide cancels owned discard without removing inventory");
                interaction.SetVisible(true);
                Check(actions.ShowDiscard(wearable_sequence),"equipment discard can reopen after hide cancellation");
                unsigned foreign_choice{};
                confirmation.Show(U"Foreign prompt",3u,[&](unsigned value){foreign_choice=value;},[](auto,auto){});
                interaction.SetVisible(false);
                Check(modal(),"page hide preserves foreign replacement dialog");
                pal4::test::ClickNativeCeguiWindow(ui,confirmation.Window("BtNo"));confirmation.RethrowEventError();
                Check(foreign_choice==2u && inventory.FindInventoryEntry(inventory.Address()+0x7cu,wearable_sequence,7u),"foreign callback remains live after equipment page hide");
                interaction.Close();interaction.Close();page_route={};
                Check(page_activity.Size()==1u && !interaction.OwnsActivity(layout.FrameAddress()+0x28u),"page reentry and retirement preserve parent and retire owned activity routing");page_activity.ReleaseStorage();
                for(unsigned c=0;c<4u;++c)detail_players.Word((c*0xb14u+0xb08u)/4u)=active_before[c];
                Check(ui.BindDialogRootParent(state.Read(36u),true),"restore component fixture after page exit");presentation.UpdateList(category,0u);
            }
            const auto all_sequences=[&]{std::set<unsigned> result;for(unsigned type=0;type<7u;++type)for(unsigned index=0,n=inventory.EquipmentCount(type);index<n;++index)
                    result.insert(inventory.ReadDword(inventory.EquipmentAt(type,index)+400u));return result;};
            const auto before_discard=all_sequences();
            Check(actions.ShowDiscard(sequence),"native equipment discard confirmation opens");
            pal4::test::ClickNativeCeguiWindow(ui,confirmation.Window("BtNo"));confirmation.RethrowEventError();
            Check(all_sequences()==before_discard && std::count(sounds.begin(),sounds.end(),"wa07.wav")==0,"actual No preserves every equipment instance");
            Check(actions.ShowDiscard(sequence),"native equipment discard confirmation reopens");
            pal4::test::ClickNativeCeguiWindow(ui,confirmation.Window("BtYes"));confirmation.RethrowEventError();
            const auto after_discard=all_sequences();std::vector<unsigned> removed;
            std::set_difference(before_discard.begin(),before_discard.end(),after_discard.begin(),after_discard.end(),std::back_inserter(removed));
            Check(removed.size()==1u && before_discard.size()==after_discard.size()+1u && std::count(sounds.begin(),sounds.end(),"wa07.wav")==1,"actual Yes removes exactly one current-hover instance");
            Check(!inventory.FindInventoryEntry(inventory.Address()+0x7cu,removed.front(),7u) && !inventory.RemoveEquipment(removed.front()),"removed equipment sequence no longer resolves or removes twice");
            Check(!after_discard.empty() && actions.ShowDiscard(*after_discard.begin()),"pending equipment prompt before retirement");
            confirmation.Show(U"Foreign prompt",4u,{},[](auto,auto){});actions.Close();actions.Close();
            Check(Export<bool (__thiscall*)(const void*)>(m,"?getModalState@Window@CEGUI@@QBE_NXZ")(Ptr(confirmation.RootAddress())),"closing equipment action owner preserves foreign replacement prompt");
            confirmation.Close();confirmation.ReleaseSelectionCallback();bool closed{};try{(void)actions.Equip(sequence);}catch(const std::logic_error&){closed=true;}Check(closed,"retired equipment action rejects calls");
            action_list.Close();
            inventory.Reset();const auto final_record=inventory.ProcessEquipment(inventory.Address()+0x7cu,definition_id);
            const auto final_sequence=inventory.ReadDword(final_record+400u);
            pal4::NativeEquipmentWindowList final_list(state,inventory,detail_players,db);unsigned final_displays{};
            pal4::NativeEquipmentActionServices final_endpoints;
            final_endpoints.sound=[](auto,auto){};
            final_endpoints.refresh_list=[&](auto category,auto character){(void)details.Clear();final_list.Populate(category,character);};
            final_endpoints.refresh_display=[&](auto record,auto){++final_displays;details.Update(record);};
            pal4::NativeEquipmentWindowActions final_actions(state,inventory,detail_players,db,refresh,confirmation,std::move(final_endpoints));
            final_list.Populate(5u,0u);Export<void (__thiscall*)(void*,int)>(m,"?setHoverRowIndex@MultiColumnList@CEGUI@@QAEXH@Z")(list,0);
            const auto final_hover=Export<void* (__thiscall*)(const void*)>(m,"?getHoverItem@MultiColumnList@CEGUI@@QBEPAVListboxItem@2@XZ")(list);
            std::cout<<"Final equipment before confirmation rows="<<count()<<" hover="<<final_hover<<" sequence="<<final_sequence<<'\n';
            Check(final_actions.ShowDiscard(final_sequence),"final equipment instance confirmation");
            const auto multi_before=Export<bool (__thiscall*)(const void*)>(m,"?wantsMultiClickEvents@Window@CEGUI@@QBE_NXZ")(Ptr(confirmation.Window("BtYes")));
            std::cout<<"Final dialog Yes="<<Ptr(confirmation.Window("BtYes"))<<" No="<<Ptr(confirmation.Window("BtNo"))
                <<" capture="<<Export<void* (__cdecl*)()>(m,"?getCaptureWindow@Window@CEGUI@@SAPAV12@XZ")()
                <<" modal="<<Export<void* (__thiscall*)(const void*)>(m,"?getModalTarget@System@CEGUI@@QBEPAVWindow@2@XZ")(Ptr(ui.SystemAddress()))
                <<" active="<<Export<bool (__thiscall*)(const void*)>(m,"?isActive@Window@CEGUI@@QBE_NXZ")(Ptr(confirmation.RootAddress()))
                <<" disabled="<<disabled(confirmation.Window("BtYes"))<<'\n';
            pal4::test::ClickNativeCeguiWindow(ui,confirmation.Window("BtYes"));confirmation.RethrowEventError();
            std::cout<<"Final equipment after confirmation rows="<<count()<<" recent="<<inventory.RecentEquipmentSequences().size()<<" record="<<inventory.FindInventoryEntry(inventory.Address()+0x7cu,final_sequence,7u)<<" displays="<<final_displays<<" choice="<<confirmation.LastSelection()<<" hover="<<Export<void* (__thiscall*)(const void*)>(m,"?getHoverItem@MultiColumnList@CEGUI@@QBEPAVListboxItem@2@XZ")(list)<<'\n';
            Check(confirmation.LastSelection()==1u,"final instance mouse selection confirms Yes");
            Check(Export<bool (__thiscall*)(const void*)>(m,"?wantsMultiClickEvents@Window@CEGUI@@QBE_NXZ")(Ptr(confirmation.Window("BtYes")))==multi_before,
                "native single-click helper restores original target multi-click mode");
            Check(!count() && inventory.RecentEquipmentSequences().empty() && !inventory.FindInventoryEntry(inventory.Address()+0x7cu,final_sequence,7u) && !final_displays,
                "last-instance discard succeeds with empty list even when original callback result is false");
            final_actions.Close();final_list.Close();
            std::cout<<"PASS: native Equip/Unequip actual slot/stats/sounds/hover, DLL No/Yes single-instance discard, repeat removal rejection and replacement-safe Close\n";
        }
        std::cout<<"PASS: native equipment details="<<descriptions<<" eligible="<<previews<<" locked="<<ineligible<<" exact real stat bonuses, all84words/six slots restored; GPU pixels="<<detail_pixels<<'\n';
    }
    inventory.Reset();page.Populate(5u,0u);character_display.UpdateEquipment(0u);
    Check(count()==0u && inventory.RecentEquipmentSequences().empty(),"inventory reset leaves no stale row/name/recent views");
    for(unsigned i=0;i<6u;++i)Check(window_text(state.Read(0x98u+i*4u)).empty() && disabled(state.Read(0x98u+i*4u)),"inventory reset clears all character equipment labels");
    const auto fresh=inventory.ProcessEquipment(inventory.Address()+0x7cu,std::get<unsigned>(db.Tables().at("Equip").rows.front()[0]));
    page.Populate(5u,0u);Check(fresh && count()==1u && page.OwnedItemCount()==2u,"fresh equipment acquisition repopulates after inventory reset");
    Check(Export<unsigned (__thiscall*)(const void*)>(m,"?getID@ListboxItem@CEGUI@@QBEIXZ")(Ptr(cell(0u,0u)))==inventory.ReadDword(fresh+400u),"post-reset row uses new inventory instance");
    bool rejected{};try{page.Populate(6u,0u);}catch(const std::out_of_range&){rejected=true;}Check(rejected,"invalid equipment category rejected");
    page.Close();page.Close();Check(count()==0u,"equipment rows detached before wrapper cleanup");
    inventory.Close();Check(inventory.OutstandingAllocations()==0u,"equipment inventory releases every record/text/slot allocation after rows retire");
    rejected=false;try{page.Populate(0u,0u);}catch(const std::logic_error&){rejected=true;}Check(rejected,"closed equipment list rejects population");
    Check(ui.BindDialogRootParent(state.Read(36u),false),"equipment detach");
    layout.Close();layout.Close();Check(!state.Read(36u) && !state.Read(0x30u) && !state.Read(0x120u),"native shared destructor retires layout and navigation buffers");
    bool retired_layout{};try{layout.DispatchCommand("use_equipment",1u);}catch(const std::logic_error&){retired_layout=true;}Check(retired_layout,"retired layout rejects command dispatch");
    {pal4::NativeEquipmentWindowLayout reopened(ui,0x1000u,[](auto,auto,auto){return true;});Check(reopened.State().Read(0x40u)!=0u,"same original XML can reopen after subscriber/layout teardown");reopened.Close();}
    std::cout<<"PASS: native equipment all original records twice, four characters/six categories, instance IDs, inactive ownership, disabled columns, GPU pixels="<<pixels<<", badge pixels="<<badge_pixels<<", cleanup\n";
    std::cout<<"PASS: native character equipment userData/sequence/Chinese names, four nameplates GPU pixels="<<nameplate_pixels<<", sentinel/direct/missing/null-name/reset branches\n";
}
}
int main(int argc,char** argv)try{std::cout<<std::unitbuf;Check(argc==2,"provide original game root");
    if(const auto crt=GetModuleHandleA("msvcrt.dll"))if(const auto mode=reinterpret_cast<int (__cdecl*)(int)>(GetProcAddress(crt,"_set_error_mode")))(void)mode(2);
    SetUnhandledExceptionFilter([](EXCEPTION_POINTERS* e)->LONG{
        HMODULE module{};char path[MAX_PATH]{};const auto address=e->ExceptionRecord->ExceptionAddress;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<const char*>(address),&module);GetModuleFileNameA(module,path,MAX_PATH);
        std::cerr<<"Equipment exception "<<std::hex<<e->ExceptionRecord->ExceptionCode<<" at "<<address<<" module="<<path
            <<" offset="<<(reinterpret_cast<std::uintptr_t>(address)-reinterpret_cast<std::uintptr_t>(module))<<'\n';
        std::cerr<<"read/write="<<e->ExceptionRecord->ExceptionInformation[0]<<" address="<<e->ExceptionRecord->ExceptionInformation[1]
            <<" eax="<<e->ContextRecord->Eax<<" ecx="<<e->ContextRecord->Ecx<<" stack:";
        const auto* stack=reinterpret_cast<const unsigned*>(e->ContextRecord->Esp);
        for(unsigned i=0;i<12u;++i)std::cerr<<' '<<stack[i];std::cerr<<'\n';
        return EXCEPTION_EXECUTE_HANDLER;});
    const pal4::test::NativeDesktopObservation desktop;Run(argv[1]);desktop.Check("native equipment list");return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
