#include "pal4/game_arena_load_handler.h"
#include "pal4/native_archive_load_request.h"
#include "pal4/game_render_system.h"
#include "pal4/native_arena_load_runtime.h"
#include "pal4/native_camera_manager_owner.h"
#include "pal4/native_game_arena_manager_owner.h"
#include "pal4/native_pal_game_iv_state_list.h"
#include "pal4/native_pal_game_iv_texture_storage.h"
#include "pal4/native_world_loading_state_owner.h"
#include "pal4/native_world_state_owner.h"
#include "pal4/native_system_menu_state_owner.h"
#include "pal4/system_menu_state.h"
#include "pal4/native_smith_state_owner.h"
#include "pal4/native_smith_state_runtime.h"
#include "pal4/native_script_system_ui_runtime.h"
#include "pal4/native_trade_state_owner.h"
#include "pal4/native_trade_system_owner.h"
#include "pal4/native_trade_state_runtime.h"
#include "pal4/trade_system_ui.h"
#include "pal4/native_trade_system_ui_runtime.h"
#include "pal4/native_shop_record_catalog.h"
#include "pal4/native_game_database.h"
#include "pal4/background_music_player.h"
#include "pal4/native_game_over_state.h"
#include "pal4/native_logo_state_dispatch.h"
#include "pal4/native_world_map_state.h"
#include <array>
#include <optional>

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Host final : pal4::INativeArenaLoadHost {
    std::vector<std::string> calls;
    bool visible = false;
    bool fail_scene_runtime = false;
    std::uint32_t scene_previous_id = 0u;
    std::uint32_t scene_argument = 1u;

    std::uint32_t ActorControlAddress() override { return 0x1000u; }
    std::uint32_t InputManagerAddress() override { return 0x2000u; }
    std::uint32_t UiFrameManagerAddress() override { return 0x3000u; }
    std::uint32_t Milliseconds() override { return 0x12345678u; }
    void ProcessActorInput(bool clear_vector) override {
        calls.emplace_back(clear_vector ? "actor-clear" : "actor");
    }
    void InitializeInputState() override {
        calls.emplace_back("input");
    }
    void ActivateDefaultCamera() override {
        calls.emplace_back("camera");
    }
    void ShowLoadingWindow(bool show) override {
        visible = show;
        calls.emplace_back(show ? "show" : "hide");
    }
    void SetDefaultCursor() override {
        calls.emplace_back("cursor");
    }
    void LogStateExchange(std::string_view previous,
                          std::string_view next,
                          std::uint32_t source_line) override {
        Check(((previous == "PALIV_LOADING" &&
                (next == "PALIV_LOADING" || next == "PALIV_SCENE")) ||
               (previous == "PALIV_SCENE" && (next == "PALIV_LOADING" || next == "PALIV_SYSTEM" || next == "PALIV_TRADE" || next == "PALIV_FOUNDRY" || next == "PALIV_BINK" || next == "PALIV_WORLD_MAP" || next == "PALIV_COMBAT")) ||
               (previous == "PALIV_WORLD_MAP" && (next == "PALIV_SCENE" || next == "PALIV_LOADING")) ||
               (previous == "PALIV_FOUNDRY" && next == "PALIV_TRADE") ||
               (previous == "PALIV_TRADE" && (next == "PALIV_SCENE" || next == "PALIV_FOUNDRY" || next == "PALIV_TRADE")) ||
               (previous == "PALIV_BINK" && (next == "PALIV_SCENE" || next == "PALIV_BINK")) ||
               (previous == "PALIV_COMBAT" && next == "PALIV_GAME_OVER") ||
               (previous == "PALIV_GAME_OVER" && next == "PALIV_LOGO") ||
               (previous == "PALIV_LOGO" && next == "PALIV_LOADING") ||
               ((previous == "PALIV_SYSTEM" || previous == "PALIV_FOUNDRY" || previous == "PALIV_COMBAT") && next == "PALIV_SCENE")) &&
                  source_line == 0x1a1u,
              "Loading exchange names");
        calls.emplace_back("exchange");
    }
    std::uint32_t RenderLoadingUi(std::uint32_t frame) override {
        Check(frame == 0x5000u, "Loading UI frame");
        calls.emplace_back("render-loading");
        return 0x99u;
    }
    void LoadSave(std::uint32_t) override {
        throw std::logic_error("save restore is not bound");
    }
    void LoadSceneResources(std::string_view world,
                            std::string_view area) override {
        Check(world == "Q01" && area == "N02", "Loading scene names");
        calls.emplace_back("scene-resources");
    }
    void LoadSceneRuntime(std::string_view resources) override {
        Check(resources.empty(), "Loading optional resources");
        calls.emplace_back("scene-runtime");
        if (fail_scene_runtime)
            throw std::runtime_error("scene runtime is not ready");
    }
    void InitializePlayerScene() override {
        calls.emplace_back("player-scene");
    }
    void LeaveSceneState(std::uint32_t next_id) override {
        Check(next_id == 3u || next_id == 2u || next_id == 4u || next_id == 5u || next_id == 6u || next_id == 11u || next_id == 12u, "Scene leaves for a bound native state");
        calls.emplace_back("scene-leave");
    }
    bool EnterSaveState(std::uint32_t, std::uint32_t) override { throw std::logic_error("unexpected Save entry in Loading fixture"); }
    void LeaveSaveState(std::uint32_t) override { throw std::logic_error("unexpected Save leave in Loading fixture"); }
    void ProcessSaveWindowEvents() override { throw std::logic_error("unexpected Save events in Loading fixture"); }
    std::uint32_t SaveFrameAddress() override { throw std::logic_error("unexpected Save frame in Loading fixture"); }
    bool SaveFrameVisible(std::uint32_t) override { throw std::logic_error("unexpected Save visibility in Loading fixture"); }
    bool EnterSceneState(std::uint32_t previous_id,
                         std::uint32_t argument) override {
        scene_previous_id = previous_id;
        scene_argument = argument;
        calls.emplace_back("scene-entry");
        return true;
    }
};

// State-dispatch fixture: UI service calls are recorded, not native CEGUI.
// Actual BJ/SDK frame updates are covered by the application integration probe.
struct SystemServices : pal4::ISystemMenuRuntime {
    explicit SystemServices(std::vector<std::string>& output) : calls(output) {}
    std::vector<std::string>& calls;
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x319cu, "System reads its UI scene collection"); return 0x4000u;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        Check(address == 0x5000u && value == 0u, "System clears console"); calls.emplace_back("console-clear");
    }
    std::uint32_t UIFrameManager() override { return 0x3000u; }
    void SetUIFlag(std::uint32_t manager, std::uint32_t flag) override {
        Check(manager == 0x3000u && flag == 1u, "System UI flag"); calls.emplace_back("ui-enable");
    }
    std::uint32_t Console() override { return 0x5000u; }
    void AddUIElement(std::uint32_t manager, std::string_view name) override {
        Check(manager == 0x3000u && name == "BJ", "System background"); calls.emplace_back("background");
    }
    void SetupUICamera(std::uint32_t collection, std::uint32_t mode) override {
        Check(collection == 0x4000u && mode == 0u, "System collection camera"); calls.emplace_back("ui-camera");
    }
    void CopyActiveCameraRaster(std::uint32_t manager) override {
        Check(manager == 0x3000u, "System snapshot"); calls.emplace_back("snapshot");
    }
    void GetActiveCamera(std::uint32_t collection) override {
        Check(collection == 0x4000u, "System camera data query"); calls.emplace_back("camera-data");
    }
    std::uint32_t CameraManager() override { return 0x6000u; }
    std::uint32_t UICameraName() override { return 0x7000u; }
    void ActivateCamera(std::uint32_t manager, std::uint32_t name) override {
        Check(manager == 0x6000u && name == 0x7000u, "System UiCamera activation"); calls.emplace_back("ui-activate");
    }
    void DispatchEvent(std::uint32_t manager, std::uint32_t key,
                       std::uint32_t first, std::uint32_t second) override {
        Check(manager == 0x3000u && first == 0u && second == 0u, "System close payload");
        Check(key == pal4::SystemMenuState::kFirstCloseEvent ||
                  key == pal4::SystemMenuState::kSecondCloseEvent, "System close key");
        calls.emplace_back(key == pal4::SystemMenuState::kFirstCloseEvent ? "hud-close" : "toolbar-close");
    }
    void ToggleWindows(std::uint32_t manager, bool enabled) override {
        Check(manager == 0x3000u, "System edge windows"); calls.emplace_back(enabled ? "edges-on" : "edges-off");
    }
    void PreRenderUI(std::uint32_t manager, std::uint32_t frame) override {
        Check(manager == 0x3000u && frame == 0x8000u, "System prerender"); calls.emplace_back("pre-render");
    }
    std::uint32_t RenderUI(std::uint32_t manager, std::uint32_t frame) override {
        Check(manager == 0x3000u && frame == 0x8000u, "System render"); calls.emplace_back("render"); return 37u;
    }
    void UpdateUI(std::uint32_t manager, double elapsed) override {
        Check(manager == 0x3000u && elapsed == 0.25, "System UI frame time"); calls.emplace_back("ui-update");
    }
    std::uint32_t RenderEffectTransform(std::uint32_t manager, double elapsed) override {
        Check(manager == 0x3000u && elapsed == 0.25, "System effect frame time"); calls.emplace_back("effects-update"); return 41u;
    }
    void CleanupEffects(std::uint32_t manager) override {
        Check(manager == 0x3000u, "System effect cleanup"); calls.emplace_back("effects-clear");
    }
};

