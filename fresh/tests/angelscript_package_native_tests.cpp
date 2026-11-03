#include "pal4/effect_render_parameters.h"
#include "pal4/game_arena_ready_handler.h"
#include "pal4/background_music_player.h"
#include "pal4/native_angelscript_engine.h"
#include "pal4/native_game_script_callbacks.h"
#include "pal4/native_script_flash_runtime.h"
#include "pal4/native_package_archive.h"
#include "pal4/pal_screen_effect.h"
#include "pal4/script_camera_callbacks.h"
#include "pal4/script_camera_sequences.h"
#include "pal4/script_player_actions.h"
#include "pal4/script_player_action.h"
#include "pal4/script_player_equipment.h"
#include "pal4/script_player_position.h"
#include "pal4/script_player_rotation.h"
#include "pal4/script_system_ui.h"
#include "pal4/script_sound_2d.h"
#include "pal4/script_wait_handler.h"
#include "pal4/script_entity_callbacks.h"
#include "pal4/player_manager_object.h"
#include "pal4/script_music_transport.h"
#include "scriptstring.h"

#include <cstddef>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <stdexcept>
#include <utility>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "%s\n", message);
        std::exit(EXIT_FAILURE);
    }
}

class RecordingCallbacks final : public pal4::IGameScriptCallbackRuntime {
public:
    void Invoke(int system_id, asIScriptGeneric& arguments) override {
        switch (system_id) {
        case -291:
            Check(arguments.GetArgFloat(0) > 0.09f &&
                      arguments.GetArgFloat(0) < 0.11f &&
                      arguments.GetArgDWord(1) == 1u &&
                      arguments.GetArgDWord(2) == 1u,
                  "original fade arguments");
            events.emplace_back("flash");
            break;
        case -237:
            events.emplace_back("music_pause");
            break;
        case -297:
            events.emplace_back("movie:" + static_cast<asCScriptString*>(
                arguments.GetArgObject(0))->buffer);
            break;
        case -110:
            Check(arguments.GetArgDWord(0) == 0u, "original movie flag");
            events.emplace_back("movie_flag");
            break;
        case -57:
            events.emplace_back("new_game");
            break;
        case -238:
            events.emplace_back("music_resume");
            break;
        case -65: {
            const auto text = [&arguments](asUINT index) {
                return static_cast<asCScriptString*>(
                    arguments.GetArgObject(index))->buffer;
            };
            Check(text(0) == "Q01" && text(1) == "N01" && text(2).empty() &&
                      arguments.GetArgDWord(3) == 1u,
                  "original arena load arguments");
            events.emplace_back("arena_load");
            break;
        }
        default:
            Check(false, "unexpected original NewGame callback");
        }
    }

    std::vector<std::string> events;
};

class MusicRuntime final : public pal4::IScriptMusicTransportRuntime {
public:
    std::uint32_t BackgroundMusicPlayer() override { return 0x1000u; }
    std::uint8_t StopSound(std::uint32_t slot) override {
        Check(slot == 0x1030u, "original script music pause slot");
        ++pauses;
        return 1u;
    }
    std::uint8_t LoadAndPlay(std::uint32_t) override { return 1u; }
    int pauses{};
};

class SceneMusicRuntime final : public pal4::IBackgroundMusicRuntime {
public:
    std::string path;
    unsigned stops{};
    unsigned plays{};

    std::string_view DefaultTrack() override { return {}; }
    void InitializeScript() override { Check(false, "unexpected music script init"); }
    std::uint32_t CreateInterpreter(std::string_view, std::string_view,
                                    std::string_view) override {
        Check(false, "unexpected music interpreter");
        return 0u;
    }
    bool ExecuteInterpreter(std::uint32_t,
                            pal4::BackgroundMusicPlayer&) override {
        Check(false, "unexpected music interpreter execution");
        return false;
    }
    void HandleException(std::uint32_t) override { Check(false, "unexpected music exception"); }
    void ReleaseException(std::uint32_t) override { Check(false, "unexpected music release"); }
    void CleanupScripts() override { Check(false, "unexpected music cleanup"); }
    std::string_view MusicDirectory() override { return "gamedata\\Music\\"; }
    std::uint32_t PlaySound(std::string_view full_path, bool restore,
                            bool on_script_finish) override {
        Check(!restore && on_script_finish, "Q01 script music playback flags");
        path = full_path;
        ++plays;
        return 7u;
    }
    std::uint32_t FadeInAudio(std::uint32_t, float) override {
        Check(false, "Q01 music has no fade in");
        return 0u;
    }
    std::uint32_t FadeOutAudio(std::uint32_t handle, float seconds) override {
        Check(handle == 0u && seconds == 1.0f,
              "Q01 music stops the previous track with a one-second fade");
        ++stops;
        return 1u;
    }
    std::uint32_t StopAudioImmediately(std::uint32_t) override {
        Check(false, "Q01 music uses a fade out");
        return 0u;
    }
    bool IsAudioActive(std::uint32_t) override { Check(false, "unexpected music active check"); return false; }
    std::uint8_t PauseAudio(std::uint32_t) override { Check(false, "unexpected music pause"); return 0u; }
    std::uint32_t SetAudioProperty(std::uint32_t, std::uint32_t) override {
        Check(false, "unexpected music property");
        return 0u;
    }
    std::uint8_t SavedTextAssignmentResult(std::string_view, bool) override {
        Check(false, "unexpected saved music assignment");
        return 0u;
    }
    std::uint32_t CleanupTemporaryText() override { Check(false, "unexpected music text cleanup"); return 0u; }
    std::uint32_t CleanupTemporaryPath() override { return 1u; }
};

class SceneCameraRuntime final : public pal4::IScriptCameraCallbackRuntime {
public:
    void AdjustYpr(float, float, float, std::uint8_t) override {
        throw std::logic_error("unexpected scene camera angle control");
    }
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
    pal4::PlayerManagerObjectState player_state{};

    std::uint32_t CameraSubsystem() override { return 0x1000u; }
    std::uint32_t CameraByName(std::uint32_t manager,
                               std::string_view name) override {
        Check(manager == 0x1000u, "Q01 camera subsystem");
        if (name == "MainCamera") return 0x2000u;
        if (name == "ScriptCamera") return 0x3000u;
        Check(false, "unexpected Q01 camera name");
        return 0u;
    }
    std::uint32_t CameraInternalId(std::uint32_t camera) override {
        Check(camera == 0x2000u || camera == 0x3000u,
              "Q01 camera object");
        return camera + 0x2000u;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        writes.emplace_back(address, value);
    }
    std::uint32_t ActiveCameraObject(std::uint32_t) override {
        throw std::logic_error("unexpected active camera request");
    }
    void SetLookAt(std::uint32_t, std::uint32_t) override {
        throw std::logic_error("unexpected camera look-at request");
    }
    std::uint32_t ActiveCameraInternalId(std::uint32_t) override {
        throw std::logic_error("unexpected active camera ID request");
    }
    float ReadFloat(std::uint32_t) override {
        throw std::logic_error("unexpected camera float request");
    }
    std::uint32_t AudioManager() override {
        throw std::logic_error("unexpected camera audio request");
    }
    std::uint32_t ReadWord(std::uint32_t) override {
        throw std::logic_error("unexpected camera word request");
    }
    void SetListenerPosition(std::uint32_t, float, float, float) override {
        throw std::logic_error("unexpected listener position request");
    }
    std::uint32_t SetListenerOrientation(std::uint32_t, float, float, float,
                                         float, float, float) override {
        throw std::logic_error("unexpected listener orientation request");
    }
    pal4::PlayerManagerObjectState& PlayerManagerState() override {
        return player_state;
    }
};

