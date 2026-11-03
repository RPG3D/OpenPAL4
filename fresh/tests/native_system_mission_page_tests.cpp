#include "support/native_no_arena_operations.h"
#include "support/native_cegui_frame_session.h"
#include "support/native_desktop_observation.h"
#include "pal4/native_system_menu_pages.h"
#include "pal4/native_system_toolbar_actions.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/native_cegui_window_pointer_events.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/native_game_database.h"
#include "pal4/native_pal_game_iv_texture_storage.h"
#include "pal4/native_pal_game_iv_state_list.h"
#include "pal4/native_game_arena_manager_owner.h"
#include "pal4/native_world_loading_state_owner.h"
#include "pal4/native_camera_manager_owner.h"
#include "pal4/native_arena_load_runtime.h"
#include "pal4/native_inventory_session.h"
#include "pal4/player_manager_object.h"
#include "pal4/input_manager.h"
#include "pal4/game_render_system.h"
#include "../native/ui/cegui_exports_native.h"
#include <rtfsyst.h>
#include <array>
#include <iostream>
#include <stdexcept>

namespace {
void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
// This UI integration fixture never loads/transitions an arena. Reject every
// host call so an unintended state operation cannot silently pass the test.
using NoArenaOperations=pal4::test::NoArenaOperations;
struct Filesystem {bool ready{};~Filesystem(){if(ready)RtFSManagerClose();}};
void Run(const std::filesystem::path& root){
    std::cout<<"stage=frame\n";
    pal4::test::NativeFrameSession frame;
    Check(frame.Start(800,600),"System Mission GPU startup");
    Check(!IsWindowVisible(frame.window),"System Mission test stays hidden");
    Filesystem filesystem;filesystem.ready=RtFSManagerOpen(8)!=FALSE;Check(filesystem.ready,"filesystem startup");
    pal4::NativeCeguiUiSession ui;Check(ui.Start(root,800,600),"CEGUI startup");
    std::cout<<"stage=fonts\n";
    Check(ui.LoadImageset(root,"gamedata\\ui\\fonts\\fontPuzzle_0.imageset"),"puzzle font");
    Check(ui.InitializeWidgetRendering(root),"widget renderer");
    for(const auto* set:{"OiramLook","zhujiemian_0","biaoti_0","biaoti2_0","biaoti3_0"}){
        const auto path=std::string("gamedata\\ui\\imagesets\\")+set+".imageset";
        Check(ui.LoadImageset(root,path.c_str()),"original System startup imageset");
    }
    Check(ui.LoadOriginalMainLayout(),"main fonts/layout");Check(ui.HideMainLayout(),"hide startup menu");
    pal4::NativeGameDatabase database;Check(database.Load(root),"original database");
    std::cout<<"stage=game\n";
    pal4::NativePalGameIVTextureStorage game([](std::uint32_t){});Check(game.Construct()!=0u,"PALGameIV");
    game.SetGameDataEntry(13,1u);
    std::cout<<"stage=arena\n";
    for(const auto story:game.StoryObjects())*reinterpret_cast<std::int32_t*>(story)=INT32_MAX;
    *reinterpret_cast<std::int32_t*>(game.GlobalAddress(0x90dec0u))=INT32_MAX;
    pal4::NativeGameArenaManagerOwner arena;Check(arena.Construct()!=0u,"arena owner");
    pal4::NativeWorldLoadingStateOwner loading;Check(loading.Construct()!=0u,"loading owner");
    pal4::NativePalGameIVStateList states(game.Address());
    Check(states.Register(loading.Address(),6u,[&]{loading.Destroy();}),"loading registration");
    pal4::NativeCameraManagerOwner cameras;
    pal4::GameRenderState render_state(0x8e31e0u);
    NoArenaOperations host;
    pal4::NativeArenaLoadRuntime transitions(arena,game,states,loading,cameras,render_state,host);
    pal4::PlayerManagerObjectState players;
    pal4::InputManager keys;(void)keys.InitializeState();
    pal4::NativeInventorySession inventory;Check(inventory.Start()!=0u,"inventory owner");
    pal4::NativeSystemMenuPages* pages{};
    pal4::NativeEventHandlerArray activity(
        [&](auto member,auto slot){Check(pages && pages->OwnsActivity(member),"System callback owner");return pages->Callback(member,slot);},
        [&](auto member,auto slot,auto event){Check(pages && pages->OwnsActivity(member),"System key owner");return pages->Predicate(member,slot,event);});
    unsigned tutorials{};
    pal4::NativeSystemMenuPageServices services;
    services.tutorial=[&](auto id){Check(id==6u,"Mission tutorial6");++tutorials;};
    services.decorations.money=[&]{return inventory.UpdateCounter(0);};
    services.decorations.game_string=[&](auto id){const auto value=database.FindTableTextById("GameString",id);
        Check(value.has_value(),"GameInfo original text");return std::string(*value);};
    services.decorations.elapsed_seconds=[]{return 0u;};
    services.decorations.clock_offset_seconds=[]{return 0u;};
    std::array<std::uint32_t,0x224u/4u> manager_image{};
    const auto manager=reinterpret_cast<std::uint32_t>(manager_image.data());
    pal4::NativeSystemMenuPages owner(ui,root,transitions,game,states,players,database,activity,keys,inventory,manager,std::move(services));
    std::cout<<"stage=pages\n";
    pages=&owner;
    owner.Event(0x8a8548u,0u,0u);Check(!owner.Mission(),"hiding absent Mission does not create it");
    owner.Event(0x8a80acu,1u,0u);
    std::cout<<"stage=toolbar\n";
    const auto toolbar=owner.Toolbar().ActiveComponentAddress();
    Check(activity.GetLast()==toolbar,"real toolbar becomes active");
    const auto module=GetModuleHandleA("CEGUIBase.dll");
    const auto key=[&](std::uint32_t code){std::array<std::uint32_t,5> event{};event[4]=code;
        const auto address=reinterpret_cast<std::uint32_t>(event.data());
        const auto result=activity.Contains(address)?1u:activity.DispatchKeyDown(activity.GetLast(),address);
        owner.RethrowError();return result;};
    const auto visible=[&](std::uint32_t w){return pal4::detail::Export<bool (__thiscall*)(const void*,bool)>(module,
        "?isVisible@Window@CEGUI@@QBE_N_N@Z")(reinterpret_cast<const void*>(w),true);};
    const auto row_count=[&](std::uint32_t w){return pal4::detail::Export<std::uint32_t (__thiscall*)(const void*)>(module,
        "?getRowCount@MultiColumnList@CEGUI@@QBEIXZ")(reinterpret_cast<const void*>(w));};
    std::uint32_t retained{};
    for(unsigned cycle=0;cycle<2u;++cycle){
        std::cout<<"stage=task cycle="<<cycle<<'\n';
        (void)pal4::NativeCeguiWindowPointerEvents(module).HandleMouseAtCenter(ui.SystemToolbarButtons()[4]);
        owner.RethrowError();Check(owner.Mission()!=nullptr,"actual DLL Task button constructs production page factory");
        const auto address=owner.Mission()->FrameAddress();
        Check(!retained || retained==address,"System Task factory reuses persistent owner");retained=address;
        const auto& mission=owner.Mission()->State();
        Check(activity.Size()==3u && activity.GetLast()==address+0x40u && row_count(mission.Read(0x54u))==28u,
            "factory routes task input and real database rows");
        Check(manager_image[0x144u/4u]==12u,"Mission sets original help context12 in live manager");
        Check(owner.Decorations().ToolbarFrame() && owner.Decorations().InfoFrame(),"original decorations are loaded");
        Check(frame.Begin() && ui.Render(),"factory Task frame renders");
        Check(key(3u)==1u && mission.bytes[0x88u]==0u && row_count(mission.Read(0x58u))==128u,
            "global shortcut reaches Mission through production System predicate router");
        Check(key(28u)==1u && activity.GetLast()==address+0x48u,"production router enters chapter body");
        Check(key(1u)==1u && key(1u)==1u && activity.GetLast()==address+0x28u,"production router unwinds body and title");
        Check(key(1u)==1u && activity.Size()==1u && activity.GetLast()==toolbar &&
            *reinterpret_cast<const std::uint32_t*>(mission.Read(0x24u)+68u)==0u,
            "Mission Escape restores real toolbar through recursive production event routing");
        Check(manager_image[0x144u/4u]==5u,"toolbar return restores help context5");
        Check(visible(ui.SystemToolbarRootAddress()),"toolbar remains visible after Task return");
        owner.Decorations().UpdateClock();owner.RethrowError();
    }
    Check(tutorials==2u,"reopening Task invokes original tutorial endpoint");
    owner.Close();owner.Close();Check(activity.Size()==0u,"System pages close removes all activity");
    std::cout<<"PASS: production System Task factory, DLL toolbar click, decorations, global input, persistent reopen and cleanup\n";
}
}
int main(int argc,char** argv)try{
    std::cout<<std::unitbuf;Check(argc==2,"provide original game directory");
    pal4::test::NativeDesktopObservation desktop;Run(argv[1]);desktop.Check("System Mission page");return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