struct SmithUiServices final : SystemServices {
    explicit SmithUiServices(std::vector<std::string>& output):SystemServices(output){}
    void DispatchEvent(unsigned manager,unsigned key,unsigned first,unsigned second)override{
        Check(manager==0x3000u && !second,"Smith event manager/payload");
        if(key==pal4::SmithUiEvents::kFlagEvent){Check(first<=1u,"Smith show/hide payload");calls.emplace_back(first?"smith-show":"smith-hide");return;}
        Check(key==pal4::SmithUiEvents::kStateEvent && first==1u,"Smith status update payload");calls.emplace_back("smith-status");
    }
};

struct TradeUiServices final : SystemServices {
    using SystemServices::SystemServices;
    std::uint32_t ReadWord(std::uint32_t address) override {
        if(address==0x8000u)return 0x1234u;
        return SystemServices::ReadWord(address);
    }
};

// Records only the audio/script boundary; the real shared music policy runs.
struct TradeAudio final : pal4::IBackgroundMusicRuntime {
    std::vector<std::string> calls;
    bool fail_play=false;
    std::string_view DefaultTrack() override{return {};}
    void InitializeScript() override{calls.emplace_back("initialize");}
    std::uint32_t CreateInterpreter(std::string_view entry,std::string_view name,std::string_view path) override{
        Check(entry=="BGMIniter" && path=="gamedata\\editData\\script\\music.csb","original scene music script");
        calls.emplace_back(name);return 17u;
    }
    bool ExecuteInterpreter(std::uint32_t interpreter,pal4::BackgroundMusicPlayer& player) override{
        Check(interpreter==17u,"music interpreter identity");player.SetSavedTrack("p01.mp3");return false;
    }
    void HandleException(std::uint32_t) override{throw std::logic_error("unexpected script exception");}
    void ReleaseException(std::uint32_t) override{throw std::logic_error("unexpected script exception release");}
    void CleanupScripts() override{calls.emplace_back("cleanup");}
    std::string_view MusicDirectory() override{return "music/";}
    std::uint32_t PlaySound(std::string_view path,bool restore,bool finish) override{
        Check(!restore && !finish,"special/normal music uses non-script sound");
        calls.emplace_back(path);if(fail_play)throw std::runtime_error("audio failure");return 23u;
    }
    std::uint32_t FadeInAudio(std::uint32_t handle,float seconds) override{Check(handle==23u && seconds==3.f,"trade fade in");calls.emplace_back("fade-in");return 0;}
    std::uint32_t FadeOutAudio(std::uint32_t,float seconds) override{Check(seconds==1.f,"trade fade out");calls.emplace_back("fade-out");return 0;}
    std::uint32_t StopAudioImmediately(std::uint32_t) override{throw std::logic_error("unexpected immediate stop");}
    bool IsAudioActive(std::uint32_t) override{throw std::logic_error("unexpected active probe");}
    std::uint8_t PauseAudio(std::uint32_t) override{throw std::logic_error("unexpected pause");}
    std::uint32_t SetAudioProperty(std::uint32_t,std::uint32_t) override{throw std::logic_error("unexpected property");}
    std::uint8_t SavedTextAssignmentResult(std::string_view,bool) override{throw std::logic_error("unexpected assignment");}
    std::uint32_t CleanupTemporaryText() override{return 0x61u;}
    std::uint32_t CleanupTemporaryPath() override{return 0;}
};

struct Cameras final : pal4::INativeCameraObjectResources {
    pal4::CameraRenderRect RenderRect() override {
        return {0, 0, 640, 480};
    }
    void LogInitialization(bool) override {}
    bool Initialize(pal4::CameraSystemObjectState& object,
                    pal4::CameraDataState& data,
                    const pal4::CameraSetupParameters&) override {
        data.At(0u) = 0x1000u;
        return object.camera == reinterpret_cast<std::uintptr_t>(&data);
    }
    void Destroy(pal4::CameraSystemObjectState& object,
                 pal4::CameraDataState& data) noexcept override {
        object.camera = 0u;
        data.At(0u) = 0u;
    }
    void SetFarClip(std::uint32_t, float) override {}
    float MinimumDistance() override { return 1.0f; }
    float MaximumDistance() override { return 1000.0f; }
    std::uint32_t UpdateMatrix(pal4::CameraDataState&,
                               std::uint32_t) override { return 1u; }
};

// Package dispatch fixture only; native Bink decode/render/update is exercised
// with original resources in the application test, not implemented here.
struct BinkServices final : pal4::INativeBinkStateDispatch {
    explicit BinkServices(std::vector<std::string>& output) : calls(output) {}
    std::vector<std::string>& calls;
    std::uint32_t words[2]{};
    bool entered = false;
    std::uint32_t Address() const override { return reinterpret_cast<std::uint32_t>(words); }
    bool Enter(std::uint32_t previous, std::uint32_t document) override {
        Check(document != 0u, "Bink entry receives a temporary document");
        words[1] = previous; entered = true; calls.emplace_back("bink-entry"); return true;
    }
    void Leave() override { entered = false; calls.emplace_back("bink-leave"); }
    void Render(std::uint32_t frame, bool clock) override {
        Check(frame == 0x8000u && !clock, "Bink forwards explicit fast-test timing");
        calls.emplace_back("bink-render");
    }
    std::uint32_t Update(float elapsed) override {
        Check(elapsed == 0.25f, "Bink forwards frame delta"); calls.emplace_back("bink-update"); return 41u;
    }
};

struct CombatServices final : pal4::INativeCombatStateDispatch {
    std::uint32_t image{};
    std::vector<std::string>& calls;
    bool entered=false;
    explicit CombatServices(std::vector<std::string>& output):calls(output) {}
    std::uint32_t Address() const override {return reinterpret_cast<std::uint32_t>(&image);}
    bool Enter(std::uint32_t previous,std::uint32_t document) override {
        Check(previous==1u && document==0x1100u,"Combat receives Scene and unchanged native XML argument");
        calls.emplace_back("combat-enter");entered=true;return true;
    }
    void Leave(std::uint32_t next) override {
        Check((next==1u || next==7u) && entered,"Combat leaves to Scene or GameOver");
        calls.emplace_back("combat-leave");entered=false;
    }
    void Render(std::uint32_t frame) override {Check(frame==0x8000u && entered,"Combat render arguments");calls.emplace_back("combat-render");}
    std::uint32_t Update(float delta) override {Check(delta==0.25f && entered,"Combat update arguments");calls.emplace_back("combat-update");return 41u;}
};