class ScenePlayerRuntime final : public pal4::IScriptPlayerActionsRuntime {
public:
    std::vector<std::int32_t> leader_indices;

    std::uint32_t PlayerManager() override { return 0x7000u; }
    std::uint32_t UpdatePlayerStates(std::uint32_t manager,
                                     std::int32_t index) override {
        Check(manager == 0x7000u, "Q01 player manager");
        leader_indices.push_back(index);
        return 1u;
    }
    std::uint32_t PlayerByIndex(std::uint32_t, std::int32_t) override {
        throw std::logic_error("unexpected Q01 player lookup");
    }
    std::uint32_t ArenaManager() override {
        throw std::logic_error("unexpected Q01 arena request");
    }
    std::uint32_t ScriptOnRemove(std::uint32_t, std::uint32_t) override {
        throw std::logic_error("unexpected Q01 player remove");
    }
    std::uint32_t ScriptOnAdd(std::uint32_t, std::uint32_t) override {
        throw std::logic_error("unexpected Q01 player add");
    }
    void SetStateFlag(std::uint32_t, std::uint32_t) override {
        throw std::logic_error("unexpected Q01 player state flag");
    }
    std::uint32_t ReadWord(std::uint32_t) override {
        throw std::logic_error("unexpected Q01 player read");
    }
    void WriteWord(std::uint32_t, std::uint32_t) override {
        throw std::logic_error("unexpected Q01 player write");
    }
    std::uint32_t MovementAdapter() override {
        throw std::logic_error("unexpected Q01 movement adapter");
    }
    std::uint32_t SetClumpProperties(std::uint32_t, std::uint32_t,
                                     std::uint32_t, std::uint32_t,
                                     std::uint32_t, std::uint32_t) override {
        throw std::logic_error("unexpected Q01 player clump properties");
    }
    std::uint32_t SetClumpRenderCallback(std::uint32_t,
                                         std::uint32_t) override {
        throw std::logic_error("unexpected Q01 player clump callback");
    }
};

class ScenePlayerLockRuntime final : public pal4::IScriptPlayerLockRuntime {
public:
    std::vector<std::string> events;

    std::uint32_t PlayerManager() override { return 0x7000u; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0x7000u + 0x2c50u) return 0u;
        if (address == 0x8004u) return 0x9000u;
        throw std::logic_error("unexpected Q01 player-lock read");
    }
    void SetActionCode(std::uint32_t component,
                       std::string_view code) override {
        Check(component == 0x7170u && code == "C01",
              "Q01 lock selects the current player's C01 action");
        events.emplace_back("action:C01");
    }
    std::uint32_t ActorControl() override { return 0x8000u; }
    void ResetAndSync(std::uint32_t control,
                      std::uint32_t flag) override {
        Check(control == 0x8000u, "Q01 lock uses ActorControl");
        events.push_back("reset:" + std::to_string(flag));
    }
    std::uint32_t UiFrameManager() override { return 0xa000u; }
    std::uint8_t ProcessUiEvents(std::uint32_t manager,
                                 std::uint8_t flag) override {
        Check(manager == 0xa000u, "Q01 lock uses UI manager");
        events.push_back("ui:" + std::to_string(flag));
        return 1u;
    }
    std::uint32_t MinimapManager() override { return 0xb000u; }
    void UpdateGrid(std::uint32_t manager, std::uint32_t position,
                    std::uint32_t direction) override {
        Check(manager == 0xb000u && position == 0x90acu &&
                  direction == 0x90a0u, "Q01 unlock minimap operands");
        events.emplace_back("grid");
    }
};

class ScenePlayerPositionRuntime final : public pal4::IScriptPlayerPositionRuntime {
public:
    pal4::PlayerPositionWaitState wait{};
    std::vector<std::string> events;

    std::uint32_t PlayerManager() override { return 0x7000u; }
    std::uint32_t PlayerByIndex(std::uint32_t, std::int32_t) override {
        throw std::logic_error("unexpected indexed player position");
    }
    void SetPosition(std::uint32_t actor,
                     const pal4::PlayerPosition& value) override {
        Check(actor == 0x9000u && value.x == 7.0f && value.y == 7.0f &&
                  value.z == -22.0f,
              "Q01 current-player position arguments");
        events.emplace_back("position");
    }
    std::uint32_t CurrentPlayer(std::uint32_t manager) override {
        Check(manager == 0x7000u, "Q01 current-player manager");
        return 0x9000u;
    }
    std::string_view ActorName(std::uint32_t actor) override {
        Check(actor == 0x9000u, "Q01 current-player name receiver");
        events.emplace_back("name");
        return "101";
    }
    std::uint32_t NpcManager() override {
        throw std::logic_error("unexpected Q01 NPC position");
    }
    std::uint32_t NpcByName(std::uint32_t,
                            std::string_view) override {
        throw std::logic_error("unexpected Q01 NPC lookup");
    }
    void SetNpcPosition(std::uint32_t, const pal4::PlayerPosition&,
                        std::uint32_t) override {
        throw std::logic_error("unexpected Q01 NPC position write");
    }
    std::int32_t SetNpcAction(std::uint32_t,
                              std::string_view) override {
        throw std::logic_error("unexpected Q01 NPC action");
    }
    float MovementProperty(std::uint32_t actor) override {
        Check(actor == 0x9000u, "Q01 movement property receiver");
        events.emplace_back("property");
        return 10.0f;
    }
    std::uint32_t MovementAdapter() override { return 0x8000u; }
    void SelectAndUpdate(std::uint32_t adapter, std::uint32_t movement,
                         float first, float second,
                         std::uint32_t mode) override {
        Check(adapter == 0x8000u && movement == 0x90acu && first == 3.5f &&
                  second == 25.3f && mode == 1u,
              "Q01 movement selection arguments");
        events.emplace_back("select");
    }
    bool CheckActive(std::uint32_t adapter, std::uint32_t movement,
                     float first, float second,
                     std::uint32_t mode) override {
        Check(adapter == 0x8000u && movement == 0x90acu && first == 3.5f &&
                  second == 25.3f && mode == 0u,
              "Q01 movement active arguments");
        events.emplace_back("active");
        return true;
    }
    void UpdateFloorColor() override { events.emplace_back("floor"); }
    void UpdateGrid(std::uint32_t position,
                    std::uint32_t context) override {
        Check(position == 0x90acu && context == 0x90a0u,
              "Q01 minimap position arguments");
        events.emplace_back("grid");
    }
    void SetAction(std::uint32_t actor,
                   std::string_view action) override {
        Check(actor == 0x9000u && action == "C01",
              "Q01 current-player idle action");
        events.emplace_back("action");
    }
    pal4::PlayerPositionWaitState* Allocate(std::uint32_t size) override {
        Check(size == sizeof(wait), "Q01 position wait allocation");
        events.emplace_back("allocate");
        return &wait;
    }
    std::uint32_t ScriptExecutor() override { return 0xa000u; }
    void Construct(pal4::PlayerPositionWaitState& state,
                   std::uint32_t mode, std::uint32_t queue) override {
        Check(&state == &wait && mode == 0u && queue == 0xa054u,
              "Q01 position wait constructor");
        events.emplace_back("construct");
    }
    void AssignName(pal4::PlayerPositionWaitState& state,
                    std::string_view name) override {
        Check(&state == &wait && name == "101-Player-Movment" &&
                  state.vtable_address == pal4::ScriptPlayerPosition::kVtable &&
                  state.enabled == 1u && state.phase == 1u,
              "Q01 position wait name and state");
        events.emplace_back("assign");
    }
    void AddHandler(std::uint32_t queue,
                    pal4::PlayerPositionWaitState* state) override {
        Check(queue == 0xa054u && state == &wait,
              "Q01 position wait queue");
        events.emplace_back("add");
    }
    std::uint32_t CleanupName() override {
        events.emplace_back("cleanup");
        return 1u;
    }
};

