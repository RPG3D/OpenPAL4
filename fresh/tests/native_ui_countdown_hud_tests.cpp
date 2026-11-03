#include "support/native_cegui_frame_session.h"
#include "support/native_desktop_observation.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/native_game_timer_queue.h"
#include "pal4/native_ui_countdown.h"
#include "pal4/native_ui_countdown_script.h"
#include "pal4/native_timed_script_stack.h"
#include "pal4/native_angelscript_engine.h"
#include "pal4/native_minimap_scene_ui_update.h"
#include "pal4/native_game_database.h"
#include "pal4/native_scene_record_catalog.h"
#include "pal4/native_game_arena_manager_owner.h"
#include "pal4/native_minimap_manager_owner.h"
#include "pal4/native_scene_gob_assets.h"
#include "pal4/native_pal_game_iv_texture_storage.h"
#include "pal4/native_script_frame_runner.h"
#include "pal4/native_new_game_script_session.h"
#include "../native/ui/cegui_exports_native.h"
#include <rtfsyst.h>
#include <array>
#include <iostream>
#include <stdexcept>

namespace {
void Require(bool value,const std::string& message){if(!value)throw std::runtime_error(message);}
struct Filesystem {
    bool ready{};
    ~Filesystem(){if(ready)RtFSManagerClose();}
};
void Run(const std::filesystem::path& root){
    pal4::test::NativeFrameSession frame;
    Require(frame.Start(800,600),"countdown SDK frame startup");
    Require(!IsWindowVisible(frame.window),"countdown test window must stay hidden");
    Filesystem filesystem;filesystem.ready=RtFSManagerOpen(8)!=FALSE;
    Require(filesystem.ready,"countdown filesystem manager startup");
    pal4::NativeCeguiUiSession ui;
    Require(ui.Start(root,800,600),ui.LastError());
    Require(ui.LoadImageset(root,"gamedata\\ui\\fonts\\fontPuzzle_0.imageset"),ui.LastError());
    Require(ui.InitializeWidgetRendering(root),ui.LastError());
    for(const auto* image:{"biaoti_0.imageset","biaoti2_0.imageset","biaoti3_0.imageset","gy_0.imageset","OiramLook.imageset"}){
        const auto name=std::string("gamedata\\ui\\imagesets\\")+image;
        Require(ui.LoadImageset(root,name.c_str()),ui.LastError());
    }
    Require(ui.LoadOriginalMainLayout(),ui.LastError());
    Require(ui.HideMainLayout(),ui.LastError());
    Require(ui.LoadOriginalMinimapLayout(root),ui.LastError());
    Require(ui.LoadOriginalSceneHudLayout(),ui.LastError());
    const auto timer=ui.SceneHudTimerWindowAddress();
    Require(timer!=0u,"original portrait/StaticTimer resolved");
    const auto module=GetModuleHandleA("CEGUIBase.dll");
    pal4::detail::Export<void (__thiscall*)(void*,bool)>(module,
        "?setVisible@Window@CEGUI@@QAEX_N@Z")(reinterpret_cast<void*>(ui.SceneHudRootAddress()),true);
    Require(pal4::detail::Export<bool (__thiscall*)(const void*,bool)>(module,
        "?isVisible@Window@CEGUI@@QBE_N_N@Z")(reinterpret_cast<const void*>(timer),false),
        "StaticTimer inherits visible HUD state");
    const auto render=[&]{
        Require(frame.Begin(),"begin countdown HUD frame");
        Require(ui.Render(),"render original countdown HUD");
        auto pixels=frame.ReadPixels();
        Require(pixels.size()==800u*600u*4u,"read actual countdown HUD raster");
        return pixels;
    };
    Require(ui.SetSceneHudTimerText(""),ui.LastError());
    const auto empty=render();
    // Combat interleaves game overlays with the GUI and explicitly discards
    // the shared renderer list. Unchanged GUI frames must still reach the GPU.
    ui.SetOverlayQueueEnabled(true);
    for(unsigned repeat=0;repeat<3u;++repeat){
        ui.ClearQueuedOverlays();
        const auto rebuilt=render();
        Require(rebuilt==empty,"discarded overlay queue must regenerate unchanged original HUD pixels");
    }
    pal4::NativeAngelScriptEngine engine;
    unsigned loads{};
    pal4::NativeTimedScriptStack scripts(engine,{},[&](std::string_view source,std::string_view name){
        Require(source=="Q01.csb" && name=="PFNTimerScript[TimerScript]","HUD expiry module routing");
        ++loads;
        const std::string text="void pulse(){ global_mvar+=1; }",module_name(name);
        Require(engine.Raw()->AddScriptSection(module_name.c_str(),"fixture",text.c_str(),static_cast<int>(text.size()))>=0,
            "add countdown SDK callback source");
        Require(engine.Raw()->Build(module_name.c_str())>=0,"compile actual countdown SDK callback");
    });
    pal4::NativeGameDatabase database;Require(database.Load(root),"load original countdown HUD scene catalog");
    pal4::NativeSceneRecordCatalog scenes(database);
    pal4::NativeGameArenaManagerOwner arena;(void)arena.Construct();
    pal4::NativeMinimapManagerOwner minimap;
    Require(minimap.Initialize(root),"initialize original minimap owner");
    Require(minimap.LoadScene(root,"Q01","N01"),"load initial original minimap");
    pal4::NativeSceneGobAssets objects;
    auto hud=std::make_unique<pal4::NativeMinimapSceneUiUpdate>(root,arena,minimap,ui,scenes,objects,800,600);
    const auto owner=hud->ActiveComponentAddress()-0x28u;
    pal4::NativeUiCountdown* countdown{};
    pal4::NativeGameTimerQueue timers([&](auto context,auto method){
        Require(hud && countdown && method==0x47caa0u,"HUD countdown callback identity");
        hud->TickCountdown(context);
    });
    std::array<std::uint32_t,0x224u/4u> manager{};manager[0x1a0u/4u]=timers.Construct();
    const auto manager_address=reinterpret_cast<std::uint32_t>(manager.data());
    unsigned handler_cleanups{};
    {
        auto& control=hud->BindCountdown(manager_address,timers,[&]{
            // This isolated HUD has no displaced scene/story wait owners.
            scripts.DispatchUiScript({},[&]{++handler_cleanups;},[&](auto& stack){
                hud->ExecuteCountdownScript({manager_address,arena.Address(),timers,stack,"Q01.csb",{}});
            });
        });
        Require(&hud->BindCountdown(manager_address,timers,[]{})==&control,"same HUD binding retains countdown owner");
        countdown=&control;
        control.Start(123u,"pulse");
        bool wrong_timer_owner{};
        try{hud->TickCountdown(owner+4u);}catch(const std::invalid_argument&){wrong_timer_owner=true;}
        Require(wrong_timer_owner,"timer dispatch rejects foreign HUD owner before ticking");
        Require(hud->TimerFrameDuration(manager_address)==122u && ui.SceneHudTimerText()=="123","real DLL initial countdown text and actual save getter share state");
        bool foreign_rejected{};
        try{(void)hud->BindCountdown(manager_address+4u,timers,[]{});}
        catch(const std::invalid_argument&){foreign_rejected=true;}
        Require(foreign_rejected && hud->TimerFrameDuration(manager_address)==122u &&
            ui.SceneHudTimerText()=="123","foreign manager rebind rejects before changing live HUD");
        const auto visible=render();
        std::size_t changed{};
        for(std::size_t i=0;i<visible.size();i+=4u)
            if(std::memcmp(visible.data()+i,empty.data()+i,3u)!=0)++changed;
        Require(changed!=0u,"countdown text must change actual RGB pixels");
        (void)control.Pause();timers.Update(4.0f);
        Require(hud->TimerFrameDuration(manager_address)==122u && ui.SceneHudTimerText()=="123","actual HUD pause preserves duration/text");
        {
            pal4::NativePalGameIVTextureStorage game([](auto dictionary){
                (void)RwTexDictionaryDestroy(reinterpret_cast<RwTexDictionary*>(dictionary));
            });
            (void)game.Construct();
            game.SetGameDataEntry(69u,17u);game.SetGameDataEntry(70u,321u);game.SetGameDataEntry(71u,19u);
            pal4::NativeAngelScriptEngine query_engine;
            pal4::NativeGameScriptServices query_services{};
            pal4::NativeGameScriptCallbacks callbacks(query_services);
            query_engine.SetCallbacks(&callbacks);
            constexpr char query[]="void query(){ global_mvar=giUiTimerGetSaveData(); }";
            Require(query_engine.Raw()->AddScriptSection("saved_timer","fixture",query,sizeof(query)-1)>=0 &&
                query_engine.Raw()->Build("saved_timer")>=0,"compile actual saved timer query");
            {
                pal4::NativeScriptFrameRunner runner(query_engine);runner.Queue("saved_timer","query");
                Require(runner.Tick(0.0f)==pal4::NativeScriptFrameStatus::failed &&
                    runner.Error()=="PAL4 game script service -236 is unavailable","unbound saved timer query explicitly fails");
            }
            query_services.ui_timer_save_data=&game;callbacks.SetServices(query_services);
            for(const auto saved:{321u,0xabcdef01u,0u}){
                game.SetGameDataEntry(70u,saved);
                pal4::NativeScriptFrameRunner runner(query_engine);runner.Queue("saved_timer","query");
                Require(runner.Tick(0.0f)==pal4::NativeScriptFrameStatus::finished &&
                    static_cast<std::uint32_t>(query_engine.GlobalStoryVariable())==saved &&
                    game.GameDataEntry(69u)==17u && game.GameDataEntry(71u)==19u &&
                    hud->TimerFrameDuration(manager_address)==122u && ui.SceneHudTimerText()=="123",
                    "SDK query reads current saved entry70 independently of live HUD countdown without changing neighbors");
            }
            bool foreign_game{};
            try{(void)game.GameDataEntry(game.Address()+4u,70u);}
            catch(const std::out_of_range&){foreign_game=true;}
            Require(foreign_game,"saved timer adapter rejects foreign game owner");
            query_engine.SetCallbacks(nullptr);
            game.Destroy();
            bool retired_game{};try{(void)game.Game();}catch(const std::logic_error&){retired_game=true;}
            Require(retired_game,"saved timer adapter rejects retired game owner");
            std::cout<<"Saved timer SDK query: real game entry70, live HUD independence, missing/foreign/retired services verified\n";
        }
        const auto flags=hud->VisibilityFlags();
        Require(minimap.LoadScene(root,"Q01","Q01") &&
            minimap.LoadScene(root,"Q01","N01"),"replace original minimap resources and restore opening scene");
        Require(hud->ActiveComponentAddress()-0x28u==owner &&
            &hud->BindCountdown(manager_address,timers,[]{})==&control &&
            hud->VisibilityFlags()==flags && hud->TimerFrameDuration(manager_address)==122u &&
            ui.SceneHudTimerText()=="123","minimap resource replacement retains HUD identity, flags and paused countdown");
        control.Resume();timers.Update(1.001f);
        Require(hud->TimerFrameDuration(manager_address)==121u && ui.SceneHudTimerText()=="122","actual HUD resumed countdown text");
        control.Clear();timers.Update(0.001f);
        Require(ui.SceneHudTimerText().empty() && hud->TimerFrameDuration(manager_address)==0u,"actual HUD clear removes text and saved duration");
        const auto cleared=render();
        Require(cleared==empty,"cleared countdown restores original raster pixels");
        control.Start(0u,"pulse");
        Require(ui.SceneHudTimerText().empty() && scripts.Active() && engine.GlobalStoryVariable()==0,
            "real HUD zero start clears text and defers script execution");
        (void)scripts.Process(0.0f,false,false,"Q01");
        Require(!scripts.Active() && loads==1u && handler_cleanups==1u && engine.GlobalStoryVariable()==1,
            "actual HUD expiry dispatch cleans handlers once and executes through SDK stack");
        std::cout<<"Original StaticTimer: initial/resumed/cleared DLL text, "<<changed
            <<" changed GPU pixels, exact clear restoration, original minimap round trip and deferred SDK expiry verified\n";
        control.Start(5u,"pulse");
    }
    const auto before_retire=loads;
    hud.reset();countdown=nullptr;timers.Update(2.0f);
    Require(loads==before_retire && !timers.Count(),"actual HUD retirement removes active countdown owner borrow");
    {
        // Use the production session overload, original arena source and CPK.
        // The parent is generated SDK code; the child is original Q01 bytecode.
        pal4::NativeNewGameScriptSession session(root,{});
        char world[]="Q01";
        auto* arena_words=reinterpret_cast<std::uint32_t*>(arena.Address());
        arena_words[2]=reinterpret_cast<std::uint32_t>(world);arena.SetSceneName("N01");
        (void)arena.QueueSceneScript([](const auto&){return false;});
        hud=std::make_unique<pal4::NativeMinimapSceneUiUpdate>(root,arena,minimap,ui,scenes,objects,800,600);
        unsigned retired_handlers{};
        pal4::NativeGameScriptServices services{};
        auto& control=hud->BindCountdown(manager_address,timers,session,[&]{return services;},[&]{++retired_handlers;});
        countdown=&control;services.ui_countdown=&control;session.BindServices(services);
        Require(&hud->BindCountdown(manager_address,timers,session,[&]{return services;},[]{})==&control,
            "production session rebind preserves the original controller and endpoints");
        {
            pal4::NativeNewGameScriptSession foreign(root,{});bool rejected{};
            try{(void)hud->BindCountdown(manager_address,timers,foreign,[&]{return services;},[]{});}
            catch(const std::invalid_argument&){rejected=true;}
            Require(rejected,"production countdown rejects a different SDK session");
        }
        Require(session.QueueConcurrentSceneInit("Q01","N01"),"prepare actual scene initializer below countdown child");
        auto* sdk=session.Context().GetEngine();
        constexpr char parent[]="void parent(){ global_mvar=1; giStartUiTimer(0,\"Q01_N01_init\"); global_mvar+=100; }";
        Require(sdk->AddScriptSection("countdown_parent","fixture",parent,sizeof(parent)-1)>=0 &&
            sdk->Build("countdown_parent")>=0,"compile active parent for production HUD binding");
        Require(session.Context().Prepare(sdk->GetFunctionIDByName("countdown_parent","parent"))>=0 &&
            session.Context().Execute()==asEXECUTION_SUSPENDED && session.GlobalStoryVariable()==1 &&
            session.TimedScriptsActive() && session.TimedScriptsStarted()==1u && !retired_handlers &&
            ui.SceneHudTimerText().empty(),
            "actual SDK start preempts its parent and prepares original arena callback without executing or clearing active handlers");
        Require(session.TickConcurrentSceneUntilBoundary(0.0f,nullptr,[]{return false;})==pal4::NativeScriptFrameStatus::queued &&
            session.GlobalStoryVariable()==1,"queued scene initializer yields to selected countdown child");
        session.ClearExecutionStack([]{});
        Require(retired_handlers==1u && !session.TimedScriptsActive() && session.GlobalStoryVariable()==1,
            "production binding drains copied cleanup endpoint after SDK return before retiring prepared contexts");
        control.Clear();timers.Update(0.001f);hud.reset();countdown=nullptr;arena_words[2]=0u;
        std::cout<<"Production HUD countdown: actual session/arena/CPK preparation, SDK parent suspension, scene priority and deferred cleanup verified\n";
    }
    timers.Destroy();
    Require(!timers.OutstandingAllocations(),"HUD countdown queue releases all allocations");
    ui.Stop();Require(!ui.SceneHudTimerWindowAddress(),"HUD stop clears borrowed StaticTimer pointer");
}
}
int main(int argc,char** argv){
    try{
        Require(argc==2,"provide original PAL4 directory");
        pal4::test::NativeDesktopObservation desktop;
        Run(argv[1]);desktop.Check("original HUD countdown preserves desktop focus and cursor");return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