struct GameOverServices final:pal4::INativeGameOverStateDispatch {
    std::array<std::uint32_t,5> image{};std::vector<std::string>& calls;bool entered{};
    explicit GameOverServices(std::vector<std::string>& c):calls(c){}
    std::uint32_t Address()const override{return reinterpret_cast<std::uint32_t>(image.data());}
    bool Enter(std::uint32_t previous,std::uint32_t doc)override{Check(previous==3u && doc==0x2200u,"GameOver preserves Combat and XML argument");entered=true;calls.push_back("over-entry");return true;}
    void Leave()override{Check(entered,"GameOver Leave entered");entered=false;calls.push_back("over-leave");}
    void Render(std::uint32_t frame)override{Check(entered && frame==0x8000u,"GameOver frame forwarding");calls.push_back("over-render");}
    void Update(float dt)override{Check(entered && dt==.25f,"GameOver elapsed forwarding");calls.push_back("over-update");}
};
struct LogoServices final:pal4::INativeLogoStateDispatch {
    std::uint32_t image{};std::vector<std::string>& calls;bool entered{};
    explicit LogoServices(std::vector<std::string>& c):calls(c){}
    std::uint32_t Address()const override{return reinterpret_cast<std::uint32_t>(&image);}
    bool Enter()override{Check(!entered,"Logo enters once");entered=true;calls.push_back("logo-entry");return true;}
    void Leave()override{Check(entered,"Logo leaves after Entry");entered=false;calls.push_back("logo-leave");}
    void Render(std::uint32_t,bool)override{throw std::logic_error("unused Logo fixture render");}
    std::uint32_t Update(float,bool)override{throw std::logic_error("unused Logo fixture update");}
};