class ScenePlayerRotationRuntime final : public pal4::IScriptPlayerRotationRuntime {
public:
    pal4::PlayerRotationWaitState wait{};
    std::vector<std::string> events;

    std::uint32_t PlayerManager() override { return 0x7000u; }
    std::uint32_t PlayerByIndex(std::uint32_t, std::int32_t) override {
        throw std::logic_error("unexpected indexed Q01 rotation");
    }
    std::uint32_t CurrentPlayer(std::uint32_t manager) override {
        Check(manager == 0x7000u, "Q01 rotation player manager");
        return 0x9000u;
    }
    std::uint32_t NpcManager() override {
        throw std::logic_error("unexpected Q01 NPC rotation");
    }
    std::uint32_t NpcByName(std::uint32_t, std::string_view) override {
        throw std::logic_error("unexpected Q01 NPC rotation lookup");
    }
    std::string_view ActorName(std::uint32_t actor) override {
        Check(actor == 0x9000u, "Q01 rotation actor name receiver");
        events.emplace_back("name");
        return "101";
    }
    float ReadX(std::uint32_t actor) override {
        Check(actor == 0x9000u, "Q01 rotation X receiver");
        events.emplace_back("read-x");
        return 7.0f;
    }
    float ReadZ(std::uint32_t actor) override {
        Check(actor == 0x9000u, "Q01 rotation Z receiver");
        events.emplace_back("read-z");
        return -22.0f;
    }
    void SetRotation(std::uint32_t actor,
                     const pal4::PlayerRotation& rotation,
                     std::uint32_t mode) override {
        Check(actor == 0x9000u && rotation.x == 7.0f &&
                  rotation.yaw == 180.0f && rotation.z == -22.0f &&
                  mode == 0u,
              "Q01 current-player angle arguments");
        events.emplace_back("rotation");
    }
    pal4::PlayerRotationWaitState* Allocate(std::uint32_t size) override {
        Check(size == sizeof(wait), "Q01 rotation wait allocation");
        events.emplace_back("allocate");
        return &wait;
    }
    std::uint32_t ScriptExecutor() override { return 0xa000u; }
    void Construct(pal4::PlayerRotationWaitState& state,
                   std::uint32_t mode, std::uint32_t queue) override {
        Check(&state == &wait && mode == 0u && queue == 0xa054u,
              "Q01 rotation wait constructor");
        events.emplace_back("construct");
    }
    bool AllocateText(pal4::PlayerRotationWaitState& state,
                      std::uint32_t size) override {
        Check(&state == &wait && size == 18u &&
                  state.vtable_address == pal4::ScriptPlayerRotation::kVtable &&
                  state.enabled == 1u && state.phase == 2u,
              "Q01 rotation wait text allocation");
        events.emplace_back("allocate-text");
        return true;
    }
    void AssignName(pal4::PlayerRotationWaitState& state,
                    std::string_view name) override {
        Check(&state == &wait && name == "101-Player-Movment",
              "Q01 rotation wait name");
        events.emplace_back("assign");
    }
    void Close(pal4::PlayerRotationWaitState& state) override {
        Check(&state == &wait, "Q01 rotation wait close receiver");
        events.emplace_back("close");
    }
    void AddHandler(std::uint32_t queue,
                    pal4::PlayerRotationWaitState* state) override {
        Check(queue == 0xa054u && state == &wait,
              "Q01 rotation wait queue");
        events.emplace_back("add");
    }
    std::uint32_t CleanupName() override {
        events.emplace_back("cleanup");
        return 1u;
    }
};

class SceneArenaReadyRuntime final : public pal4::IGameArenaReadyRuntime {
public:
    std::unordered_map<std::uint32_t, std::uint32_t> writes;
    std::vector<std::string> events;

    std::uint32_t ActorControl() override { return 0x8000u; }
    std::uint32_t ArenaManager() override { return 0xc000u; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (const auto found = writes.find(address); found != writes.end())
            return found->second;
        if (address == 0x8000u) return 1u;
        if (address == 0xc0c4u) return 0u;
        if (address == 0x8004u) return 0x9000u;
        throw std::logic_error("unexpected Q01 arena-ready read");
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        writes[address] = value;
    }
    std::string_view ReadText(std::uint32_t address) override {
        if (address == 0xc008u) return "Q01";
        if (address == 0xc0e0u) return "N01";
        throw std::logic_error("unexpected Q01 arena-ready text");
    }
    std::uint32_t GameDatabase() override { return 0xd000u; }
    std::span<const pal4::ArenaReadyHintRecord> HintRecords(
        std::uint32_t member) override {
        Check(member == 0xd034u, "Q01 arena-ready hint catalog");
        events.emplace_back("hints");
        return {};
    }
    std::uint32_t CreateHintWindow() override {
        throw std::logic_error("unexpected Q01 hint window");
    }
    void ShowHint(std::uint32_t, std::string_view, float, float, float,
                  std::uint32_t, std::uint32_t) override {
        throw std::logic_error("unexpected Q01 hint display");
    }
    std::int32_t ReleaseHintMatches() override {
        events.emplace_back("release-hints");
        return 0;
    }
    void ProcessActorInput(std::uint32_t actor) override {
        Check(actor == 0x8000u, "Q01 ready actor input");
        events.emplace_back("actor-input");
    }
    void ProcessActorInputAndClearVector(std::uint32_t actor) override {
        Check(actor == 0x8000u, "Q01 ready actor vector");
        events.emplace_back("clear-vector");
    }
    std::uint32_t InputManager() override { return 0xf000u; }
    void InitializeInputState(std::uint32_t input) override {
        Check(input == 0xf000u, "Q01 ready input manager");
        events.emplace_back("input");
    }
    std::uint32_t PetManager() override { return 0xe000u; }
    void InitializePetData(std::uint32_t pet) override {
        Check(pet == 0xe000u, "Q01 ready pet manager");
        events.emplace_back("pet");
    }
    std::uint32_t MinimapManager() override { return 0xb000u; }
    void UpdateGrid(std::uint32_t minimap, std::uint32_t position,
                    std::uint32_t direction) override {
        Check(minimap == 0xb000u && position == 0x90acu &&
                  direction == 0x90a0u, "Q01 ready minimap operands");
        events.emplace_back("grid");
    }
    float LeaderY() override {
        throw std::logic_error("unexpected Q01 special minimap Y");
    }
    float LeaderZ() override {
        throw std::logic_error("unexpected Q01 special minimap Z");
    }
    void SetMinimapTexture(std::uint32_t) override {
        throw std::logic_error("unexpected Q01 special minimap texture");
    }
    std::uint32_t Milliseconds() override { return 123u; }
    std::uint32_t GameState() override { return 0x11000u; }
};