void EnterLoadingFromArenaCallback() {
    pal4::NativeGameArenaManagerOwner arena;
    Check(arena.Construct() != 0u, "arena manager constructs");
    pal4::NativePalGameIVTextureStorage game([](std::uint32_t) {});
    Check(game.Construct() != 0u, "PALGameIV constructs");
    pal4::NativeWorldLoadingStateOwner loading;
    Check(loading.Construct() != 0u, "Loading object constructs");
    pal4::NativeWorldStateOwner scene;
    Check(scene.Construct() != 0u, "Scene object constructs");
    pal4::NativePalGameIVStateList states(game.Address());
    Check(states.Register(scene.Address(), 1u,
                          [&] { scene.Destroy(); }),
          "Scene is registered as state one");
    Check(states.Register(loading.Address(), 6u,
                          [&] { loading.Destroy(); }),
          "Loading is registered as state six");
    pal4::NativeSystemMenuStateOwner system;
    Check(system.Construct() != 0u && states.Register(system.Address(), 2u,
              [&] { system.Destroy(); }), "System is registered as state two");
    pal4::NativeSmithStateOwner smith;Check(!smith.Address() && smith.Construct() && states.Register(smith.Address(),5u,[&](){smith.Destroy();}),"native Smith is registered as state five");
    Check(*reinterpret_cast<const unsigned*>(smith.Address())==pal4::SmithState::kVtable && pal4::SmithState::kVtable==0x849468u,"Smith image uses original verified four-byte vtable tag");
    bool duplicate_smith=false;try{(void)smith.Construct();}catch(const std::logic_error&){duplicate_smith=true;}Check(duplicate_smith,"Smith owner rejects double construction");
    pal4::NativeTradeStateOwner trade;
    Check(!trade.Address() && trade.Construct() && states.Register(trade.Address(),4u,[&]{trade.Destroy();}),"Trade registered as state four");
    Check(*reinterpret_cast<const unsigned*>(trade.Address())==0x849450u,"Trade four-byte original identity");
    bool duplicate_trade=false;try{(void)trade.Construct();}catch(const std::logic_error&){duplicate_trade=true;}Check(duplicate_trade,"Trade owner rejects double construction");
    pal4::NativeWorldMapStateOwner world_map;
    Check(world_map.Construct() && states.Register(world_map.Address(),11u,[&]{world_map.Destroy();}),"WorldMap registered as state eleven");
    pal4::GameRenderState game_state(0x8e31e0u);
    pal4::NativeCameraManagerOwner camera_manager;
    Cameras camera_resources;
    Check(camera_manager.Construct() != 0u &&
              camera_manager.InitializeCameras(camera_resources),
          "startup cameras are ready before giArenaLoad");
    Host host;
    BinkServices bink_services(host.calls);
    CombatServices combat_services(host.calls);
    GameOverServices over(host.calls);LogoServices logo(host.calls);
    Check(states.Register(over.Address(),7u,[]{}) &&
          states.Register(logo.Address(),0u,[]{}),"register protocol GameOver/Logo owners");
    Check(states.Register(bink_services.Address(), 12u, [&] {
              Check(!bink_services.entered, "Bink registry retires after Leave");
          }), "Bink is registered as state twelve");
    {
        pal4::NativeArenaLoadRuntime runtime(
            arena, game, states, loading, camera_manager,
            game_state, host);
        Check(pal4::GameArenaLoadHandler::Load(
                  {"Q01"}, {"N01"}, {""}, runtime) == 0 &&
                  runtime.LoadingActive() &&
                  runtime.LoadingScenePending() &&
                  runtime.LiveDocumentCount() == 0u &&
                  arena.ReadyFlag() == 0u &&
                  loading.String(12u) == "Q01" &&
                  loading.String(24u) == "N01" &&
                  loading.String(36u).empty() &&
                  game_state.Read(40u) == 0u &&
                  game_state.Read(0x56cu) == 0x12345678u &&
                  game_state.Read(0x570u) == 0x12345678u &&
                  game_state.Read(0x574u) == 0x12345678u &&
                   camera_manager.State().At(0x30u) ==
                       camera_manager.State().At(0x34u) &&
                  host.visible,
              "giArenaLoad enters the registered native Loading state");
        Check(host.calls == std::vector<std::string>{
                  "cursor", "camera", "show", "actor",
                  "actor-clear", "input"},
              "state, camera, UI and actor/input calls follow IDA order");
        arena.SetSceneName("N01");
        Check(runtime.LoadingScenePending(),
              "scene name alone cannot complete the Loading state");
        const auto before = host.calls.size();
        Check(pal4::GameArenaLoadHandler::Load(
                  {"OTHER"}, {"OTHER"}, {""}, runtime) == 0 &&
                  host.calls.size() == before &&
                  loading.String(12u) == "Q01",
              "arena ready gate rejects a second load request");
        *reinterpret_cast<std::uint32_t*>(
            static_cast<std::uintptr_t>(arena.Address() + 196u)) = 1u;
        Check(pal4::GameArenaLoadHandler::Load(
                  {"Q01"}, {"N02"}, {""}, runtime) == 0 &&
                  runtime.LoadingActive() && host.visible &&
                  runtime.LoadingScenePending() &&
                  loading.String(12u) == "Q01" &&
                  loading.String(24u) == "N02" &&
                  arena.ReadyFlag() == 0u,
              "arena request can re-enter Loading for a second scene");
        Check(std::vector<std::string>(host.calls.begin() + before,
                                       host.calls.end()) ==
                  std::vector<std::string>{"hide", "cursor", "exchange",
                                           "camera", "show", "actor",
                                           "actor-clear", "input"},
              "re-entering Loading follows leave and entry order");
        Check(runtime.RenderLoading(0x5000u) == 0x99u && host.calls.back() == "render-loading",
              "active Loading forwards its real UI callback");
        const auto before_failure = host.calls.size();
        host.fail_scene_runtime = true;
        bool failed = false;
        try {
            static_cast<void>(runtime.AdvanceLoading());
        } catch (const std::runtime_error&) {
            failed = true;
        }
        Check(failed && runtime.LoadingActive() &&
                  *reinterpret_cast<const std::uint32_t*>(
                      static_cast<std::uintptr_t>(loading.Address() + 4u)) ==
                      0u &&
                  std::vector<std::string>(host.calls.begin() + before_failure,
                                           host.calls.end()) ==
                      std::vector<std::string>{"scene-resources",
                                               "scene-runtime"},
              "incomplete scene runtime keeps Loading uncommitted");
        host.fail_scene_runtime = false;
        const auto before_scene = host.calls.size();
        Check(runtime.AdvanceLoading() == 0u &&
                  !runtime.LoadingActive() &&
                  !host.visible && host.scene_previous_id == 6u &&
                  host.scene_argument == 0u &&
                  *reinterpret_cast<const std::uint32_t*>(
                      static_cast<std::uintptr_t>(loading.Address() + 4u)) ==
                      1u &&
                  *reinterpret_cast<const std::uint32_t*>(
                      static_cast<std::uintptr_t>(game.Address() + 0x18cu)) ==
                      scene.Address(),
              "Loading Advance commits only after Scene entry succeeds");
        Check(std::vector<std::string>(host.calls.begin() + before_scene,
                                       host.calls.end()) ==
                  std::vector<std::string>{
                      "scene-resources", "scene-runtime", "player-scene",
                      "hide", "cursor", "exchange", "scene-entry"},
              "Loading Advance follows the original scene and state order");
        const auto before_unbound_system = host.calls.size();
        bool outside_loading_rejected=false;
        try{(void)runtime.RenderLoading(0x5000u);}catch(const std::logic_error&){outside_loading_rejected=true;}
        Check(runtime.SceneActive() && outside_loading_rejected && host.calls.size()==before_unbound_system,
              "Scene rejects stale Loading render without calling UI");
        bool unbound_system_rejected = false;
        try { (void)runtime.TransitionToSystem(); }
        catch (const std::logic_error&) { unbound_system_rejected = true; }
        Check(unbound_system_rejected && host.calls.size() == before_unbound_system &&
                  !runtime.SystemActive(), "unbound System fails before leaving Scene");
        {
            std::array<std::uint8_t,0x224u> ui{};
            std::uint32_t console = 7u;
            pal4::NativeWorldMapStateServices services;
            services.manager = reinterpret_cast<std::uint32_t>(ui.data());
            services.console = reinterpret_cast<std::uint32_t>(&console);
            services.music = 0x100u;
            services.script_executor = 0x200u;
            services.play_music = [&](auto track, float fade, auto flags) {
                Check(track == "P01-1.mp3" && fade == 3.f && !flags && ui[0x221u] == 1u && !console,
                      "WorldMap enters with original UI/console/music order");
                host.calls.emplace_back("worldmap-music");
            };
            services.update_ui = [&](double dt) { Check(dt == .25,"WorldMap UI delta"); host.calls.emplace_back("worldmap-ui"); };
            services.process_scripts = [&](double dt) { Check(dt == .25,"WorldMap script delta"); host.calls.emplace_back("worldmap-scripts"); return 43u; };
            services.render_ui = [&](auto frame) { Check(frame == 0x8000u,"WorldMap frame"); host.calls.emplace_back("worldmap-render"); return 47u; };
            pal4::NativeWorldMapStateRuntime map_runtime(std::move(services));
            bool unbound = false;
            try { (void)runtime.TransitionToWorldMap(); } catch (const std::logic_error&) { unbound = true; }
            Check(unbound && runtime.SceneActive() && host.calls.size() == before_unbound_system,
                  "unbound WorldMap rejects before leaving Scene");
            runtime.BindWorldMapState(&map_runtime);
            bool foreign = false;
            try { (void)runtime.TransitionToWorldMap(0xdeadbeefu); } catch (const std::invalid_argument&) { foreign = true; }
            Check(foreign && runtime.SceneActive() && host.calls.size() == before_unbound_system,
                  "foreign WorldMap document rejects before leaving Scene");
            auto& xml = static_cast<pal4::IGameArenaLoadRuntime&>(runtime);
            const auto document = xml.InitializeXmlDocument();
            xml.InitializeStateAndUi(xml.GameInstance(),11u,document);
            Check(runtime.WorldMapActive() && !runtime.SceneActive() && states.StateId(world_map.Address()) == 11u,
                  "script callback commits registered WorldMap state eleven");
            Check(xml.DestroyXmlDocument(document) == 0 && !runtime.LiveDocumentCount(),"WorldMap entry borrows temporary XML only");
            bool active = false;
            try { runtime.BindWorldMapState(nullptr); } catch (const std::logic_error&) { active = true; }
            Check(active && runtime.WorldMapActive(),"active WorldMap retains borrowed services");
            const auto before_frame = host.calls.size();
            Check(runtime.UpdateWorldMap(.25f) == 43u && runtime.RenderWorldMap(0x8000u) == 47u &&
                  std::vector<std::string>(host.calls.begin()+before_frame,host.calls.end()) ==
                  std::vector<std::string>{"worldmap-ui","worldmap-scripts","worldmap-render"},
                  "WorldMap updates UI before root script waits and renders only UI");
            Check(runtime.TransitionToScene() && host.scene_previous_id == 11u,"WorldMap returns through shared Scene entry with previous ID eleven");
            Check(runtime.TransitionToWorldMap(),"Scene reenters WorldMap");
            *reinterpret_cast<std::uint32_t*>(arena.Address()+196u) = 1u;
            Check(pal4::GameArenaLoadHandler::Load({"Q01"},{"N02"},{""},runtime) == 0 && runtime.LoadingActive() && !runtime.WorldMapActive(),
                  "destination arena callback leaves WorldMap and enters actual Loading state");
            Check(runtime.AdvanceLoading() == 0u && runtime.SceneActive() && host.scene_previous_id == 6u,
                  "WorldMap destination Loading completes the normal Scene initialization");
            runtime.BindWorldMapState(nullptr);
            bool inactive = false;
            try { (void)runtime.UpdateWorldMap(.25f); } catch (const std::logic_error&) { inactive = true; }
            Check(inactive,"inactive WorldMap rejects calls after retiring services");
        }
        const auto before_unbound_bink = host.calls.size();
        bool unbound_bink_rejected = false;
        try { (void)runtime.TransitionToBink(); }
        catch (const std::logic_error&) { unbound_bink_rejected = true; }
        Check(unbound_bink_rejected && host.calls.size() == before_unbound_bink,
              "unbound Bink fails before leaving Scene");
        runtime.BindBinkState(&bink_services);
        const auto before_bink = host.calls.size();
        Check(runtime.TransitionToBink() && runtime.BinkActive() &&
                  bink_services.words[1] == 1u && runtime.LiveDocumentCount() == 0u,
              "Scene-to-Bink stores previous state one and releases entry document");
        runtime.RenderBink(0x8000u, false);
        Check(runtime.UpdateBink(0.25f) == 41u &&
                  std::vector<std::string>(host.calls.begin() + before_bink, host.calls.end()) ==
                      std::vector<std::string>{"scene-leave", "cursor", "exchange", "bink-entry",
                          "bink-render", "bink-update"},
              "Bink dispatch preserves lifecycle order and frame results");
        const auto return_document = runtime.CreateBinkReturnDocument();
        runtime.ReturnBinkToScene(return_document);
        Check(!runtime.BinkActive() && !bink_services.entered && host.scene_previous_id == 12u &&
                  host.scene_argument == return_document && runtime.LiveDocumentCount() == 1u,
              "Bink return forwards previous twelve and the live empty XML document to Scene");
        Check(runtime.DestroyBinkReturnDocument(return_document) == 0u && !runtime.LiveDocumentCount(),
              "Bink return document is destroyed after Scene entry");
        runtime.BindBinkState(nullptr);
        SystemServices system_services(host.calls);
        runtime.BindSystemMenu(&system_services);
        const auto before_system = host.calls.size();
        Check(runtime.TransitionToSystem() && runtime.SystemActive() &&
                  *reinterpret_cast<const std::uint32_t*>(game.Address() + 0x18cu) == system.Address() &&
                  game_state.Read(0x56cu) == 0x12345678u &&
                  game_state.Read(0x574u) == 0x12345678u,
              "shared transition commits native System identity and clocks");
        Check(std::vector<std::string>(host.calls.begin() + before_system, host.calls.end()) ==
                  std::vector<std::string>{"scene-leave", "cursor", "exchange", "ui-enable",
                      "console-clear", "background", "ui-camera", "snapshot", "camera-data",
                      "ui-activate", "hud-close", "toolbar-close", "edges-on"},
              "System entry uses shared lifecycle after Scene leave and cursor reset");
        bool active_unbind_rejected = false;
        try { runtime.BindSystemMenu(nullptr); }
        catch (const std::logic_error&) { active_unbind_rejected = true; }
        runtime.BindSystemMenu(&system_services);
        Check(active_unbind_rejected && runtime.SystemActive(), "active System retains its borrowed services");
        const auto before_system_frame = host.calls.size();
        Check(runtime.RenderSystem(0x8000u) == 37u && runtime.UpdateSystem(0.25f, 0u, 0u) == 41u &&
                  std::vector<std::string>(host.calls.begin() + before_system_frame, host.calls.end()) ==
                      std::vector<std::string>{"pre-render", "render", "ui-update", "effects-update"},
              "active System forwards actual shared frame entrypoints");
        const auto before_system_return = host.calls.size();
        Check(runtime.TransitionToScene() && !runtime.SystemActive() && host.scene_previous_id == 2u &&
                  *reinterpret_cast<const std::uint32_t*>(game.Address() + 0x18cu) == scene.Address() &&
                  std::vector<std::string>(host.calls.begin() + before_system_return, host.calls.end()) ==
                      std::vector<std::string>{"edges-off", "effects-clear", "cursor", "exchange", "scene-entry"},
              "System cleanup precedes shared return to Scene with previous ID two");
        // Script entry must commit the actual registered state before publishing
        // its selection event; UI rendering itself is recorded in this fixture.
        std::uint32_t script_selections{};
        pal4::NativeScriptSystemUiServices script_ui_services;
        script_ui_services.enter_system=[&]{Check(runtime.TransitionToSystem(),"script System transition");};
        script_ui_services.event=[&](auto key,auto choice,auto reserved){
            Check(runtime.SystemActive() && key==0x8a80b8u && choice==5u && !reserved,
                  "script selection observes committed native System state and original index plus one");
            ++script_selections;
        };
        pal4::NativeScriptSystemUiRuntime script_ui(game,host.UiFrameManagerAddress(),nullptr,std::move(script_ui_services));
        pal4::ScriptSystemUi::Open(4u,script_ui);
        Check(script_selections==1u && runtime.SystemActive(),"native script adapter enters real registered System state");
        Check(runtime.TransitionToScene() && host.scene_previous_id==2u,"script System returns through shared Scene lifecycle");
        const auto before_bad_script=host.calls.size();
        for(const auto bad: {7u,0xffffffffu,0x80000000u})pal4::ScriptSystemUi::Open(bad,script_ui);
        Check(script_selections==1u && host.calls.size()==before_bad_script,"out-of-range script indices have no state or UI effects");
        bool bad_script_owner=false;
        try {script_ui.InitializeStateAndUi(game.Address()+4u,2u,0u);}
        catch(const std::invalid_argument&){bad_script_owner=true;}
        bool bad_script_tree=false;
        try {script_ui.ProcessEvent(host.UiFrameManagerAddress(),0x8a80b8u,1u,0u);}
        catch(const std::invalid_argument&){bad_script_tree=true;}
        Check(bad_script_owner && bad_script_tree && host.calls.size()==before_bad_script && script_selections==1u,
              "script adapter rejects foreign owner and event tree before callbacks");
        runtime.BindSystemMenu(nullptr);
        bool inactive_system_rejected = false;
        try { (void)runtime.RenderSystem(0x8000u); }
        catch (const std::logic_error&) { inactive_system_rejected = true; }
        Check(inactive_system_rejected, "inactive System cannot render through retired services");
        std::cout << "Native registered Scene-System-Scene dispatch, borrowed service gates and frame ordering verified\n";
        {
            SmithUiServices smith_ui(host.calls);std::uint8_t flag=0xaau;
            pal4::IGameArenaLoadRuntime& smith_xml=runtime;
            pal4::NativeSmithStateRuntime smith_runtime(smith_ui,game_state,flag,[&](unsigned doc,std::string_view key){return runtime.ReadConfigInteger(doc,key);});
            const auto before_missing=host.calls.size();bool missing=false;try{(void)runtime.TransitionToSmith();}catch(const std::logic_error&){missing=true;}
            Check(missing && host.calls.size()==before_missing,"unbound Smith refuses transition before leaving Scene");
            bool partial=false;try{runtime.BindSmithState(&smith_runtime,nullptr);}catch(const std::invalid_argument&){partial=true;}Check(partial,"Smith requires lifecycle and UI render services together");
            runtime.BindSmithState(&smith_runtime,&smith_ui);
            const auto before_bad=host.calls.size();bool bad_document=false;try{(void)runtime.TransitionToSmith(0xdeadbeefu);}catch(const std::invalid_argument&){bad_document=true;}
            Check(bad_document && host.calls.size()==before_bad,"foreign config rejected before Scene leave");
            for(const auto value:std::vector<std::optional<int>>{std::nullopt,1,0,-1,2}){
                unsigned document{};if(value){document=smith_xml.InitializeXmlDocument();const auto a=smith_xml.InitializeIntAttribute("Smith_OpenWithSystem",*value);smith_xml.AddAttribute(document,a);smith_xml.DestroyAttribute(a);}
                game_state.Write(1412u,0xffffffffu);game_state.Write(1364u,0xffffffffu);
                const auto before=host.calls.size();Check(runtime.TransitionToSmith(document) && runtime.SmithActive() && !runtime.SystemActive() && states.StateId(smith.Address())==5u &&
                    *reinterpret_cast<const unsigned*>(game.Address()+0x18cu)==smith.Address(),"shared transition commits original Smith identity");
                Check(flag==static_cast<std::uint8_t>(value && *value>0) && !game_state.Read(1412u) && !game_state.Read(1364u),"real native flag and both render flags follow signed-positive config");
                Check(std::vector<std::string>(host.calls.begin()+before,host.calls.end())==std::vector<std::string>{"scene-leave","cursor","exchange","ui-enable","console-clear","background","camera-data","ui-activate","smith-show"},"Smith enter reuses original ordering without System snapshot/edge actions");
                if(document)Check(smith_xml.DestroyXmlDocument(document)==0u && !runtime.LiveDocumentCount(),"Smith does not retain temporary config after entry");
                bool active_unbind=false;try{runtime.BindSmithState(nullptr,nullptr);}catch(const std::logic_error&){active_unbind=true;}
                runtime.BindSmithState(&smith_runtime,&smith_ui);Check(active_unbind && runtime.SmithActive(),"active Smith keeps borrowed services");
                const auto frame_begin=host.calls.size();Check(runtime.RenderSmith(0x8000u)==37u && runtime.UpdateSmith(0.25f)==41u &&
                    std::vector<std::string>(host.calls.begin()+frame_begin,host.calls.end())==std::vector<std::string>{"pre-render","render","smith-status","ui-update","effects-update"},"Smith uses common renderer then original status/UI/effects update order");
                const auto leave_begin=host.calls.size();Check(runtime.TransitionToScene() && !runtime.SmithActive() && host.scene_previous_id==5u && !host.scene_argument &&
                    std::vector<std::string>(host.calls.begin()+leave_begin,host.calls.end())==std::vector<std::string>{"smith-hide","effects-clear","cursor","exchange","scene-entry"},"Smith hide/effect cleanup precede Scene entry with previous ID five");
            }
            const auto empty_config=smith_xml.InitializeXmlDocument();Check(runtime.TransitionToSmith(empty_config) && !flag && runtime.TransitionToScene(),"owned config without Smith attribute defaults to normal mode after prior System mode");
            Check(smith_xml.DestroyXmlDocument(empty_config)==0u && !runtime.LiveDocumentCount(),"empty Smith config cleanup is balanced");
            runtime.BindSmithState(nullptr,nullptr);bool inactive=false;try{(void)runtime.RenderSmith(0x8000u);}catch(const std::logic_error&){inactive=true;}Check(inactive,"inactive Smith cannot render borrowed services");
            std::cout<<"Native registered Scene-Foundry-Scene, actual flag/config/render-state storage, lifecycle/update/render dispatch verified (recorded UI services)\n";
        }
        {
            const auto before_missing=host.calls.size();bool missing=false;
            try{(void)runtime.TransitionToTrade();}catch(const std::logic_error&){missing=true;}
            Check(missing && host.calls.size()==before_missing,"unbound Trade rejects before leaving Scene");
            pal4::NativeTradeSystemOwner trade_system;
            bool unavailable=false;try{(void)trade_system.State();}catch(const std::logic_error&){unavailable=true;}
            Check(unavailable && trade_system.Construct() && trade_system.OutstandingAllocations()==1u,"real Trade system owns one54-byte allocation");
            Check(trade_system.ReadWord(4u)==1u,"original Trade constructor initializes word1");
            for(unsigned offset=8u;offset<0x54u;offset+=4u)Check(!trade_system.ReadWord(offset),"original Trade constructor clears only trailing20 words");
            bool duplicate=false;try{(void)trade_system.Construct();}catch(const std::logic_error&){duplicate=true;}
            Check(duplicate,"Trade singleton owner refuses duplicate construction");
            TradeUiServices trade_ui(host.calls);
            pal4::NativeTradeStateServices endpoints;
            endpoints.frame_camera=[&](auto frame){return trade_ui.ReadWord(frame);};
            endpoints.world_frame=[&](auto frame){Check(frame==0x1234u,"Trade world receives dereferenced frame word");host.calls.emplace_back("trade-world");};
            endpoints.wait_handlers=[&](double delta){Check(delta==0.25,"Trade script wait delta widened from float");host.calls.emplace_back("trade-waits");};
            pal4::NativeTradeSystemUiRuntime* close_services=nullptr;
            endpoints.close_shop=[&]{
                host.calls.emplace_back("trade-close");
                if(close_services){pal4::TradeSystemUi operations(trade_system.State());(void)operations.CloseShop(*close_services);}
                else trade_system.State()=0u;
            };
            endpoints.bind_ui_camera=[&](auto camera){Check(camera==camera_manager.ActiveCameraPointer(),"Trade effects borrow selected actual camera data");host.calls.emplace_back("trade-camera-bind");};
            bool incomplete=false;auto partial=endpoints;partial.wait_handlers={};
            try{pal4::NativeTradeStateRuntime bad(trade_ui,camera_manager,trade_system,arena.Address(),0x7000u,std::move(partial));}
            catch(const std::invalid_argument&){incomplete=true;}
            Check(incomplete && host.calls.size()==before_missing,"Trade requires real script wait service before dispatch");
            pal4::NativeTradeStateRuntime trade_runtime(trade_ui,camera_manager,trade_system,arena.Address(),0x7000u,std::move(endpoints));
            runtime.BindTradeState(&trade_runtime);
            // The headless loading host records resource loading, so supply its
            // world-name field explicitly and restore the borrowed pointer.
            struct BorrowedWorldName {
                unsigned& field;unsigned original;
                ~BorrowedWorldName(){field=original;}
            } world_name{*reinterpret_cast<unsigned*>(arena.Address()+8u),
                *reinterpret_cast<unsigned*>(arena.Address()+8u)};
            world_name.field=reinterpret_cast<unsigned>("q01");
            pal4::NativeGameDbTable shop_table,good_table;
            shop_table.name="ShopProperty";
            shop_table.fields={{"ID",2u},{"NAME",2u},{"POSITION",2u},{"TYPE",1u}};
            shop_table.rows={{std::string("shop01"),std::string("shop"),std::string("Q01"),0u}};
            good_table.name="Good";
            good_table.fields={{"ID",1u},{"SHOP_ID",2u},{"GOODS_ID",1u},{"GOODS_TYPE",1u},{"OPEN_CONDITION",1u},{"OPEN_NUM",1u}};
            good_table.rows={{1u,std::string("shop01"),1001u,0u,0u,0u}};
            pal4::NativeShopRecordCatalog shop_catalog(shop_table,good_table);
            pal4::BackgroundMusicPlayer music;
            TradeAudio audio;
            struct TradeEvent{unsigned key,first,second;};std::vector<TradeEvent> trade_events;
            std::string selected_shop="shop01";
            pal4::NativeTradeSystemUiServices ui_services;
            ui_services.event=[&](auto tree,auto key,auto first,auto second){
                Check(tree==0x3104u,"actual trade UI event tree");trade_events.push_back({key,first,second});
                if(key==pal4::TradeSystemUi::kEventShowSelector){
                    Check(first==1u && !second && runtime.TradeActive(),"selector published after committed Trade4");host.calls.emplace_back("shop-selector");
                }
                return 0x134;
            };
            ui_services.edges=[&](auto manager,bool visible){Check(manager==0x3000u,"trade edge manager");host.calls.emplace_back(visible?"edges-on":"edges-off");};
            ui_services.shop_text=[&](auto descriptor)->std::string_view{Check(descriptor==0x200u,"selector-owned text identity");return selected_shop;};
            pal4::NativeTradeSystemUiRuntime clear_services(runtime,arena,game,shop_catalog,music,audio,0x3000u,ui_services);
            bool missing_service=false;auto incomplete_services=ui_services;incomplete_services.edges={};
            try{pal4::NativeTradeSystemUiRuntime bad(runtime,arena,game,shop_catalog,music,audio,0x3000u,std::move(incomplete_services));}
            catch(const std::invalid_argument&){missing_service=true;}
            Check(missing_service,"native Trade UI requires all application services");
            pal4::TradeSystemUi trade_operations(trade_system.State());
            const auto entry=host.calls.size();
            Check(trade_operations.ClearTradeItemInfo(clear_services)==0x34u && runtime.TradeActive() && !trade_system.State() &&
                states.StateId(trade.Address())==4u &&
                std::vector<std::string>(host.calls.begin()+entry,host.calls.end())==std::vector<std::string>{"scene-leave","cursor","exchange","ui-enable","console-clear","shop-selector"},
                "existing ClearTradeItemInfo establishes native state0 and enters actual Trade4 before publishing shop selector");
            const auto first_open_event=trade_events.size();
            Check(trade_operations.OpenShop(0x200u,1u,2u,clear_services)==1u && trade_system.State()==1u &&
                music.State()==3u && music.CurrentTrack()=="p37.mp3" &&
                audio.calls==std::vector<std::string>{"fade-out","music/p37.mp3","fade-in"} &&
                trade_events.size()==first_open_event+3u &&
                trade_events[first_open_event].key==pal4::TradeSystemUi::kEventTradeActive && trade_events[first_open_event].first==1u &&
                trade_events[first_open_event+1u].key==pal4::TradeSystemUi::kEventTradeSelector && trade_events[first_open_event+1u].first==1u &&
                trade_events[first_open_event+2u].key==pal4::TradeSystemUi::kEventTradeGoods && trade_events[first_open_event+2u].first==2u &&
                trade_events.back().second==shop_catalog.FindShop("shop01"),
                "native trade UI reuses actual catalog and music policy before publishing all three trade events");
            const auto close_begin=audio.calls.size();
            Check(trade_operations.CloseShop(clear_services)==0x61 && !trade_system.State() && music.State()==0u && music.CurrentTrack()=="p01.mp3" &&
                std::vector<std::string>(audio.calls.begin()+close_begin,audio.calls.end())==std::vector<std::string>{"initialize","Q01_MUSIC","cleanup","fade-out","music/p01.mp3","fade-in"} &&
                trade_events.back().key==pal4::TradeSystemUi::kEventTradeActive && !trade_events.back().first && host.calls.back()=="edges-off",
                "close restores normal music through existing world initializer and clears trade/edge UI");
            const auto invalid_begin=trade_events.size();
            selected_shop="absent";
            Check(!trade_operations.OpenShop(0x200u,0u,0u,clear_services) && trade_system.State()==1u && trade_events.size()==invalid_begin,
                "missing shop preserves original music/state side effects without publishing trade UI");
            selected_shop="shop01";
            Check(!trade_operations.OpenShop(0x200u,0u,4u,clear_services) && trade_events.size()==invalid_begin,"invalid goods group never publishes trade UI");
            bool foreign_table=false,foreign_word=false,foreign_state=false;
            try{(void)clear_services.FindShopGoods(1u,0x200u);}catch(const std::invalid_argument&){foreign_table=true;}
            try{(void)clear_services.ReadWord(arena.Address()+12u);}catch(const std::invalid_argument&){foreign_word=true;}
            try{(void)clear_services.InitializeGameState(game.Address(),4u,1u);}catch(const std::invalid_argument&){foreign_state=true;}
            Check(foreign_table && foreign_word && foreign_state && runtime.TradeActive(),"foreign identities reject before raw reads or state changes");
            audio.fail_play=true;bool failed_audio=false;
            try{(void)trade_operations.OpenShop(0x200u,0u,0u,clear_services);}catch(const std::runtime_error&){failed_audio=true;}
            audio.fail_play=false;
            // A failed audio backend must not poison the next temporary string.
            Check(failed_audio && trade_operations.OpenShop(0x200u,0u,0u,clear_services)==1u,"audio failure unwinds host temporary music text and allows retry");
            (void)trade_operations.CloseShop(clear_services);
            bool active_unbind=false;try{runtime.BindTradeState(nullptr);}catch(const std::logic_error&){active_unbind=true;}
            runtime.BindTradeState(&trade_runtime);Check(active_unbind,"active Trade retains its runtime");
            const auto inactive_frame=host.calls.size();
            Check(runtime.RenderTrade(0x8000u)==37u && runtime.UpdateTrade(0.25f)==1u &&
                camera_manager.State().At(0x30u)==camera_manager.FindByName("MainCamera") &&
                std::vector<std::string>(host.calls.begin()+inactive_frame,host.calls.end())==std::vector<std::string>{"trade-world","render","trade-waits","ui-update"},
                "closed shop renders world then UI; scripts wait before UI and actual MainCamera without effects");
            trade_system.State()=2u;const auto active_frame=host.calls.size();
            Check(runtime.RenderTrade(0x8000u)==37u && runtime.UpdateTrade(0.25f)==41u &&
                camera_manager.State().At(0x30u)==camera_manager.FindByName("UiCamera") &&
                std::vector<std::string>(host.calls.begin()+active_frame,host.calls.end())==std::vector<std::string>{"pre-render","render","trade-waits","ui-update","trade-camera-bind","effects-update"},
                "any nonzero trade state renders UI, selects actual UiCamera, binds effects and preserves effect return");
            const auto before_return=host.calls.size();
            Check(runtime.TransitionToScene() && host.scene_previous_id==4u && !runtime.TradeActive() && !trade_system.State() &&
                std::vector<std::string>(host.calls.begin()+before_return,host.calls.end())==std::vector<std::string>{"trade-close","effects-clear","cursor","exchange","scene-entry"},
                "Trade close/effect cleanup precedes native Scene restoration with previous4");
            SmithUiServices smith_ui(host.calls);std::uint8_t smith_flag{};
            pal4::NativeSmithStateRuntime smith_runtime(smith_ui,game_state,smith_flag,[&](unsigned doc,std::string_view key){return runtime.ReadConfigInteger(doc,key);});
            runtime.BindSmithState(&smith_runtime,&smith_ui);
            Check(runtime.TransitionToSmith(),"normal Scene-to-Foundry entry before shop return");
            trade_system.State()=7u;const auto foundry_return=host.calls.size();
            Check(trade_operations.ClearTradeItemInfo(clear_services)==0x34u && runtime.TradeActive() && !runtime.SmithActive() && !trade_system.State() &&
                std::vector<std::string>(host.calls.begin()+foundry_return,host.calls.end())==std::vector<std::string>{"smith-hide","effects-clear","cursor","exchange","ui-enable","console-clear","shop-selector"},
                "ordinary Foundry returns through original ClearTradeItemInfo to registered Trade4 and shop selector instead of Scene");
            const auto before_bad=host.calls.size();bool foreign=false;
            try{(void)runtime.TransitionToSmith(0xdeadbeefu);}catch(const std::invalid_argument&){foreign=true;}
            Check(foreign && host.calls.size()==before_bad && runtime.TradeActive(),"foreign Smith config rejects before leaving Trade");
            Check(runtime.TransitionToSmith() && runtime.SmithActive() && !runtime.TradeActive(),"Trade-to-Foundry routes through shared lifecycle");
            Check(trade_operations.ClearTradeItemInfo(clear_services)==0x34u && runtime.TradeActive(),"second Foundry return reaches same Trade owner");
            Check(trade_operations.ClearTradeItemInfo(clear_services)==0x34u && runtime.TradeActive(),"Trade reentry preserves original leave/enter sequence");
            Check(runtime.TransitionToScene() && runtime.TransitionToSmith() && runtime.TransitionToScene(),"normal shop close to Scene then Foundry remains supported");
            // Complete a real Trade-state leave through the native UI adapter,
            // rather than recording CloseShop at that final composition boundary.
            close_services=&clear_services;
            const auto setup_events=trade_events.size();
            Check(trade_operations.SetTradeItemInfo(0x200u,0x204u,clear_services)==0x34u && runtime.TradeActive() &&
                trade_events.size()==setup_events+3u && trade_events[setup_events].key==pal4::TradeSystemUi::kEventShopId &&
                trade_events[setup_events].first==1u && trade_events[setup_events].second==0x200u &&
                trade_events[setup_events+1u].key==pal4::TradeSystemUi::kEventTradeTask && trade_events[setup_events+1u].second==0x204u,
                "real state4 entry precedes original shop/task/show publication");
            Check(trade_operations.OpenShop(0x200u,0u,0u,clear_services)==1u,"native shop opens before actual state leave");
            const auto leave_events=trade_events.size(),leave_audio=audio.calls.size(),leave_calls=host.calls.size();
            Check(trade_operations.SetTradeState(clear_services)==1 && !runtime.TradeActive() && host.scene_previous_id==4u &&
                trade_system.State()==2u && music.State()==0u && music.CurrentTrack()=="p01.mp3" &&
                trade_events.size()==leave_events+1u && trade_events.back().key==pal4::TradeSystemUi::kEventTradeActive && !trade_events.back().first &&
                std::vector<std::string>(audio.calls.begin()+leave_audio,audio.calls.end())==std::vector<std::string>{"initialize","Q01_MUSIC","cleanup","fade-out","music/p01.mp3","fade-in"} &&
                std::vector<std::string>(host.calls.begin()+leave_calls,host.calls.end())==std::vector<std::string>{"trade-close","edges-off","effects-clear","cursor","exchange","scene-entry"},
                "real Trade leave closes page/restores world music before effect cleanup/Scene entry, then original writes state2");
            close_services=nullptr;
            runtime.BindSmithState(nullptr,nullptr);runtime.BindTradeState(nullptr);
            bool inactive=false;try{(void)runtime.RenderTrade(0x8000u);}catch(const std::logic_error&){inactive=true;}
            Check(inactive,"inactive Trade cannot render retired borrowed services");
            trade_system.Destroy();trade_system.Destroy();
            Check(!trade_system.Address() && !trade_system.OutstandingAllocations(),"native Trade system retires without leaks and supports repeated Destroy");
            std::cout<<"Native Scene-Trade-Foundry-Trade-Scene dispatch, real trade storage/camera controller and original shop-selector ordering verified (recorded UI/world/script endpoints)\n";
        }
        const auto before_unbound_combat=host.calls.size();
        bool unbound_combat=false;
        try {(void)runtime.TransitionToCombat(0x1100u);}catch(const std::logic_error&){unbound_combat=true;}
        Check(unbound_combat && host.calls.size()==before_unbound_combat &&
            *reinterpret_cast<const std::uint32_t*>(game.Address()+0x18cu)==scene.Address(),
            "unbound Combat rejects before Scene leave");
        bool unregistered_combat=false;
        try {runtime.BindCombatState(&combat_services);}catch(const std::invalid_argument&){unregistered_combat=true;}
        Check(unregistered_combat && states.Register(combat_services.Address(),3u,[&] {
            Check(!combat_services.entered,"Combat registry retires after leave");
        }),"Combat services require state registration");
        runtime.BindCombatState(&combat_services);
        const auto before_combat_entry=host.calls.size();
        Check(runtime.TransitionToCombat(0x1100u) && runtime.CombatActive() &&
            std::vector<std::string>(host.calls.begin()+before_combat_entry,host.calls.end())==
                std::vector<std::string>{"scene-leave","cursor","exchange","combat-enter"},
            "shared Scene-Combat entry order");
        bool combat_unbind=false;
        try {runtime.BindCombatState(nullptr);}catch(const std::logic_error&){combat_unbind=true;}
        Check(combat_unbind && runtime.CombatActive(),"active Combat retains borrowed services");
        const auto before_combat_frame=host.calls.size();
        runtime.RenderCombat(0x8000u);
        Check(runtime.UpdateCombat(0.25f)==41u &&
            std::vector<std::string>(host.calls.begin()+before_combat_frame,host.calls.end())==
                std::vector<std::string>{"combat-render","combat-update"},"Combat frame dispatch");
        const auto before_combat_return=host.calls.size();
        Check(runtime.ReturnCombatToScene(0x2200u) && !runtime.CombatActive() &&
            host.scene_previous_id==3u && host.scene_argument==0x2200u &&
            std::vector<std::string>(host.calls.begin()+before_combat_return,host.calls.end())==
                std::vector<std::string>{"combat-leave","cursor","exchange","scene-entry"},
            "Combat leave precedes Scene entry with original return document");
        Check(runtime.TransitionToCombat(0x1100u),"second Combat begins for GameOver routing");
        const auto before_missing_over=host.calls.size();bool missing_over=false;
        try{(void)runtime.TransitionCombatToGameOver(0x2200u);}catch(const std::logic_error&){missing_over=true;}
        Check(missing_over && runtime.CombatActive() && host.calls.size()==before_missing_over,"unbound GameOver rejected before Combat Leave");
        runtime.BindGameOverState(&over);runtime.BindLogoState(&logo);
        const auto before_over=host.calls.size();
        Check(runtime.TransitionCombatToGameOver(0x2200u) && runtime.GameOverActive() && !runtime.CombatActive() &&
            std::vector<std::string>(host.calls.begin()+before_over,host.calls.end())==std::vector<std::string>{"combat-leave","cursor","exchange","over-entry"},"actual shared transition orders Combat Leave before GameOver Entry");
        bool unbind_over=false;try{runtime.BindGameOverState(nullptr);}catch(const std::logic_error&){unbind_over=true;}
        Check(unbind_over && over.entered,"active GameOver retains owner");
        runtime.RenderGameOver(0x8000u);runtime.UpdateGameOver(.25f);
        const auto before_logo=host.calls.size();
        Check(runtime.TransitionToLogo() && runtime.LogoActive() && !runtime.GameOverActive() &&
            std::vector<std::string>(host.calls.begin()+before_logo,host.calls.end())==std::vector<std::string>{"over-leave","cursor","exchange","logo-entry"},"GameOver Leave precedes actual Logo Entry");
        runtime.BindGameOverState(nullptr);
        bool over_inactive=false;try{runtime.UpdateGameOver(.25f);}catch(const std::logic_error&){over_inactive=true;}
        Check(over_inactive,"retired GameOver cannot advance");
        *reinterpret_cast<std::uint32_t*>(arena.Address()+196u)=1u;
        (void)pal4::GameArenaLoadHandler::Load({"Q01"},{"N02"},{""},runtime);
        arena.SetSceneName("N02");Check(runtime.LoadingActive() && runtime.AdvanceLoading()==0u && !runtime.LoadingActive(),"fixture returns Logo through real Loading to Scene");
        runtime.BindLogoState(nullptr);
        std::cout<<"Registered GameOver dispatch: preflight, Combat/Logo Leave-Entry order, frame forwarding and owner gates pass (protocol fixture)\n";
        runtime.BindCombatState(nullptr);
        bool inactive_combat=false;
        try {runtime.RenderCombat(0x8000u);}catch(const std::logic_error&){inactive_combat=true;}
        Check(inactive_combat,"inactive Combat cannot render through retired services");
        std::cout<<"Registered Combat dispatch, unchanged XML arguments, owner gates and return order verified (fixture only)\n";

        arena.SetSceneName("N02");
        *reinterpret_cast<std::uint32_t*>(
            static_cast<std::uintptr_t>(arena.Address() + 196u)) = 1u;
        Check(pal4::GameArenaLoadHandler::Load(
                  {"Q01"}, {"N02"}, {""}, runtime) == 0 &&
                  runtime.LoadingScenePending() &&
                  runtime.AdvanceLoading() == 0u &&
                  !runtime.LoadingActive() &&
                  *reinterpret_cast<const std::uint32_t*>(
                      static_cast<std::uintptr_t>(loading.Address() + 4u)) ==
                      1u,
              "same-scene request still performs a complete Loading pass");
        *reinterpret_cast<std::uint32_t*>(
            static_cast<std::uintptr_t>(arena.Address() + 196u)) = 1u;
        const auto before_scene_exit = host.calls.size();
        const auto scene_exit_result = pal4::GameArenaLoadHandler::Load(
            {"Q01"}, {"N01"}, {""}, runtime);
        Check(scene_exit_result == 0 &&
                  runtime.LoadingActive() && runtime.LoadingScenePending() &&
                  loading.String(24u) == "N01" &&
                  std::vector<std::string>(
                      host.calls.begin() + before_scene_exit,
                      host.calls.end()) ==
                      std::vector<std::string>{
                          "scene-leave", "cursor", "exchange", "camera",
                          "show", "actor", "actor-clear", "input"},
              "Scene leave precedes the next Loading entry");
        pal4::BinaryXmlNode archive;
        archive.attributes.push_back({"PALGAMEIV_LOADING_TYPE", 1u, std::int32_t{1}});
        archive.attributes.push_back({"PALGAMEIV_LOADING_ARCHIVE", 1u, std::int32_t{27}});
        pal4::EnterNativeArchiveLoading(archive, runtime);
        Check(runtime.LoadingActive() && !runtime.LoadingScenePending() && host.visible &&
                  runtime.LiveDocumentCount() == 0u &&
                  *reinterpret_cast<const std::uint32_t*>(static_cast<std::uintptr_t>(loading.Address() + 8u)) == 1u &&
                  *reinterpret_cast<const std::uint32_t*>(static_cast<std::uintptr_t>(loading.Address() + 48u)) == 27u,
              "save-page archive config enters real native Loading and releases temporary XML");
        bool restore_failed = false;
        try { (void)runtime.AdvanceLoading(); } catch (const std::logic_error&) { restore_failed = true; }
        Check(restore_failed && runtime.LoadingActive() && host.visible,
              "unbound archive restoration remains an explicit failure without false Scene completion");
        const auto before_invalid = host.calls.size();
        archive.attributes[1].value = std::int32_t{100};
        bool invalid = false;
        try { pal4::EnterNativeArchiveLoading(archive, runtime); } catch (const std::invalid_argument&) { invalid = true; }
        Check(invalid && runtime.LiveDocumentCount() == 0u && host.calls.size() == before_invalid,
              "invalid archive slot is rejected before changing native state");

    }
    Check(!over.entered && !logo.entered,"GameOver/Logo fixtures retire after Leave");
    states.Clear();
    Check(loading.Address() == 0u && scene.Address() == 0u && system.Address() == 0u && smith.Address()==0u && trade.Address()==0u && world_map.Address()==0u &&
              states.OutstandingAllocations() == 0u,
          "state cleanup releases the Loading object");
    game.Destroy();
    arena.Destroy();
    camera_manager.Destroy();
    Check(arena.OutstandingAllocations() == 0u,
          "arena cleanup releases native manager members");
}

}  // namespace

int main() {
    try {
        EnterLoadingFromArenaCallback();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