class SceneSystemUiRuntime final : public pal4::IScriptSystemUiRuntime {
public:
    std::vector<std::pair<std::uint32_t, std::uint8_t>> entries;

    std::uint32_t Game() override { return 0x11000u; }
    void SetGameDataEntry(std::uint32_t game, std::uint32_t entry,
                          std::uint8_t value) override {
        Check(game == 0x11000u, "Q01 system UI game owner");
        entries.emplace_back(entry, value);
    }
    void InitializeStateAndUi(std::uint32_t, std::uint32_t,
                              std::uint32_t) override {
        throw std::logic_error("unexpected Q01 system UI state change");
    }
    std::uint32_t UiFrameManager() override {
        throw std::logic_error("unexpected Q01 system UI manager");
    }
    void ProcessEvent(std::uint32_t, std::uint32_t, std::uint32_t,
                      std::uint32_t) override {
        throw std::logic_error("unexpected Q01 system UI event");
    }
    void SetFrameFlag(std::uint32_t, std::uint8_t) override {
        throw std::logic_error("unexpected Q01 system UI frame flag");
    }
};

class ScenePlayerActionRuntime final : public pal4::IScriptPlayerActionRuntime {
public:
    pal4::PlayerActionWaitState wait{};
    std::vector<std::string> events;

    std::uint32_t PlayerManager() override { return 0x7000u; }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::int32_t index) override {
        Check(manager == 0x7000u && index == 0,
              "Q01 indexed action player");
        events.emplace_back("player");
        return 0x9000u;
    }
    std::uint32_t CurrentPlayer(std::uint32_t) override {
        throw std::logic_error("unexpected Q01 current action player");
    }
    std::uint32_t NpcManager() override {
        throw std::logic_error("unexpected Q01 action NPC manager");
    }
    std::uint32_t NpcByName(std::uint32_t, std::string_view) override {
        throw std::logic_error("unexpected Q01 action NPC lookup");
    }
    std::int32_t SetAction(std::uint32_t actor,
                           std::string_view action) override {
        Check(actor == 0x9000u && action == "J08",
              "Q01 first player action");
        events.emplace_back("action:J08");
        return 1;
    }
    void SetFlag(std::uint32_t actor, std::uint32_t flag) override {
        Check(actor == 0x9000u && flag == 1u,
              "Q01 first action flag");
        events.emplace_back("flag:1");
    }
    void ClearFlag(std::uint32_t, std::uint32_t) override {
        throw std::logic_error("unexpected Q01 action flag clear");
    }
    bool GetFlag(std::uint32_t) override {
        throw std::logic_error("unexpected Q01 action flag read");
    }
    std::string_view ActionName(std::uint32_t) override {
        throw std::logic_error("unexpected Q01 action name read");
    }
    void ApplyEquipmentVisuals(std::uint32_t) override {
        throw std::logic_error("unexpected Q01 equipment visuals");
    }
    void ClearWeaponEffects(std::uint32_t actor) override {
        Check(actor == 0x9000u, "Q01 action weapon effects receiver");
        events.emplace_back("clear-weapons");
    }
    void UpdateEffectList(std::uint32_t) override {
        throw std::logic_error("unexpected Q01 action effect list");
    }
    void UpdateEffectList2(std::uint32_t) override {
        throw std::logic_error("unexpected Q01 action effect list 2");
    }
    void UpdateEffectList3(std::uint32_t actor) override {
        Check(actor == 0x9000u, "Q01 action effect list 3 receiver");
        events.emplace_back("effects:3");
    }
    void UpdateEffectList4(std::uint32_t) override {
        throw std::logic_error("unexpected Q01 action effect list 4");
    }
    void CleanupWeaponEffects(std::uint32_t) override {
        throw std::logic_error("unexpected Q01 action cleanup effects");
    }
    std::string_view FormatActorName(std::uint32_t actor) override {
        Check(actor == 0x9000u, "Q01 action actor label receiver");
        events.emplace_back("name");
        return "101-Player-Movment";
    }
    pal4::PlayerActionWaitState* AllocateAction(
        std::uint32_t size) override {
        Check(size == sizeof(wait), "Q01 action wait allocation");
        events.emplace_back("allocate");
        return &wait;
    }
    pal4::PlayerEndActionWaitState* AllocateEnd(
        std::uint32_t) override {
        throw std::logic_error("unexpected Q01 end-action allocation");
    }
    std::uint32_t ScriptExecutor() override { return 0xa000u; }
    void ConstructAction(pal4::PlayerActionWaitState& state,
                         std::uint8_t mode,
                         std::uint32_t queue) override {
        Check(&state == &wait && mode == 1u && queue == 0xa054u,
              "Q01 action wait mode and queue");
        events.emplace_back("construct");
    }
    void ConstructEnd(pal4::PlayerEndActionWaitState&,
                      std::uint32_t, std::uint32_t) override {
        throw std::logic_error("unexpected Q01 end-action constructor");
    }
    void AssignName(pal4::PlayerActionWaitState& state,
                    std::string_view name) override {
        Check(&state == &wait && name == "101-Player-Movment" &&
                  state.vtable_address == pal4::ScriptPlayerAction::kActionVtable &&
                  state.actor == 0x9000u && state.execute_flag == -1 &&
                  state.phase == 3u && state.completed_loops == 0u,
              "Q01 action wait state");
        events.emplace_back("assign");
    }
    void FlushAction(pal4::PlayerActionWaitState& state) override {
        Check(&state == &wait, "Q01 action flush receiver");
        events.emplace_back("flush");
    }
    void FlushEnd(pal4::PlayerEndActionWaitState&) override {
        throw std::logic_error("unexpected Q01 end-action flush");
    }
    void AddAction(std::uint32_t queue,
                   pal4::PlayerActionWaitState* state) override {
        Check(queue == 0xa054u && state == &wait,
              "Q01 action wait registration");
        events.emplace_back("add");
    }
    std::uint32_t AddEnd(std::uint32_t,
                         pal4::PlayerEndActionWaitState*) override {
        throw std::logic_error("unexpected Q01 end-action registration");
    }
    std::uint32_t CleanupName() override {
        events.emplace_back("cleanup");
        return 1u;
    }
    void CloseAction(pal4::PlayerActionWaitState&) override {
        throw std::logic_error("unexpected Q01 action close");
    }
    void CloseEnd(pal4::PlayerEndActionWaitState&) override {
        throw std::logic_error("unexpected Q01 end-action close");
    }
};

class ScenePlayerEquipmentRuntime final
    : public pal4::IScriptPlayerEquipmentRuntime {
public:
    pal4::InventoryItemSlotsState slots{};
    std::vector<std::uint32_t> requested;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
    std::uint32_t current_id{};
    std::uint32_t refreshes{};

    std::uint32_t PlayerManager() override { return 0x7000u; }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::int32_t index) override {
        Check(manager == 0x7000u && index == 0,
              "Q01 equipment player index");
        return 0x9000u;
    }
    std::uint32_t GetGameDatabase() override { return 0xd000u; }
    std::uint8_t IsEquipment(std::uint32_t table,
                             std::uint32_t id) override {
        Check(table == 0xd058u && (id == 3586u || id == 3501u),
              "Q01 equipment catalog request");
        requested.push_back(id);
        return 1u;
    }
    std::uint32_t GetInventory() override { return 0xe000u; }
    std::uint32_t ProcessInventoryCollection(
        std::uint32_t collection, std::uint32_t id) override {
        Check(collection == 0xe07cu && id == requested.back(),
              "Q01 equipment inventory request");
        current_id = id;
        return 0x5000u;
    }
    pal4::InventoryItemSlotsState ReadEquipmentSlots(
        std::uint32_t address) override {
        Check(address == 0x9920u, "Q01 equipment slot address");
        return slots;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        if (address == 0x5190u) return current_id;
        if (address == 0x5094u) return 1u;
        throw std::logic_error("unexpected Q01 equipment record read");
    }
    std::uint32_t ProcessGameDatabaseArray(
        std::uint32_t table, std::uint32_t value) override {
        Check(table == 0xd058u && value == 1u,
              "Q01 equipment category lookup");
        return 4u;
    }
    std::uint32_t FindInventoryEntry(std::uint32_t, std::uint32_t,
                                     std::uint32_t) override {
        throw std::logic_error("unexpected Q01 equipment entry lookup");
    }
    std::uint32_t ProcessEntry(std::uint32_t, std::uint32_t,
                               std::uint32_t) override {
        throw std::logic_error("unexpected Q01 equipment entry process");
    }
    void InsertEntry(std::uint32_t, std::uint32_t) override {
        throw std::logic_error("unexpected Q01 equipment entry insertion");
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check((address == 0x9930u || address == 0x9934u) &&
                  value == current_id,
              "Q01 equipment slot write");
        slots.ids[(address - 0x9920u) / 4u] = value;
        writes.emplace_back(address, value);
    }
    std::uint8_t RefreshEquipmentState(
        std::uint32_t player_state) override {
        Check(player_state == 0x9654u,
              "Q01 equipment player refresh receiver");
        ++refreshes;
        return 1u;
    }
};

class SceneCameraSequenceRuntime final
    : public pal4::IScriptCameraSequenceRuntime {
public:
    pal4::CameraSequenceWaitState wait{};
    std::vector<std::string> events;
    bool circle{},has_cloud{true};
    std::uint8_t expected_wait_mode{1u};

    std::uint32_t CameraSubsystem() override { return 0x1000u; }
    std::uint32_t ActiveCameraObject(std::uint32_t subsystem) override {
        Check(subsystem == 0x1000u, "Q01 camera sequence subsystem");
        return 0x2000u;
    }
    void ClearCameraCollection(std::uint32_t camera) override {
        Check(camera == 0x2000u, "Q01 camera sequence collection");
        events.emplace_back("clear");
    }
    std::string_view ArenaScene() override { return "N01"; }
    std::string_view ArenaWorld() override { return "Q01"; }
    std::uint32_t CloudManager() override { return 0x3000u; }
    std::uint32_t LoadCameraFile(
        std::uint32_t manager, std::string_view world,
        std::string_view scene, std::string_view name) override {
        Check(manager == 0x3000u && world == "Q01" && scene == "N01" &&
                  name == "MC001", "Q01 camera file request");
        events.emplace_back("load:MC001");
        return 1u;
    }
    std::uint32_t FindCloud(std::uint32_t manager,
                            std::string_view name) override {
        Check(manager == 0x3000u && name == "001",
              "Q01 prepared camera record lookup");
        events.emplace_back("find:001");
        return 0x4000u;
    }
    std::uint32_t FirstCloud(std::uint32_t) override {
        Check(circle,"circle lookup requires the circle callback");
        return has_cloud ? 0x4000u : 0u;
    }
    std::uint32_t NextCloud(std::uint32_t) override {
        Check(circle,"circle advances prepared records");return 0u;
    }
    pal4::CameraSequenceControl ReadCloudControl(
        std::uint32_t) override {
        Check(circle,"circle reads record control");return {7u,0u};
    }
    pal4::CameraSequenceWaitState* AllocateWait(
        std::uint32_t size) override {
        Check(size == sizeof(wait), "Q01 camera wait allocation");
        events.emplace_back("allocate");
        return &wait;
    }
    std::uint32_t ScriptExecutor() override { return 0xa000u; }
    void ConstructWait(pal4::CameraSequenceWaitState& state,
                       std::uint8_t mode, std::uint32_t queue) override {
        Check(&state == &wait && mode == expected_wait_mode && queue == 0xa054u,
              "Q01 camera wait mode");
        events.emplace_back("construct");
    }
    std::uint32_t CreateCameraTask() override {
        events.emplace_back("task");
        return 0x5000u;
    }
    std::uint32_t TaskContainer(std::uint32_t camera) override {
        Check(camera == 0x2000u, "Q01 camera task container receiver");
        events.emplace_back("container");
        return 0x6000u;
    }
    void AddCameraTask(
        std::uint32_t container, std::uint32_t task,
        std::string_view name,
        const std::array<std::uint32_t, 3>& options) override {
        Check(container == 0x6000u && task == 0x5000u &&
                  name == "synchroCameraRun-CLD" &&
                  options == (circle ? std::array<std::uint32_t,3>{1u,0u,7u} : std::array<std::uint32_t,3>{3u,0u,0u}),
              "Q01 single camera task configuration");
        events.emplace_back("add-task");
    }
    std::uint32_t CameraInternalId(std::uint32_t camera) override {
        Check(camera == 0x2000u, "Q01 internal camera receiver");
        events.emplace_back("camera-id");
        return 0x7000u;
    }
    void ApplyCameraRecord(std::uint32_t task,
                           std::uint32_t camera_id,
                           std::uint32_t record) override {
        Check(task == 0x5000u && camera_id == 0x7000u && record == 0x4000u,
              "Q01 single camera record application");
        events.emplace_back("apply");
    }
    void MarkProcessed(std::uint32_t camera) override {
        Check(camera == 0x2000u, "Q01 processed camera receiver");
        events.emplace_back("processed");
    }
    void FlushWait(pal4::CameraSequenceWaitState& state) override {
        Check(&state == &wait &&
                  state.vtable_address ==
                      (circle ? pal4::ScriptCameraSequences::kCircleVtable : pal4::ScriptCameraSequences::kSingleVtable),
              "Q01 camera wait handler image");
        events.emplace_back("flush");
    }
    void CloseWait(pal4::CameraSequenceWaitState&) override {
        throw std::logic_error("unexpected Q01 camera wait close");
    }
    std::uint32_t AddWait(std::uint32_t queue,
                          pal4::CameraSequenceWaitState* state) override {
        Check(queue == 0xa054u && state == &wait,
              "Q01 camera wait queue");
        events.emplace_back("add-wait");
        return 1u;
    }
    bool CameraCompleted(std::uint32_t) override {
        throw std::logic_error("unexpected Q01 camera completion check");
    }
    pal4::CameraSequenceVector ReadVector(std::uint32_t) override {
        throw std::logic_error("unexpected Q01 camera vector read");
    }
    std::uint32_t AudioManager() override {
        throw std::logic_error("unexpected Q01 camera audio manager");
    }
    std::uint32_t AudioHandle(std::uint32_t) override {
        throw std::logic_error("unexpected Q01 camera audio handle");
    }
    void SetListenerPosition(std::uint32_t,
                             pal4::CameraSequenceVector) override {
        throw std::logic_error("unexpected Q01 camera audio position");
    }
    std::uint32_t SetListenerOrientation(
        std::uint32_t, pal4::CameraSequenceVector,
        pal4::CameraSequenceVector) override {
        throw std::logic_error("unexpected Q01 camera audio direction");
    }
};

class SceneSound2dRuntime final : public pal4::IScriptSound2dRuntime {
public:
    std::vector<std::string> played_names;
    std::uint32_t current_handle{};

    std::string_view SoundName(std::uint32_t source) override {
        Check(source != 0u, "Q01 sound script-string source");
        const auto* name = reinterpret_cast<const asCScriptString*>(
            static_cast<std::uintptr_t>(source));
        return name->buffer;
    }
    std::uint32_t AudioManager() override { return 0xb000u; }
    std::uint32_t PlaySound(std::uint32_t audio,
                            std::uint32_t source,
                            std::string_view path,
                            std::int32_t mode) override {
        Check(audio == 0xb000u && SoundName(source) == "WJ001" &&
                  path == "gamedata\\PALSound\\WJ001.wav" && mode == 1,
              "Q01 first 2D sound request");
        played_names.emplace_back(SoundName(source));
        return 0x1234u;
    }
    std::uint32_t ScriptExecutor() override { return 0xa000u; }
    std::int32_t InsertSoundId(std::uint32_t,
                               std::uint32_t) override {
        throw std::logic_error("unexpected Q01 sound ID insertion");
    }
    void SetCurrentHandle(std::uint32_t address,
                          std::uint32_t handle) override {
        Check(address == 0xa020u && handle == 0x1234u,
              "Q01 current sound handle");
        current_handle = handle;
    }
    std::uint32_t CheckActive(std::uint32_t) override {
        throw std::logic_error("unexpected Q01 sound active check");
    }
    std::uint32_t StopCurrent(std::uint32_t) override {
        throw std::logic_error("unexpected Q01 sound stop");
    }
    std::uint32_t RemoveSoundId(std::uint32_t,
                                std::int32_t) override {
        throw std::logic_error("unexpected Q01 sound ID removal");
    }
};

class SceneWaitRuntime final : public pal4::IScriptWaitHandlerRuntime {
public:
    pal4::ScriptWaitHandlerState handler{};
    std::vector<std::string> events;
    double duration{};
    asIScriptContext* context{};
    unsigned closes{};

    pal4::ScriptWaitHandlerState* Allocate(std::uint32_t size) override {
        Check(size == pal4::ScriptWaitHandler::kSize,
              "Q01 timed wait allocation size");
        events.emplace_back("allocate");
        return &handler;
    }
    std::uint32_t ScriptExecutor() override { return 0xa000u; }
    void ConstructBase(pal4::ScriptWaitHandlerState& state,
                       std::uint32_t mode, std::uint32_t queue) override {
        Check(&state == &handler && mode == 1u && queue == 0xa054u,
              "Q01 timed wait base");
        events.emplace_back("construct");
    }
    void Flush(pal4::ScriptWaitHandlerState& state) override {
        Check(&state == &handler &&
                  state.vtable_address == pal4::ScriptWaitHandler::kVtable,
              "Q01 timed wait vtable");
        duration = state.remaining;
        events.emplace_back("flush");
    }
    void AddHandler(std::uint32_t queue,
                    pal4::ScriptWaitHandlerState* state) override {
        Check(queue == 0xa054u && state == &handler,
              "Q01 timed wait queue");
        events.emplace_back("add");
    }
    int SetExecutionFlag(std::uint32_t queue) override {
        Check(queue == 0xa054u, "Q01 timed wait execution flag");
        Check(context != nullptr && context->Suspend() >= 0,
              "Q01 timed wait suspends script context");
        events.emplace_back("flag");
        return 1;
    }
    void SetClosedFlag(pal4::ScriptWaitHandlerState& state) override {
        Check(&state == &handler && state.remaining == 0.0,
              "Q01 timed wait closes after countdown");
        ++closes;
    }
};

class SceneFlashRuntime final : public pal4::IScriptFlashRuntime {
public:
    pal4::FlashWaitState wait{};
    std::vector<std::string> events;

    std::uint32_t ScreenEffect() override { return 0x9000u; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x9030u, "Q01 flash effect active flag");
        return 1u;
    }
    void StartFade(std::uint32_t effect, float duration,
                   std::uint32_t hold, pal4::FlashColor color,
                   float alpha_start, float alpha_end,
                   float alpha_on_finish) override {
        Check(effect == 0x9000u && duration == 2.0f && hold == 0u &&
                  color.red == 0u && color.green == 0u && color.blue == 0u &&
                  alpha_start == 255.0f && alpha_end == 0.0f &&
                  alpha_on_finish == 0.0f,
              "Q01 first black fade-in arguments");
        events.emplace_back("fade-in");
    }
    pal4::FlashWaitState* Allocate(std::uint32_t bytes) override {
        Check(bytes == sizeof(wait), "Q01 flash wait allocation");
        events.emplace_back("allocate");
        return &wait;
    }
    std::uint32_t ScriptExecutor() override { return 0xa000u; }
    void Construct(pal4::FlashWaitState& state,
                   std::uint8_t mode, std::uint32_t queue) override {
        Check(&state == &wait && mode == 0u && queue == 0xa054u,
              "Q01 nonblocking flash wait");
        events.emplace_back("construct");
    }
    void Flush(pal4::FlashWaitState& state) override {
        Check(&state == &wait &&
                  state.vtable_address == pal4::ScriptFlashEffect::kWaitVtable,
              "Q01 flash wait handler image");
        events.emplace_back("flush");
    }
    void Close(pal4::FlashWaitState&) override {
        throw std::logic_error("unexpected Q01 flash wait close");
    }
    std::uint32_t AddHandler(std::uint32_t queue,
                             pal4::FlashWaitState* state) override {
        Check(queue == 0xa054u && state == &wait,
              "Q01 flash wait handler queue");
        events.emplace_back("add");
        return 1u;
    }
};

}  // namespace

int main(int argc, char** argv) {
    Check(argc == 2, "provide original PAL4 installation path");
    pal4::NativePackageArchive archive;
    const std::string package = std::string(argv[1]) + "\\gamedata\\script.cpk";
    Check(archive.Open(package.c_str()), "open original script.cpk");
    const auto member = archive.ReadMember("script.csb");
    Check(member && member->size() >= sizeof(std::uint32_t),
          "read original script.csb");
    std::uint32_t declared_size = 0;
    std::memcpy(&declared_size, member->data(), sizeof(declared_size));
    Check(declared_size == member->size() - sizeof(declared_size),
          "original CSB length header");

    const auto* payload = reinterpret_cast<const std::byte*>(
        member->data() + sizeof(declared_size));
    pal4::NativeAngelScriptEngine engine;
    constexpr char kModule[] = "gamedata\\editData\\script\\script.csb";
    const auto result = engine.LoadBytecode(kModule, {payload, declared_size});
    Check(result.code == 0 && result.bytes_read == declared_size,
          "AngelScript 2.2.0 restores the complete original CSB");
    const int new_game = engine.FunctionId(kModule, "NewGame");
    Check(engine.Raw()->GetFunctionCount(kModule) == 1 && new_game >= 0 &&
              std::strcmp(engine.Raw()->GetFunctionDeclaration(new_game),
                          "void NewGame()") == 0,
          "original NewGame entry is available through the public engine API");
    asIScriptContext* context = nullptr;
    Check(engine.Raw()->CreateContext(&context) == 0 && context != nullptr,
          "create original script execution context");
    Check(context->Prepare(new_game) >= 0,
          "prepare original NewGame bytecode");
    Check(context->Execute() == asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),
                          "PAL4 game script callback -291 is not bound") == 0,
          "unbound original game callback fails as a script exception");
    pal4::NativeGameScriptCallbacks native_callbacks({});
    engine.SetCallbacks(&native_callbacks);
    Check(context->Prepare(new_game) >= 0 &&
              context->Execute() == asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),
                          "PAL4 game script service -291 is unavailable") == 0,
          "original NewGame requires a native screen-effect service");
    pal4::PalScreenEffectState screen{};
    pal4::EffectRenderRuntimeStub render;
    (void)pal4::EffectRenderControl::Initialize(screen.render, 640, 480);
    screen.render.texture = 1u;
    pal4::NativeScriptFlashRuntime flash(screen, render, *context);
    MusicRuntime music;
    pal4::NativeGameScriptCallbacks timed_callbacks({&flash.Flash(), &music});
    engine.SetCallbacks(&timed_callbacks);
    Check(context->Prepare(new_game) >= 0 &&
              context->Execute() == asEXECUTION_SUSPENDED &&
              flash.Waiting() && music.pauses == 0,
          "original NewGame suspends on its first blocking black fade");
    Check(!flash.Tick(0.05f) && flash.Waiting() &&
              flash.Tick(0.05f) && !flash.Waiting(),
          "original black fade waits for its 0.1-second duration");
    Check(context->Execute() == asEXECUTION_EXCEPTION &&
              music.pauses == 1 &&
              std::strcmp(context->GetExceptionString(),
                          "PAL4 game script service -297 is unavailable") == 0,
          "original NewGame resumes through music then reaches movie service");
    RecordingCallbacks callbacks;
    engine.SetCallbacks(&callbacks);
    Check(context->Prepare(new_game) >= 0 &&
              context->Execute() == asEXECUTION_FINISHED,
          "execute original NewGame through registered callback boundary");
    Check(callbacks.events == std::vector<std::string>{
              "flash", "music_pause", "movie:1A.bik", "movie:Pal4A.bik",
              "movie_flag", "new_game", "music_resume", "arena_load"} &&
              engine.GlobalStoryVariable() == 10100,
          "original NewGame actions and story state follow bytecode order");
    (void)context->Release();

    const auto scene_script = archive.ReadMember("Q01.csb");
    Check(scene_script && scene_script->size() >= sizeof(std::uint32_t),
          "read original Q01 scene script");
    std::uint32_t scene_size{};
    std::memcpy(&scene_size, scene_script->data(), sizeof(scene_size));
    Check(scene_size == scene_script->size() - sizeof(scene_size),
          "original Q01 CSB length header");
    constexpr char kSceneModule[] = "gamedata\\editData\\script\\Q01.csb";
    const auto scene_result = engine.LoadBytecode(
        kSceneModule,
        {reinterpret_cast<const std::byte*>(
             scene_script->data() + sizeof(scene_size)), scene_size});
    Check(scene_result.code == 0 && scene_result.bytes_read == scene_size,
          "AngelScript restores original Q01 scene bytecode");
    const int scene_init = engine.FunctionId(kSceneModule, "Q01_N01_init");
    Check(scene_init >= 0 &&
              std::strcmp(engine.Raw()->GetFunctionDeclaration(scene_init),
                          "void Q01_N01_init()") == 0,
          "original Q01/N01 scene-init entry is available");
    pal4::BackgroundMusicPlayer scene_music_player;
    SceneMusicRuntime scene_music_runtime;
    SceneCameraRuntime scene_camera_runtime;
    ScenePlayerRuntime scene_player_runtime;
    ScenePlayerLockRuntime scene_player_lock_runtime;
    ScenePlayerPositionRuntime scene_player_position_runtime;
    ScenePlayerRotationRuntime scene_player_rotation_runtime;
    SceneArenaReadyRuntime scene_arena_ready_runtime;
    SceneSystemUiRuntime scene_system_ui_runtime;
    ScenePlayerActionRuntime scene_player_action_runtime;
    ScenePlayerEquipmentRuntime scene_player_equipment_runtime;
    SceneCameraSequenceRuntime scene_camera_sequence_runtime;
    SceneSound2dRuntime scene_sound_runtime;
    SceneWaitRuntime scene_wait_runtime;
    SceneFlashRuntime scene_flash_runtime;
    pal4::ScriptFlashEffect scene_flash(scene_flash_runtime);
    pal4::NativeGameScriptServices scene_services{};
    scene_services.background_music_player = &scene_music_player;
    scene_services.background_music_runtime = &scene_music_runtime;
    scene_services.camera = &scene_camera_runtime;
    scene_services.player_actions = &scene_player_runtime;
    scene_services.player_lock = &scene_player_lock_runtime;
    scene_services.player_position = &scene_player_position_runtime;
    scene_services.player_rotation = &scene_player_rotation_runtime;
    scene_services.arena_ready = &scene_arena_ready_runtime;
    scene_services.system_ui = &scene_system_ui_runtime;
    scene_services.player_action = &scene_player_action_runtime;
    scene_services.player_equipment = &scene_player_equipment_runtime;
    scene_services.camera_sequences = &scene_camera_sequence_runtime;
    scene_services.sound_2d = &scene_sound_runtime;
    scene_services.wait = &scene_wait_runtime;
    scene_services.flash = &scene_flash;
    pal4::NativeGameScriptCallbacks scene_callbacks(scene_services);
    engine.SetCallbacks(&scene_callbacks);
    asIScriptContext* scene_context = nullptr;
    Check(engine.Raw()->CreateContext(&scene_context) == 0 && scene_context &&
              scene_context->Prepare(scene_init) >= 0,
          "prepare original Q01/N01 scene-init bytecode");
    scene_wait_runtime.context = scene_context;
    const auto scene_status = scene_context->Execute();
    Check(scene_status == asEXECUTION_SUSPENDED &&
              scene_music_player.PendingTrack() == "P23" &&
              scene_music_player.State() == 2u &&
              scene_music_player.Mode() == 2u &&
              scene_music_player.FadeBits() == std::bit_cast<std::uint32_t>(2.5f) &&
              scene_music_runtime.path == "gamedata\\Music\\P23.mp3" &&
              scene_music_runtime.stops == 1u &&
              scene_music_runtime.plays == 1u &&
              scene_camera_runtime.writes.size() == 2u &&
              scene_camera_runtime.writes[0].first == 0x41a8u &&
              scene_camera_runtime.writes[1].first == 0x51a8u &&
              scene_camera_runtime.writes[0].second ==
                  scene_camera_runtime.writes[1].second &&
              scene_player_runtime.leader_indices.size() == 1u &&
              scene_player_runtime.leader_indices.front() == 0 &&
              scene_camera_runtime.player_state.Word(0xb0cu / 4u) == 0u &&
              scene_player_lock_runtime.events.size() >= 3u &&
              scene_player_lock_runtime.events[0] == "action:C01" &&
              scene_player_lock_runtime.events[1] == "reset:0" &&
              scene_player_lock_runtime.events[2] == "ui:0" &&
               scene_player_position_runtime.events ==
                   std::vector<std::string>{
                       "position", "property", "property", "select",
                       "active", "floor", "grid", "action", "name",
                       "allocate", "construct", "assign", "add", "cleanup"} &&
               scene_player_rotation_runtime.events ==
                   std::vector<std::string>{
                       "read-x", "read-z", "rotation", "name", "allocate",
                       "construct", "allocate-text", "assign", "close",
                       "add", "cleanup"} &&
               scene_arena_ready_runtime.events ==
                   std::vector<std::string>{
                       "hints", "release-hints", "actor-input", "clear-vector",
                       "input", "pet", "grid"} &&
               scene_arena_ready_runtime.writes[0xc0c4u] == 1u &&
               scene_arena_ready_runtime.writes[0xc1bcu] == 0u &&
               scene_arena_ready_runtime.writes[0x1156cu] == 123u &&
               scene_arena_ready_runtime.writes[0x11570u] == 123u &&
               scene_arena_ready_runtime.writes[0x11574u] == 123u &&
               scene_system_ui_runtime.entries ==
                   std::vector<std::pair<std::uint32_t, std::uint8_t>>{
                       {20u, 0u}, {19u, 1u}, {12u, 0u}} &&
               scene_player_action_runtime.events ==
                   std::vector<std::string>{
                       "player", "action:J08", "flag:1", "clear-weapons",
                       "name", "allocate", "construct", "assign",
                       "effects:3", "flush", "add", "cleanup"} &&
               scene_player_equipment_runtime.requested ==
                   std::vector<std::uint32_t>{3586u, 3501u} &&
               scene_player_equipment_runtime.writes ==
                   std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                       {0x9930u, 3586u}, {0x9934u, 3501u}} &&
               scene_player_equipment_runtime.refreshes == 2u &&
               scene_camera_sequence_runtime.events ==
                   std::vector<std::string>{
                       "clear", "load:MC001", "find:001", "allocate",
                       "construct", "clear", "container", "task", "add-task",
                       "camera-id", "apply", "processed", "flush", "add-wait"} &&
               scene_sound_runtime.played_names ==
                   std::vector<std::string>{"WJ001"} &&
               scene_sound_runtime.current_handle == 0x1234u &&
               scene_flash_runtime.events ==
                   std::vector<std::string>{
                       "fade-in", "allocate", "construct", "flush", "add"} &&
               scene_wait_runtime.duration == 1.5 &&
               scene_wait_runtime.events ==
                   std::vector<std::string>{
                       "allocate", "construct", "flush", "add", "flag"},
           "original Q01 script starts camera, sound, fade-in and timed wait");
    pal4::ScriptWaitHandler timed_wait(scene_wait_runtime);
    timed_wait.Update(scene_wait_runtime.handler, 1.0);
    Check(scene_wait_runtime.handler.remaining == 0.5 &&
              scene_wait_runtime.closes == 0u,
          "Q01 1.5-second wait remains active after one second");
    timed_wait.Update(scene_wait_runtime.handler, 0.5);
    Check(scene_wait_runtime.handler.remaining == 0.0 &&
              scene_wait_runtime.closes == 1u,
          "Q01 timed wait closes at 1.5 seconds");
    (void)scene_context->Release();

    constexpr char kCameraReturnSource[] =
        "int CameraReturns() { if (giCameraPrepare(\"MC001\") != 1) "
        "return -1; return giCameraRunSingle(\"001\", true); }";
    constexpr char kCameraReturnModule[] = "camera_returns";
    Check(engine.Raw()->AddScriptSection(
              kCameraReturnModule, "camera_returns.cs", kCameraReturnSource,
              sizeof(kCameraReturnSource) - 1u) >= 0 &&
              engine.Raw()->Build(kCameraReturnModule) >= 0,
          "compile camera callback return-value probe");
    asIScriptContext* camera_context = nullptr;
    const auto camera_function = engine.FunctionId(
        kCameraReturnModule, "CameraReturns");
    Check(camera_function >= 0 &&
              engine.Raw()->CreateContext(&camera_context) >= 0 &&
              camera_context != nullptr &&
              camera_context->Prepare(camera_function) >= 0 &&
              camera_context->Execute() == asEXECUTION_FINISHED &&
              camera_context->GetReturnDWord() == 1u,
          "camera prepare and single-run return values cross generic ABI");
    (void)camera_context->Release();
    constexpr char kCircleSource[] =
        "int CircleAsync() { return giCameraRunCircle(false); }"
        "int CircleWait() { return giCameraRunCircle(true); }";
    Check(engine.Raw()->AddScriptSection("circle_returns","circle.cs",kCircleSource,sizeof(kCircleSource)-1u)>=0 &&
          engine.Raw()->Build("circle_returns")>=0,"compile circle generic ABI probe");
    scene_camera_sequence_runtime.circle=true;
    for(unsigned probe=0;probe<3;++probe) {
        scene_camera_sequence_runtime.has_cloud=probe!=2;
        scene_camera_sequence_runtime.expected_wait_mode=probe==1 ? 1u : 0u;
        scene_camera_sequence_runtime.events.clear();
        Check(engine.Raw()->CreateContext(&camera_context)>=0 && camera_context,
              "create circle callback context");
        Check(camera_context->Prepare(engine.FunctionId("circle_returns",probe==1?"CircleWait":"CircleAsync"))>=0 &&
              camera_context->Execute()==asEXECUTION_FINISHED &&
              camera_context->GetReturnDWord()==(probe==2?0u:1u),
              "circle wait argument and return value cross generic ABI");
        Check(probe==2 ? scene_camera_sequence_runtime.events.empty() :
              scene_camera_sequence_runtime.events.back()=="add-wait",
              "empty circle does not enqueue; populated circle keeps its wait lifecycle");
        (void)camera_context->Release();
    }
    return EXIT_SUCCESS;
}
