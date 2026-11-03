#include "pal4/comparison_gob_script_trace.h"
#include "pal4/native_angelscript_engine.h"
#include "pal4/native_game_script_callbacks.h"
#include "pal4/native_script_frame_runner.h"
#include "pal4/script_builtin_handlers.h"
#include "pal4/script_flash_effect.h"
#include "pal4/script_global_callbacks.h"
#include "pal4/script_music_transport.h"
#include "pal4/script_bgm_callbacks.h"
#include "pal4/script_object_visibility.h"
#include "pal4/script_object_touch.h"
#include "pal4/script_gob_reset.h"
#include "pal4/script_event_volume_visibility.h"
#include "pal4/script_object_research.h"
#include "pal4/script_effect_callbacks.h"
#include "pal4/script_entity_callbacks.h"
#include "pal4/script_builtin_forwarders.h"
#include "pal4/script_command_queue.h"
#include "pal4/script_arena_skill_state.h"
#include "pal4/script_player_material.h"
#include "pal4/script_player_garb.h"
#include "pal4/script_random_player_position.h"
#include "pal4/script_world_misc.h"
#include "pal4/script_camera_callbacks.h"
#include "pal4/script_inventory_query.h"
#include "pal4/script_forbidden_skill.h"
#include "pal4/script_object_timed_move.h"
#include <bit>
#include <stdexcept>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "%s\n", message);
        std::exit(EXIT_FAILURE);
    }
}

struct ForbiddenSkillRuntime final : pal4::IScriptForbiddenSkillRuntime {
    std::vector<std::pair<std::int32_t, std::uint32_t>> updates;
    std::uint32_t PlayerManager() override { return 0x1000u; }
    std::uint32_t FindPlayer(std::uint32_t manager, std::int32_t index) override {
        Check(manager == PlayerManager(), "forbidden skill manager");
        return index == 0 ? 0x2000u : 0u;
    }
    std::uint8_t SetPresent(std::uint32_t collection, std::int32_t skill,
                            std::uint32_t enabled) override {
        Check(collection == 0x2938u, "forbidden skill component offset");
        updates.emplace_back(skill, enabled);
        return 1u;
    }
};

struct InventoryCountRuntime final : pal4::IScriptInventoryQueryRuntime {
    std::uint32_t queries{},reads{};
    std::uint32_t count=7u;
    std::uint32_t Inventory() override { return 0x1000u; }
    std::uint32_t FindElement(std::uint32_t inventory,std::int32_t id,std::uint32_t category) override {
        Check(inventory==0x1000u && category==5u && (id==3253 || id==3254),
              "compiled item count queries actual property category and signed item id");
        ++queries;return id==3253?0x2000u:0u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address==0x2008u,"compiled item count reads borrowed entry quantity");
        ++reads;return count;
    }
};

struct GarbRuntime final : pal4::IScriptPlayerGarbRuntime {
    std::vector<std::string> models;
    unsigned comparisons{};
    std::uint32_t PlayerManager() override{return 0x1000u;}
    std::uint32_t PlayerByIndex(std::uint32_t manager,std::int32_t index) override{
        Check(manager==0x1000u && index>=0 && index<3,"garb indexes actual three story players");return manager+0xb14u*index;
    }
    void AssignModelText(std::uint32_t buffer,std::string_view model) override{
        Check(buffer==0x1000u+0xb14u*models.size()+0xa18u && models.size()<2u,"garb assigns first two role-owned text fields");models.emplace_back(model);
    }
    bool AllocateModelText(std::uint32_t buffer,std::uint32_t length,std::uint32_t preserve) override{
        Check(buffer==0x1000u+2u*0xb14u+0xa18u && length==3u && preserve==1u,"garb retains original third-role allocation contract");return true;
    }
    void WriteModelText(std::uint32_t,std::string_view model) override{models.emplace_back(model);}
    bool NeedsModelReload(std::uint32_t player) override{Check(player==0x1000u+0xb14u*comparisons++,"garb compares each selected role");return false;}
    std::uint32_t ReadDword(std::uint32_t) override{throw std::logic_error("unchanged garb must not read reload state");}
    std::uint32_t ArenaManager() override{throw std::logic_error("unchanged garb must not access arena");}
    void ScriptOnRemove(std::uint32_t,std::uint32_t) override{throw std::logic_error("unexpected garb removal");}
    void ProcessAndCleanup(std::uint32_t) override{throw std::logic_error("unexpected garb reload");}
    void SetStateFlag(std::uint32_t,std::uint32_t) override{throw std::logic_error("unexpected garb state write");}
    void ScriptOnAdd(std::uint32_t,std::uint32_t) override{throw std::logic_error("unexpected garb addition");}
    std::uint32_t ActorControl() override{throw std::logic_error("unexpected garb controller");}
    void SetActor(std::uint32_t,std::uint32_t) override{throw std::logic_error("unexpected garb rebind");}
};

struct FlashRuntime final : pal4::IScriptFlashRuntime {
    pal4::FlashWaitState wait{};
    bool white=false,fade_in=false;
    std::vector<std::string>& events;

    explicit FlashRuntime(std::vector<std::string>& output) : events(output) {}
    std::uint32_t ScreenEffect() override { return 0x1000u; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x1030u, "flash phase offset");
        return 1u;
    }
    void StartFade(std::uint32_t effect, float duration,
                   std::uint32_t hold, pal4::FlashColor color,
                   float start, float end, float finish) override {
        const auto channel=white?255u:0u;
        Check(effect == 0x1000u && duration == 0.25f && hold == (fade_in?0u:1u) &&
                  color.red == channel && color.green == channel && color.blue == channel &&
                  start == (fade_in?255.0f:0.0f) && end == (fade_in?0.0f:255.0f) && finish == end,
              "script flash arguments reach the existing effect handler");
        events.emplace_back("fade");
    }
    pal4::FlashWaitState* Allocate(std::uint32_t size) override {
        Check(size == sizeof(wait), "flash wait state size");
        return &wait;
    }
    std::uint32_t ScriptExecutor() override { return 0x2000u; }
    void Construct(pal4::FlashWaitState&, std::uint8_t mode,
                   std::uint32_t queue) override {
        Check(mode == 1u && queue == 0x2054u, "flash wait mode and queue");
    }
    void Flush(pal4::FlashWaitState& state) override {
        Check(state.vtable_address == pal4::ScriptFlashEffect::kWaitVtable,
              "flash wait state setup");
    }
    void Close(pal4::FlashWaitState&) override {}
    std::uint32_t AddHandler(std::uint32_t queue,
                             pal4::FlashWaitState* state) override {
        Check(queue == 0x2054u && state == &wait,
              "flash wait handler registration");
        events.emplace_back("flash_wait");
        return 1u;
    }
};

struct MusicRuntime final : pal4::IScriptMusicTransportRuntime {
    std::vector<std::string>& events;

    explicit MusicRuntime(std::vector<std::string>& output) : events(output) {}
    std::uint32_t BackgroundMusicPlayer() override { return 0x3000u; }
    std::uint8_t StopSound(std::uint32_t slot) override {
        Check(slot == 0x3030u, "music pause sound slot");
        events.emplace_back("pause");
        return 1u;
    }
    std::uint8_t LoadAndPlay(std::uint32_t player) override {
        Check(player == 0x3000u, "music resume player");
        events.emplace_back("resume");
        return 1u;
    }
};

struct ArenaSkillRuntime final : pal4::IScriptArenaSkillStateRuntime {
    std::vector<std::string> calls;
    std::uint32_t ArenaSkillManager() override { return 0x7100u; }
    std::uint32_t CleanupPlayerData(std::uint32_t manager) override {
        Check(manager == 0x7100u, "hide uses the bound arena skill manager");
        calls.emplace_back("hide"); return 1u;
    }
    std::uint32_t UpdatePlayerData(std::uint32_t manager) override {
        Check(manager == 0x7100u, "show uses the bound arena skill manager");
        calls.emplace_back("show"); return 1u;
    }
};

struct GlobalRuntime final : pal4::IScriptGlobalCallbackRuntime {
    std::vector<std::string>& events;
    std::uint32_t flags{};

    explicit GlobalRuntime(std::vector<std::string>& output) : events(output) {}
    std::uint32_t GlobalData() override { return 0x4000u; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x4018u, "movie flag address");
        return flags;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        Check(address == 0x4018u, "movie flag write address");
        flags = value;
        events.emplace_back("movie_flag");
    }
    void SaveDialogSettings(std::uint32_t data) override {
        Check(data == 0x4000u, "movie flag persistence");
    }
    std::uint32_t GameInstance() override { return 0u; }
    std::uint32_t GetGameDataEntry(std::uint32_t, std::uint32_t) override {
        return 0u;
    }
    void SetGameDataEntry(std::uint32_t, std::uint32_t,
                          std::uint32_t) override {}
};

struct NewGameRuntime final : pal4::IScriptNewGameRuntime {
    std::vector<std::string>& events;

    explicit NewGameRuntime(std::vector<std::string>& output) : events(output) {}
    std::uint32_t InitializePlayerManager() override { return 0x5000u; }
    void ResetAllPlayerStates(std::uint32_t manager) override {
        Check(manager == 0x5000u, "new game player reset");
        events.emplace_back("reset_players");
    }
    void LoadAndProcessPlayerData(std::uint32_t manager) override {
        Check(manager == 0x5000u, "new game player load");
        events.emplace_back("load_players");
    }
    std::uint32_t ActorControlInstance() override { return 0x6000u; }
    void ShutdownActorControl(std::uint32_t control) override {
        Check(control == 0x6000u, "new game actor shutdown");
        events.emplace_back("actor_shutdown");
    }
    int InitializeActorControl(std::uint32_t control) override {
        Check(control == 0x6000u, "new game actor initialization");
        events.emplace_back("actor_init");
        return 1;
    }
};

struct VisibilityRuntime final : pal4::IScriptObjectVisibilityRuntime {
    std::vector<std::string>& events;
    bool found = true;
    std::uint32_t visible = 1u;

    explicit VisibilityRuntime(std::vector<std::string>& output)
        : events(output) {}
    std::uint32_t ObjectManager() override { return 0x7000u; }
    std::uint32_t FindByName(std::uint32_t manager,
                             std::string_view name) override {
        Check(manager == 0x7000u && name == "entrance1",
              "visible callback forwards the script object name");
        events.emplace_back("find");
        return found ? 0x7100u : 0u;
    }
    std::uint32_t ArenaManager() override { return 0x7200u; }
    void RemoveAnimatedObject(std::uint32_t arena,
                              std::uint32_t object) override {
        Check(arena == 0x7200u && object == 0x7100u,
              "visible callback removes the original object");
        events.emplace_back("remove");
    }
    void SetObjectState(std::uint32_t object,
                        std::uint32_t state) override {
        Check(object == 0x7100u && state == 1u,
              "visible callback enables object state");
        events.emplace_back("state");
    }
    void AddAnimatedObject(std::uint32_t arena,
                           std::uint32_t object) override {
        Check(arena == 0x7200u && object == 0x7100u,
              "visible callback adds the original object");
        events.emplace_back("add");
    }
    std::uint32_t MinimapManager() override { return 0x7300u; }
    void RefreshDialog(std::uint32_t minimap) override {
        Check(minimap == 0x7300u, "visible callback refreshes scene UI");
        events.emplace_back("refresh");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0x7100u + 0x184u) {
            events.emplace_back("query-state");
            return visible;
        }
        Check(address == 0x72e0u || address == 0x7208u,
              "visible callback reads arena log fields");
        return address;
    }
    std::uint32_t LogVisibility(std::uint32_t line,
                                std::string_view name,
                                std::uint32_t word8,
                                std::uint32_t word_e0,
                                std::uint8_t visible) override {
        Check(line == 0x356u && name == "entrance1" &&
                  word8 == 0x7208u && word_e0 == 0x72e0u,
              "visible callback keeps original log operands");
        events.emplace_back("log:" + std::to_string(visible));
        return 1u;
    }
};

struct GobResetRuntime final : pal4::IScriptGobResetRuntime {
    bool found = true;
    unsigned resets{};
    std::uint32_t ObjectManager() override { return 0x7000u; }
    std::uint32_t FindObject(std::uint32_t manager,std::string_view name) override {
        Check(manager==0x7000u && name=="entrance1","GOB reset uses actual script name");
        return found ? 0x7100u : 0u;
    }
    std::uint32_t InvokeReset(std::uint32_t object) override {
        Check(object==0x7100u,"GOB reset forwards the resolved receiver");
        ++resets; return 42u;
    }
};

struct EventVolumeVisibilityRuntime final
    : pal4::IScriptEventVolumeVisibilityRuntime {
    std::vector<std::string> calls;
    bool found = true;
    std::uint32_t ReadWord(std::uint32_t address) override {
        return *reinterpret_cast<const std::uint32_t*>(
            static_cast<std::uintptr_t>(address));
    }
    std::uint32_t EmptyStringAddress() override { return 0u; }
    std::uint32_t EventVolumeManager() override { return 0x8000u; }
    std::uint32_t FindVolume(std::uint32_t manager,
                             std::uint32_t name) override {
        Check(manager == 0x8000u &&
                  std::strcmp(reinterpret_cast<const char*>(
                      static_cast<std::uintptr_t>(name)), "EV001") == 0,
              "event volume callback forwards its script name");
        calls.emplace_back("find");
        return found ? 0x8100u : 0u;
    }
    std::uint32_t ArenaManager() override { return 0x8200u; }
    std::uint32_t Deactivate(std::uint32_t arena,
                             std::uint32_t volume) override {
        Check(arena == 0x8200u && volume == 0x8100u,
              "event volume callback deactivation target");
        calls.emplace_back("deactivate");
        return 1u;
    }
    std::uint32_t Activate(std::uint32_t arena,
                           std::uint32_t volume) override {
        Check(arena == 0x8200u && volume == 0x8100u,
              "event volume callback activation target");
        calls.emplace_back("activate");
        return 1u;
    }
};

struct ObjectResearchRuntime final : pal4::IScriptObjectResearchRuntime {
    std::uint32_t gate = 0u;
    bool found = true;
    std::uint32_t ReadWord(std::uint32_t address) override {
        return *reinterpret_cast<const std::uint32_t*>(
            static_cast<std::uintptr_t>(address));
    }
    std::uint32_t EmptyStringAddress() override { return 0u; }
    std::uint32_t ObjectManager() override { return 0x9000u; }
    std::uint32_t FindByName(std::uint32_t manager,
                             std::uint32_t name) override {
        Check(manager == 0x9000u &&
                  std::strcmp(reinterpret_cast<const char*>(
                      static_cast<std::uintptr_t>(name)), "MO002") == 0,
              "research callback forwards the script object name");
        return found ? 0x9100u : 0u;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        Check(address == 0x928cu, "research callback writes the GOB gate");
        gate = value;
    }
};

struct NpcVisibilityRuntime final : pal4::IScriptNpcVisibilityRuntime {
    std::vector<std::string>& events;
    bool found = true;

    explicit NpcVisibilityRuntime(std::vector<std::string>& output)
        : events(output) {}
    std::uint32_t NpcManager() override { return 0x7400u; }
    std::uint32_t FindNpc(std::uint32_t manager,
                          std::string_view name) override {
        Check(manager == 0x7400u && name == "MN001",
              "NPC callback forwards the script name");
        events.emplace_back("find-npc");
        return found ? 0x7500u : 0u;
    }
    std::uint32_t ArenaManager() override { return 0x7600u; }
    std::uint32_t LeaveNpcScript(std::uint32_t arena,
                                 std::uint32_t npc) override {
        Check(arena == 0x7600u && npc == 0x7500u,
              "NPC callback removes the matching actor");
        events.emplace_back("leave-npc");
        return 1u;
    }
    void SetStateFlag(std::uint32_t npc, std::uint32_t flag) override {
        Check(npc == 0x7500u && flag == 1u,
              "NPC callback enables the actor");
        events.emplace_back("show-npc");
    }
    std::uint32_t EnterNpcScript(std::uint32_t arena,
                                 std::uint32_t npc) override {
        Check(arena == 0x7600u && npc == 0x7500u,
              "NPC callback enters the matching actor");
        events.emplace_back("enter-npc");
        return 1u;
    }
};

struct CameraPitchRuntime final : pal4::IScriptCameraCallbackRuntime {
    std::vector<float> deltas;
    std::vector<std::uint8_t> options;
    void AdjustYpr(float yaw, float pitch_value, float roll, std::uint8_t option) override {
        deltas.insert(deltas.end(), {yaw, pitch_value, roll}); options.push_back(option);
    }
    float pitch = -17.25f;
    std::uint32_t CameraSubsystem() override { return 0x1000u; }
    std::uint32_t ActiveCameraInternalId(std::uint32_t manager) override {
        Check(manager==0x1000u,"pitch script uses current camera manager");return 0x2000u;
    }
    float ReadFloat(std::uint32_t address) override {
        Check(address==0x2040u,"pitch script reads active camera data+64");return pitch;
    }
    std::uint32_t CameraByName(std::uint32_t,std::string_view) override { throw std::runtime_error("unexpected named pitch camera"); }
    std::uint32_t CameraInternalId(std::uint32_t) override { throw std::runtime_error("unexpected pitch object"); }
    void WriteWord(std::uint32_t,std::uint32_t) override { throw std::runtime_error("pitch must not write camera"); }
    std::uint32_t ActiveCameraObject(std::uint32_t) override { throw std::runtime_error("unexpected pitch object query"); }
    void SetLookAt(std::uint32_t,std::uint32_t) override { throw std::runtime_error("pitch must not seek"); }
    std::uint32_t AudioManager() override { throw std::runtime_error("pitch must not query audio"); }
    std::uint32_t ReadWord(std::uint32_t) override { throw std::runtime_error("pitch must read float"); }
    void SetListenerPosition(std::uint32_t,float,float,float) override { throw std::runtime_error("pitch must not set audio"); }
    std::uint32_t SetListenerOrientation(std::uint32_t,float,float,float,float,float,float) override { throw std::runtime_error("pitch must not orient audio"); }
    pal4::PlayerManagerObjectState& PlayerManagerState() override { throw std::runtime_error("pitch must not query players"); }
};

struct CameraDistanceRuntime final : pal4::IScriptCameraDistanceRuntime {
    std::vector<std::uint32_t> values;
    std::uint32_t SetDistanceOptimization(
        const std::uint32_t enabled) override {
        values.push_back(enabled);
        return enabled;
    }
};

struct TraceCallbacks final : pal4::IGameScriptCallbackRuntime {
    std::vector<int> calls;
    void Invoke(int system_id, asIScriptGeneric&) override {
        calls.push_back(system_id);
    }
};

struct GateWait final : pal4::INativeScriptFrameWaitRuntime {
    bool blocked = true;
    bool release = false;
    bool BlocksScript() const override { return blocked; }
    bool Tick(float) override {
        if (release) {
            blocked = false;
            return true;
        }
        return false;
    }
};

struct EffectRuntime final : pal4::IScriptEffectRuntime {
    struct Spawn {
        std::string name;
        std::uint32_t repeats;
        std::uint32_t position;
        std::array<float, 3> coordinates;
    };
    std::vector<Spawn> spawns;
    unsigned resets{};
    std::vector<std::int32_t> cg_keys;
    unsigned cg_stops{};
    bool missing_actor{};
    std::uint32_t EffectManager() override { return 0x1000u; }
    std::uint32_t HandleScriptCallback(std::uint32_t manager, std::int32_t key) override {
        Check(manager==0x1000u,"CG callback manager");cg_keys.push_back(key);return 0u;
    }
    std::uint32_t CleanupScriptCallback(std::uint32_t manager) override {Check(manager==0x1000u,"CG stop manager");++cg_stops;return 0u;}
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0x2000u + 0x2c50u) return 2u;
        return *reinterpret_cast<const std::uint32_t*>(address);
    }
    std::uint32_t EmptyStringAddress() override { std::abort(); }
    std::uint32_t PlayerManager() override { return 0x2000u; }
    std::uint32_t PlayerByIndex(std::uint32_t manager, std::int32_t index) override {
        Check(manager == 0x2000u && index == -1, "signed script player index");
        return missing_actor ? 0u : 0x3000u;
    }
    std::uint32_t NpcManager() override { return 0x4000u; }
    std::uint32_t FindNpc(std::uint32_t manager, std::uint32_t name) override {
        Check(manager == 0x4000u &&
                  std::strcmp(reinterpret_cast<const char*>(name), "MN001") == 0,
              "script NPC name payload");
        return missing_actor ? 0u : 0x5000u;
    }
    pal4::ScriptEffectHandle CreateAtCoordinates(
        std::uint32_t manager, std::uint32_t name,
        const std::array<float, 3>& position, std::uint32_t repeats,
        std::uint32_t flag) override {
        Check(manager == 0x1000u && flag == 0u, "script effect group and flag");
        spawns.push_back({reinterpret_cast<const char*>(name), repeats, 0u, position});
        return {17u, 0x1000u};
    }
    pal4::ScriptEffectHandle CreateAtAddress(
        std::uint32_t manager, std::uint32_t name, std::uint32_t position,
        std::uint32_t repeats, std::uint32_t flag) override {
        auto handle = CreateAtCoordinates(manager, name, {}, repeats, flag);
        spawns.back().position = position;
        return handle;
    }
    std::uint32_t ResetHandle(pal4::ScriptEffectHandle& handle) override {
        Check(handle.first == 17u && handle.second == 0x1000u,
              "script invalidates temporary effect handle");
        ++resets;
        handle = {};
        return 0u;
    }
};

int Run(pal4::NativeAngelScriptEngine& engine, asIScriptContext& context,
        const char* name) {
    const int function = engine.FunctionId("dispatch", name);
    Check(function >= 0 && context.Prepare(function) >= 0,
          "prepare script callback test");
    return context.Execute();
}

void VerifyRandomPlayerPosition() {
    struct Runtime final : pal4::IScriptRandomPlayerPositionRuntime {
        unsigned components{}, radii{}, checks{}, writes{}, grids{};
        std::uint32_t PlayerManager() override {return 0x1000u;}
        std::uint32_t PlayerByIndex(std::uint32_t manager,std::int32_t index) override {
            Check(manager==0x1000u && (index==0 || index==-1),"signed random-position player index");
            return index==0?manager:0u;
        }
        std::uint32_t ReadWord(std::uint32_t a) override {
            Check(a==0x3c50u,"random current uses saved leader index");return 0u;
        }
        float ReadFloat(std::uint32_t a) override {
            Check(a>=0x10acu && a<=0x10b4u && (a-0x10acu)%4u==0u,"random source XYZ");
            return std::array<float,3>{100.f,50.f,200.f}[(a-0x10acu)/4u];
        }
        double RandomComponent(float seed) override {
            Check(seed==0.f,"random component preserves zero-seed argument");
            return components++%3u==0u?1.0:0.0;
        }
        double RandomRadius(float a,float b,float seed) override {
            Check(a==200.f && b==500.f && seed==0.f,"SDK forwards random radius float bounds");
            ++radii;return 10.;
        }
        std::uint32_t CollisionWorld() override {return 7u;}
        void ForEachCollisionObject(std::uint32_t w,pal4::ScriptCollisionProbe& p) override {
            Check(w==7u && p.segment==std::array<float,6>{110.f,150.f,200.f,110.f,-50.f,200.f},"SDK candidate performs vertical event collision query");
        }
        std::uint32_t ProcessClumpCollision(const std::array<float,6>&,std::uint32_t,std::array<std::uint8_t,24>&) override {throw std::logic_error("empty event fixture");}
        double ObjectProperty(std::uint32_t p) override {Check(p==0x1000u,"random player scale");return 10.;}
        std::uint32_t MovementAdapter() override {return 8u;}
        bool CheckMovement(std::uint32_t m,std::array<float,3>& p,float a,float b) override {
            Check(m==8u && a==3.5f && b>25.29f && b<25.31f,"random ground dimensions");
            if(++checks==1u)return false;
            p[1]=49.f;return true;
        }
        void SetTransform(std::uint32_t actor,const std::array<float,3>& p,std::uint32_t mode) override {
            Check(actor==0x1000u && mode==0u && p==std::array<float,3>{110.f,49.f,200.f},"random setter uses floor-adjusted candidate");++writes;
        }
        std::uint32_t MinimapManager() override {return 9u;}
        std::uint32_t UpdateGrid(std::uint32_t m,std::uint32_t p,std::uint32_t r) override {
            Check(m==9u && p==0x10acu && r==0x10a0u,"random setter updates player grid");++grids;return 0u;
        }
    } runtime;
    pal4::NativeAngelScriptEngine engine;
    constexpr char source[]="void Current(){giPlayerCurrentRandomPosition(200.0f,500.0f);}"
        "void RandomNumber(){global_mvar=giGetRandnum(1,360);}"
        "void Indexed(){giPlayerRandomPosition(0,200.0f,500.0f);}"
        "void Missing(){giPlayerRandomPosition(-1,200.0f,500.0f);}";
    Check(engine.Raw()->AddScriptSection("dispatch","random.cs",source,sizeof(source)-1)>=0 &&
          engine.Raw()->Build("dispatch")>=0,"compile actual SDK random-position callbacks");
    struct IntegerRandom final : pal4::IScriptRandomIntegerRuntime {
        unsigned draws{};
        std::int32_t Rand() override { ++draws; return 718; }
    } integer;
    pal4::NativeGameScriptServices services;services.random_player_position=&runtime;
    services.random_integer=&integer;
    pal4::NativeGameScriptCallbacks callbacks(services);engine.SetCallbacks(&callbacks);
    asIScriptContext* context{};
    Check(engine.Raw()->CreateContext(&context)>=0 && context,"random SDK context");
    Check(Run(engine,*context,"Current")==asEXECUTION_FINISHED && runtime.writes==1u &&
        runtime.grids==1u && runtime.components==6u && runtime.radii==2u,"current random retries rejected ground and commits once");
    Check(Run(engine,*context,"Indexed")==asEXECUTION_FINISHED && runtime.writes==2u &&
        runtime.grids==2u && runtime.components==9u && runtime.radii==3u,"indexed random shares candidate and transform policy");
    Check(Run(engine,*context,"Missing")==asEXECUTION_FINISHED && runtime.components==9u &&
        runtime.writes==2u,"missing indexed player neither draws RNG nor writes transform");
    Check(Run(engine,*context,"RandomNumber")==asEXECUTION_FINISHED &&
        engine.GlobalStoryVariable()==1 && integer.draws==1u,
        "compiled integer RNG draws once and keeps exclusive upper bound modulo359");
    context->Release();
}

void VerifyGobDirectPosition() {
    struct PositionRuntime final : pal4::IScriptObjectTimedMoveRuntime {
        std::vector<std::string> events;
        pal4::TimedMovePosition written{};
        unsigned writes{};
        [[noreturn]] static void Forbidden(){throw std::runtime_error("direct GOB position touched activity/timed movement");}
        std::uint32_t ObjectManager() override {events.emplace_back("manager");return 0x1000u;}
        std::uint32_t FindByName(std::uint32_t manager,std::string_view name) override {
            Check(manager==0x1000u,"direct GOB position uses existing manager");
            events.emplace_back("find:"+std::string(name));return name=="JA03"?0x2000u:0u;
        }
        bool HasAnimatedObject(std::uint32_t) override {Forbidden();}
        pal4::TimedMovePosition ReadPosition(std::uint32_t) override {Forbidden();}
        pal4::TimedObjectMoveWaitState* AllocateWait(std::uint32_t) override {Forbidden();}
        std::uint32_t ScriptExecutor() override {Forbidden();}
        void ConstructWait(pal4::TimedObjectMoveWaitState&,std::uint8_t,std::uint32_t) override {Forbidden();}
        void AssignName(pal4::TimedObjectMoveWaitState&,std::string_view) override {Forbidden();}
        void FlushWait(pal4::TimedObjectMoveWaitState&) override {Forbidden();}
        void AddHandler(std::uint32_t,pal4::TimedObjectMoveWaitState*) override {Forbidden();}
        std::uint32_t CleanupName() override {Forbidden();}
        float Interpolate(float,float,float) override {Forbidden();}
        void CloseWait(pal4::TimedObjectMoveWaitState&) override {Forbidden();}
        void SetPosition(std::uint32_t object,const pal4::TimedMovePosition& position,std::uint32_t flags) override {
            Check(object==0x2000u && flags==0u,"direct GOB position preserves absolute flags0");
            written=position;++writes;events.emplace_back("set");
        }
    } positions;
    pal4::NativeAngelScriptEngine engine;
    constexpr char source[]=
        "void Existing(){giGOBSetPosition(\"JA03\",130.0f,2000.0f,-3600.0f);global_mvar=7;}"
        "void Missing(){giGOBSetPosition(\"missing\",1.0f,2.0f,3.0f);global_mvar=8;}"
        "void Empty(){giGOBSetPosition(\"\",4.0f,5.0f,6.0f);global_mvar=9;}";
    Check(engine.Raw()->AddScriptSection("dispatch","gob_position.cs",source,sizeof(source)-1)>=0 &&
        engine.Raw()->Build("dispatch")>=0,"compile actual SDK direct GOB position callbacks");
    pal4::NativeGameScriptServices services;services.object_move=&positions;
    pal4::NativeGameScriptCallbacks callbacks(services);engine.SetCallbacks(&callbacks);
    asIScriptContext* context{};
    Check(engine.Raw()->CreateContext(&context)>=0 && context,"create GOB position SDK context");
    auto& gob_trace=pal4::GetComparisonGobScriptTrace();
    const auto trace_begin=gob_trace.Count();
    Check(Run(engine,*context,"Existing")==asEXECUTION_FINISHED && engine.GlobalStoryVariable()==7 &&
        positions.writes==1u && positions.written==pal4::TimedMovePosition{130.f,2000.f,-3600.f} &&
        positions.events==std::vector<std::string>({"manager","find:JA03","set"}),
        "direct XYZ callback performs one setter without activity gate, wait or timed-move cleanup");
    if(gob_trace.Enabled()) {
        Check(gob_trace.Count()==trace_begin+2u,"actual SDK observer brackets one successful callback");
        for(unsigned offset=0;offset<2u;++offset){
            const auto& event=gob_trace.Entries()[trace_begin+offset];
            Check(event.id==-176 && event.after==(offset!=0u) &&
                  std::string_view(event.name.data(),event.name_length)=="JA03" &&
                  !event.name_truncated && event.argc==3u &&
                  event.args[0]==0x43020000u && event.args[1]==0x44fa0000u && event.args[2]==0xc5610000u,
                  "actual SDK raw one-word access preserves float argument bits in observer");
        }
    }
    positions.events.clear();
    Check(Run(engine,*context,"Missing")==asEXECUTION_FINISHED && engine.GlobalStoryVariable()==8 &&
        positions.writes==1u && positions.events==std::vector<std::string>({"manager","find:missing"}),
        "missing object returns through shared lookup without setter or wait");
    positions.events.clear();
    Check(Run(engine,*context,"Empty")==asEXECUTION_FINISHED && engine.GlobalStoryVariable()==9 &&
        positions.writes==1u && positions.events==std::vector<std::string>({"manager","find:"}),
        "empty name reaches existing lookup as empty text without fabricated success");
    callbacks.SetServices({});
    Check(Run(engine,*context,"Existing")==asEXECUTION_EXCEPTION &&
        std::strcmp(context->GetExceptionString(),"PAL4 game script service -176 is unavailable")==0,
        "direct GOB position requires the actual bound transform service");
    engine.SetCallbacks(nullptr);(void)context->Release();
}

}  // namespace

struct CombatCommands final : pal4::IScriptCommandRuntime {
    pal4::ScriptCommandQueue queue;
    pal4::ScriptCommandQueue& ScriptExecutorQueue() override { return queue; }
};

namespace {
struct ObjectTouchRuntime final : pal4::IScriptObjectTouchRuntime {
    bool found = true;
    unsigned touches = 0;
    unsigned player_reads = 0;
    std::uint32_t PlayerManager() override { ++player_reads; return 0x1000u; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x3c50u, "touch reads original current slot offset"); return 2u;
    }
    std::uint32_t ObjectManager() override { return 0x8000u; }
    std::uint32_t FindByName(std::uint32_t manager, std::string_view name) override {
        Check(manager == 0x8000u && name == "Jeffect001", "compiled touch preserves object name");
        return found ? 0x9000u : 0u;
    }
    std::uint32_t Touch(std::uint32_t object, std::uint32_t direction) override {
        Check(object == 0x9000u && direction == 0x1000u + 2u * 0xb14u + 0xacu,
              "compiled touch dispatches virtual with selected player position");
        ++touches; return 0u;
    }
};
}
int main() {
    VerifyRandomPlayerPosition();
    VerifyGobDirectPosition();
    pal4::NativeAngelScriptEngine engine;
    constexpr char source[] =
        "void Garb2() { giAllPlayerGarb2(); } "
        "void Garb1() { giPlayerGarb1(0); giPlayerGarb1(1); giPlayerGarb1(2); } "
        "void PropertyCount() { global_mvar=giGetPropertyNumb(3253); } "
        "void MissingPropertyCount() { global_mvar=giGetPropertyNumb(3254); } "
        "float CameraPitch() { return giCameraGetPitch(); } "
        "void CameraAdjust() { giCameraCtrlYPR(-1.25, 2.5, -3.75, true); giCameraCtrlYPR(0, -0.0, 4, false); } "
        "void BgmAreaCheck() { if (giBGMConfigIsInArea(\"Ab9\")) global_mvar=11; else global_mvar=12; } "
        "void BgmEmptyArea() { if (giBGMConfigIsInArea(\"\")) global_mvar=13; else global_mvar=14; } "
        "void Bound() { giFlashOutBlack(0.25f, true, true); "
        "giScriptMusicPause(); giScriptMusicResume(); "
        "giOpenMovieFlag(0); giNewGame(); } "
        "void WhiteOut() { giFlashOutWhite(0.25f, true, true); } "
        "void WhiteIn() { giFlashInWhite(0.25f, true); } "
        "void StopPursuit() { giMonsterStopPursuit(); } "
        "void CG() { giCGEffPlay(11); giCGEffStop(); giCGEffPlay(99); giCGEffStop(); } "
        "void EffectXYZ() { giEffectPlay(\"foot_2\", -1, 1.25f, -2.0f, 3.5f); } "
        "void EffectPlayer() { giEffectPlayWithPlayer(\"foot_2\", 2, -1); } "
        "void EffectCurrent() { giEffectPlayWithCurrentPlayer(\"foot_2\", 3); } "
        "void EffectNPC() { giEffectPlayWithNPC(\"foot_2\", 4, \"MN001\"); } "
        "void ImmediateBegin() { giIMMBegin(); } "
        "void ImmediateEnd() { giIMMEnd(); } "
        "void ResetObject() { giGOBReset(\"entrance1\"); } "
        "void TouchObject() { giGOMTouch(\"Jeffect001\"); } "
        "void VisibleQuery() { if (giGetVisibleObject(\"entrance1\")) global_mvar=1; else global_mvar=0; } "
        "void VisibleOff() { giSetObjectVisible(\"entrance1\", false); } "
        "void VisibleOn() { giSetObjectVisible(\"entrance1\", true); } "
        "void EventHidden() { giEventVolumeVisible(\"EV001\", false); } "
        "void EventShown() { giEventVolumeVisible(\"EV001\", true); } "
        "void ResearchOn() { giGameObjectSetResearch(\"MO002\", true); } "
        "void ResearchOff() { giGameObjectSetResearch(\"MO002\", false); } "
        "void NpcVisible() { giSetNpcVisible(\"MN001\", true); } "
        "void NpcHidden() { giSetNpcVisible(\"MN001\", false); } "
        "void CombatAdds() { giAddCombatMonster(201, 3); giAddCombatMonster(-1, 0); } "
        "void DistOff() { giCameraSetDistOptEnable(false); } "
        "void DistOn() { giCameraSetDistOptEnable(true); } "
        "void CameraMode() { giCameraSetMode(0, false); } "
        "void CameraSeek() { giCameraSeekToPlayer(); } "
        "void Tutorial() { giShowTutorial(1001); } "
        "void HideSkills() { giHideGASkillObject(); } "
        "void ShowSignpost() { giShowSignpost(); } "
        "void WalkTo() { giPlayerWalkTo(0, 1.0f, 2.0f, 3.0f, true); } "
        "void WalkCurrent() { giPlayerCurrentWalkTo(1.0f, 2.0f, 3.0f, true); } "
        "void FaceCurrentObject() { giPlayerCurrentFaceToGOB(\"light01_f\", true); } "
        "void ObjectOut() { giOBJBlendOut(\"fog\",1.0f,false); } "
        "void ObjectIn() { giOBJBlendIn(\"fog\",0.5f,true); } "
        "void ObjectMove() { giGOBMovment(\"fog\",2.0f,1.0f,2.0f,3.0f,false); } "
        "void CurrentAction() { giPlayerCurrentDoAction(\"z03\", 1, false); } "
        "void EndCurrentAction() { giPlayerCurrentEndAction(); } "
        "void RunTo() { giPlayerRunTo(0, 1.0f, 2.0f, 3.0f, true); } "
        "void BackTo() { giPlayerBackTo(0, 1.0f, 2.0f, 3.0f, true); } "
        "void EndMove() { giPlayerEndMove(0); } "
        "void EndCurrentMove() { giCurrentPlayerEndMove(); } "
        "void NpcAction() { giNpcDoAction(\"MN001\", \"J01\", 1, true); } "
        "void NpcEndAction() { giNpcEndAction(\"MN001\", true); } "
        "void SteppedLoad() { giArenaLoad(\"Q01\", \"Q01\", \"\", false); "
        "giIMMBegin(); giSetObjectVisible(\"entrance1\", false); } "
        "void MonsterHide() { giMonsterSetHide(\"m01-01-01\", false); } "
        "void MonsterVisible() { giMonsterSetVisible(\"m01-01-01\", true); } "
        "void IndexedAngle() { giPlayerSetAng(1, 222.0f); } "
        "void NpcAngle() { giNpcSetAng(\"4012\", 31.0f); } "
        "void IndexedVisible() { giPlayerSetVisible(1, true); } "
        "void CurrentVisible() { giPlayerCurrentSetVisible(true); } "
        "void IndexedPosition() { giPlayerSetPos(1, -1622.0f, 46.0f, 1411.0f); } "
        "void NpcPosition() { giNpcSetPos(\"MN002\", -141.0f, -79.0f, 1256.0f); } "
        "void NpcTurn() { giNpcSetDir(\"MN002\",288.0f,false); } "
        "void NpcFaceNpc() { giNpcFaceToNpc(\"4010\",\"MN001\",true); } "
        "void NpcFacePlayer() { giNpcFaceToPlayer(\"MN001\",0,false); } "
        "void PlayerFaceNpc() { giPlayerFaceToNpc(0,\"MN002\",true); } "
        "void ShadowOff() { giEnableShadow(false); } "
        "void NpcEmotion() { giNpcSetEmotion(\"MN001\",\"101_by\"); } "
        "void NpcEmotionReset() { giNpcResetEmotion(\"MN001\"); } "
        "void SoundStop() { gi2DSoundStop(); } "
        "void SoundStopId() { gi2DSoundStopID(7); } "
        "void LearnStunt() { giPlayerAddSkill(0,5502,true); } "
        "void ForbidStunt() { giPlayerForbidenSkill(0,5501,true); } "
        "void AllowStunt() { giPlayerForbidenSkill(0,5501,false); } "
        "void MissingForbidPlayer() { giPlayerForbidenSkill(4,5501,true); } "
        "void GainMoney() { giAddMoney(1000,true); } "
        "void SpendMoney() { giPayMoney(-7,false); } "
        "void PlayerFacePlayer() { giPlayerFaceToPlayer(0,1,true); } "
        "void PlayerEmotion() { giPlayerSetEmotion(0,\"101_by\"); } "
        "void PlayerCollision() { giPlayerAttachCollision(1); } "
        "void PlayerDetach() { giPlayerDetachCollision(1); } "
        "void LegsInjured() { giLINGSHALegsInjured(\"103_sst\"); } "
        "void LegsHealing() { giLINGSHALegsHealing(); } "
        "void ResetEmotion() { giPlayerResetEmotion(0); giPlayerResetEmotion(-1); giPlayerResetEmotion(4); } "
        "void NpcFadeIn() { giNpcBlendIn(\"MN001\",2.0f,true); } "
        "void ArenaSkillOff() { giArenaSkillEnable(false); } "
        "void NpcUnhold() { giNpcUnHoldAct(\"MN003\"); } "
        "void MissingMovie() { giPlayMovie(\"1A.bik\"); } "
        "void MissingArena() { giArenaLoad(\"Q01\", \"N01\", \"\", true); }";
    Check(engine.Raw()->AddScriptSection("dispatch", "dispatch.cs", source,
                                         sizeof(source) - 1) >= 0 &&
              engine.Raw()->Build("dispatch") >= 0,
          "compile game callback script");
    asIScriptContext* context = nullptr;
    Check(engine.Raw()->CreateContext(&context) >= 0 && context != nullptr,
          "create game callback context");

    std::vector<std::string> events;
    FlashRuntime flash_runtime(events);
    pal4::ScriptFlashEffect flash(flash_runtime);
    MusicRuntime music(events);
    GlobalRuntime globals(events);
    NewGameRuntime new_game(events);
    pal4::NativeGameScriptCallbacks callbacks({
        &flash, &music, nullptr, &globals, &new_game, nullptr});
    engine.SetCallbacks(&callbacks);
    Check(Run(engine,*context,"PropertyCount")==asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),
                          "PAL4 game script service -190 is unavailable")==0,
          "item count requires the live inventory query service");
    Check(Run(engine,*context,"Garb2")==asEXECUTION_EXCEPTION &&
          std::strcmp(context->GetExceptionString(),"PAL4 game script service -150 is unavailable")==0,
          "compiled garb callback requires real lifecycle services");
    GarbRuntime garb;pal4::NativeGameScriptServices garb_services{};garb_services.player_garb=&garb;
    callbacks.SetServices(garb_services);
    Check(Run(engine,*context,"Garb2")==asEXECUTION_FINISHED && garb.comparisons==3u &&
          garb.models==std::vector<std::string>{"102","104","107"},
          "compiled giAllPlayerGarb2 executes shared three-player model selection and unchanged-model early returns");
    GarbRuntime first_garb;
    garb_services.player_garb=&first_garb;
    callbacks.SetServices(garb_services);
    Check(Run(engine,*context,"Garb1")==asEXECUTION_FINISHED && first_garb.comparisons==3u &&
          first_garb.models==std::vector<std::string>{"101","103","106"},
          "compiled indexed giPlayerGarb1 selects the original three role models");
    InventoryCountRuntime item_counts;
    pal4::NativeGameScriptServices count_services{};count_services.inventory_query=&item_counts;
    callbacks.SetServices(count_services);
    Check(Run(engine,*context,"PropertyCount")==asEXECUTION_FINISHED &&
              engine.GlobalStoryVariable()==7 && item_counts.queries==1u && item_counts.reads==1u,
          "actual SDK giGetPropertyNumb returns the stored quantity");
    Check(Run(engine,*context,"MissingPropertyCount")==asEXECUTION_FINISHED &&
              engine.GlobalStoryVariable()==0 && item_counts.queries==2u && item_counts.reads==1u,
          "actual SDK missing item returns zero without reading a nonexistent entry");
    item_counts.count=3u;
    Check(Run(engine,*context,"PropertyCount")==asEXECUTION_FINISHED &&
              engine.GlobalStoryVariable()==3 && item_counts.queries==3u && item_counts.reads==2u,
          "item count rereads the borrowed inventory quantity");
    callbacks.SetServices({&flash,&music,nullptr,&globals,&new_game,nullptr});
    Check(Run(engine,*context,"BgmAreaCheck")==asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),
                          "PAL4 game script service -125 is unavailable")==0,
          "area query requires its current scene binding");
    struct AreaName final : pal4::IScriptBgmAreaName {
        std::string_view value="ab9";
        std::string_view SceneName() const override { return value; }
    } music_area;
    pal4::NativeGameScriptServices bgm_services{};
    bgm_services.background_music_area=&music_area;
    callbacks.SetServices(bgm_services);
    Check(Run(engine,*context,"BgmAreaCheck")==asEXECUTION_FINISHED &&
              engine.GlobalStoryVariable()==11,
          "SDK bool area query preserves CRT case-insensitive matching");
    music_area.value="1";
    Check(Run(engine,*context,"BgmAreaCheck")==asEXECUTION_FINISHED &&
              engine.GlobalStoryVariable()==12,
          "SDK area query rereads the borrowed scene and returns false");
    music_area.value="";
    Check(Run(engine,*context,"BgmEmptyArea")==asEXECUTION_FINISHED &&
              engine.GlobalStoryVariable()==13,
          "SDK area query supports the original empty-string comparison");
    callbacks.SetServices({});
    Check(Run(engine, *context, "EffectXYZ") == asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),
                          "PAL4 game script service -138 is unavailable") == 0,
          "effect creation requires its runtime");
    EffectRuntime effects;
    pal4::NativeGameScriptServices effect_services{};
    effect_services.effects = &effects;
    callbacks.SetServices(effect_services);
    Check(Run(engine,*context,"CG")==asEXECUTION_FINISHED && effects.cg_keys==std::vector<std::int32_t>{12,15} && effects.cg_stops==2u,
          "compiled CG callbacks preserve adjusted OldTV key, upper clamp and stop without waits");
    for (const auto* function : {"EffectXYZ", "EffectPlayer", "EffectCurrent", "EffectNPC"})
        Check(Run(engine, *context, function) == asEXECUTION_FINISHED,
              "effect script executes without creating a wait");
    Check(effects.spawns.size() == 4u && effects.resets == 4u &&
              effects.spawns[0].name == "foot_2" &&
              effects.spawns[0].repeats == 0xffffffffu &&
              effects.spawns[0].coordinates == std::array<float, 3>{1.25f, -2.0f, 3.5f} &&
              effects.spawns[1].repeats == 2u && effects.spawns[1].position == 0x30acu &&
              effects.spawns[2].repeats == 3u &&
              effects.spawns[2].position == 0x2000u + 2u * 0xb14u + 0xacu &&
              effects.spawns[3].repeats == 4u && effects.spawns[3].position == 0x50acu,
          "actual AngelScript ABI reaches shared coordinate/player/current/NPC handlers");
    effects.missing_actor = true;
    Check(Run(engine, *context, "EffectPlayer") == asEXECUTION_FINISHED &&
              Run(engine, *context, "EffectNPC") == asEXECUTION_FINISHED &&
              effects.spawns.size() == 4u && effects.resets == 4u,
          "missing actors create no effect or temporary handle");
    callbacks.SetServices({&flash, &music, nullptr, &globals, &new_game, nullptr});
    Check(Run(engine, *context, "Bound") == asEXECUTION_FINISHED &&
              events == std::vector<std::string>{
                  "fade", "flash_wait", "pause", "resume", "movie_flag",
                  "reset_players", "load_players", "actor_shutdown",
                  "actor_init"} && globals.flags == 1u,
          "game callbacks run the existing handlers in script order");
    flash_runtime.white=true;
    const auto before_white=events.size();
    Check(Run(engine,*context,"WhiteOut")==asEXECUTION_FINISHED,
          "white fade out preserves color, hold and wait handler through SDK callback");
    flash_runtime.fade_in=true;
    Check(Run(engine,*context,"WhiteIn")==asEXECUTION_FINISHED && events.size()==before_white+4u,
          "white fade in reverses alpha, clears hold and retains the shared wait handler");
    flash_runtime.white=flash_runtime.fade_in=false;
    Check(Run(engine, *context, "ImmediateBegin") ==
                  asEXECUTION_FINISHED && callbacks.ImmediateMode() == 1u &&
              Run(engine, *context, "ImmediateEnd") ==
                  asEXECUTION_FINISHED && callbacks.ImmediateMode() == 0u,
          "script immediate mode saves and restores the active record");
    callbacks.ResetImmediateMode();
    Check(callbacks.ImmediateMode() == 0u,
          "new script module starts outside immediate mode");
    Check(Run(engine, *context, "MissingMovie") == asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),
                          "PAL4 game script service -297 is unavailable") == 0,
          "missing movie runtime is reported at its system ID");
    Check(Run(engine, *context, "CameraMode") == asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),
                          "PAL4 game script service -81 is unavailable") == 0,
          "camera mode requires a bound scene service");
    Check(Run(engine, *context, "CameraSeek") == asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),
                          "PAL4 game script service -104 is unavailable") == 0,
          "camera seek requires a bound scene service");
    Check(Run(engine, *context, "Tutorial") == asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),
                          "PAL4 game script service -309 is unavailable") == 0,
          "tutorial requires a bound scene service");
    Check(Run(engine, *context, "MissingArena") == asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),
                          "PAL4 game script service -65 is unavailable") == 0,
          "missing arena runtime cannot be silently skipped");
    Check(Run(engine, *context, "VisibleOff") == asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),
                          "PAL4 game script service -97 is unavailable") == 0,
          "missing scene object manager cannot be silently skipped");
    Check(Run(engine, *context, "EventHidden") == asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),
                          "PAL4 game script service -149 is unavailable") == 0,
          "event volume visibility requires a bound EVF service");
    EventVolumeVisibilityRuntime event_visibility;
    pal4::NativeGameScriptServices event_visibility_services{};
    event_visibility_services.event_volume_visibility = &event_visibility;
    callbacks.SetServices(event_visibility_services);
    Check(Run(engine, *context, "EventHidden") == asEXECUTION_FINISHED &&
              event_visibility.calls ==
                  std::vector<std::string>{"find", "deactivate"},
          "script hides the named event volume");
    event_visibility.calls.clear();
    Check(Run(engine, *context, "EventShown") == asEXECUTION_FINISHED &&
              event_visibility.calls == std::vector<std::string>{
                  "find", "deactivate", "activate"},
          "script reactivates the named event volume");
    event_visibility.found = false;
    event_visibility.calls.clear();
    Check(Run(engine, *context, "EventShown") == asEXECUTION_FINISHED &&
              event_visibility.calls == std::vector<std::string>{"find"},
          "missing event volume has no scene side effects");
    Check(Run(engine, *context, "ResearchOn") == asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),
                          "PAL4 game script service -122 is unavailable") == 0,
          "object research requires a bound GOB service");
    ObjectResearchRuntime research;
    pal4::NativeGameScriptServices research_services{};
    research_services.object_research = &research;
    callbacks.SetServices(research_services);
    Check(Run(engine, *context, "ResearchOn") == asEXECUTION_FINISHED &&
              research.gate == 1u &&
              Run(engine, *context, "ResearchOff") == asEXECUTION_FINISHED &&
              research.gate == 0u,
          "script updates the named object's research gate");
    research.found = false;
    Check(Run(engine, *context, "ResearchOn") == asEXECUTION_FINISHED &&
              research.gate == 0u,
          "unknown object does not change a research gate");
    callbacks.SetServices({});
    Check(Run(engine,*context,"ResetObject")==asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),"PAL4 game script service -210 is unavailable")==0,
          "GOB reset requires its actual scene service");
    GobResetRuntime reset;
    pal4::NativeGameScriptServices reset_services{};
    reset_services.gob_reset=&reset;
    callbacks.SetServices(reset_services);
    Check(Run(engine,*context,"ResetObject")==asEXECUTION_FINISHED && reset.resets==1,
          "compiled giGOBReset uses the shared named virtual dispatch");
    reset.found=false;
    Check(Run(engine,*context,"ResetObject")==asEXECUTION_FINISHED && reset.resets==1,
          "missing GOB is ignored without dispatching reset");
    callbacks.SetServices({});
    Check(Run(engine,*context,"TouchObject")==asEXECUTION_EXCEPTION &&
          std::strcmp(context->GetExceptionString(),"PAL4 game script service -102 is unavailable")==0,
          "object touch requires the actual scene binding");
    ObjectTouchRuntime touch;
    pal4::NativeGameScriptServices touch_services{};
    touch_services.object_touch=&touch;
    callbacks.SetServices(touch_services);
    Check(Run(engine,*context,"TouchObject")==asEXECUTION_FINISHED &&
          touch.touches==1 && touch.player_reads==2,
          "SDK callback -102 reuses shared named touch and re-reads current player");
    touch.found=false;
    Check(Run(engine,*context,"TouchObject")==asEXECUTION_FINISHED && touch.touches==1,
          "missing script touch target does not dispatch a virtual call");
    VisibilityRuntime visibility(events);
    pal4::NativeGameScriptServices visible_services{};
    visible_services.object_visibility = &visibility;
    callbacks.SetServices(visible_services);
    for (const auto state : {0u,1u,0x80000000u}) {
        visibility.visible = state;
        events.clear();
        Check(Run(engine, *context, "VisibleQuery") == asEXECUTION_FINISHED &&
                  engine.GlobalStoryVariable() == (state != 0u) &&
                  events == std::vector<std::string>{"find","query-state"},
              "compiled giGetVisibleObject reads the actual nonzero state without side effects");
    }
    visibility.found = false;
    events.clear();
    Check(Run(engine, *context, "VisibleQuery") == asEXECUTION_FINISHED &&
              engine.GlobalStoryVariable() == 0 && events == std::vector<std::string>{"find"},
          "compiled visibility query returns false for a missing object");
    callbacks.SetServices({});
    Check(Run(engine, *context, "VisibleQuery") == asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(), "PAL4 game script service -154 is unavailable") == 0,
          "visibility query requires the real object service");
    callbacks.SetServices(visible_services);
    visibility.found = true;
    events.clear();
    Check(Run(engine, *context, "VisibleOff") == asEXECUTION_FINISHED &&
              events == std::vector<std::string>{
                  "find", "remove", "refresh", "log:0"},
          "script hides a loaded scene object through the existing handler");
    events.clear();
    Check(Run(engine, *context, "VisibleOn") == asEXECUTION_FINISHED &&
              events == std::vector<std::string>{
                  "find", "remove", "state", "add", "refresh", "log:1"},
          "script shows a loaded scene object through the existing handler");
    visibility.found = false;
    events.clear();
    Check(Run(engine, *context, "VisibleOff") == asEXECUTION_FINISHED &&
              events == std::vector<std::string>{"find"},
          "unknown scene object has no visibility side effects");
    Check(Run(engine, *context, "NpcVisible") == asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),
                          "PAL4 game script service -76 is unavailable") == 0,
          "missing NPC service is reported at its system ID");
    NpcVisibilityRuntime npcs(events);
    pal4::NativeGameScriptServices npc_services{};
    npc_services.npc_visibility = &npcs;
    callbacks.SetServices(npc_services);
    events.clear();
    Check(Run(engine, *context, "NpcVisible") == asEXECUTION_FINISHED &&
              events == std::vector<std::string>{
                  "find-npc", "leave-npc", "show-npc", "enter-npc"},
          "script NPC show removes before re-entering the actor");
    events.clear();
    Check(Run(engine, *context, "NpcHidden") == asEXECUTION_FINISHED &&
              events == std::vector<std::string>{"find-npc", "leave-npc"},
          "script NPC hide only removes the actor");
    npcs.found = false;
    events.clear();
    Check(Run(engine, *context, "NpcVisible") == asEXECUTION_FINISHED &&
              events == std::vector<std::string>{"find-npc"},
          "unknown NPC has no scene side effects");
    Check(Run(engine, *context, "DistOff") == asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),
                          "PAL4 game script service -120 is unavailable") == 0,
          "missing camera distance plugin is reported at its system ID");
    CameraDistanceRuntime camera_distance;
    Check(Run(engine,*context,"CameraPitch")==asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),"PAL4 game script service -63 is unavailable")==0,
          "pitch query reports the missing camera service at its actual SDK ID");
    Check(Run(engine,*context,"CameraAdjust")==asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),"PAL4 game script service -58 is unavailable")==0,
          "incremental YPR rejects an unbound camera service");
    CameraPitchRuntime camera_pitch;
    pal4::NativeGameScriptServices pitch_services{};pitch_services.camera=&camera_pitch;
    callbacks.SetServices(pitch_services);
    Check(Run(engine,*context,"CameraAdjust")==asEXECUTION_FINISHED &&
              camera_pitch.deltas==std::vector<float>{-1.25f,2.5f,-3.75f,0.0f,-0.0f,4.0f} &&
              camera_pitch.options==std::vector<std::uint8_t>{1u,0u},
          "compiled giCameraCtrlYPR passes all delta floats and boolean through SDK ABI");
    for(const auto pitch:{-17.25f,-0.0f,359.5f}){
        camera_pitch.pitch=pitch;
        Check(Run(engine,*context,"CameraPitch")==asEXECUTION_FINISHED &&
                  std::bit_cast<std::uint32_t>(context->GetReturnFloat())==std::bit_cast<std::uint32_t>(pitch),
              "compiled giCameraGetPitch returns real float bits through SDK generic ABI");
    }
    pal4::NativeGameScriptServices distance_services{};
    distance_services.camera_distance = &camera_distance;
    callbacks.SetServices(distance_services);
    Check(Run(engine, *context, "DistOff") == asEXECUTION_FINISHED &&
              Run(engine, *context, "DistOn") == asEXECUTION_FINISHED &&
              camera_distance.values == std::vector<std::uint32_t>{0u, 1u},
          "camera distance script forwards both plugin flag values");
    Check(Run(engine, *context, "RunTo") == asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),
                          "PAL4 game script service -258 is unavailable") == 0,
          "player run-to requires a bound movement service");
    Check(Run(engine, *context, "BackTo") == asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),
                          "PAL4 game script service -260 is unavailable") == 0,
          "player back-to requires the shared movement service");
    Check(Run(engine, *context, "WalkTo") == asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),
                          "PAL4 game script service -257 is unavailable") == 0,
          "player walk-to requires a bound movement service");
    Check(Run(engine, *context, "WalkCurrent") == asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),
                          "PAL4 game script service -259 is unavailable") == 0,
          "current walk dispatches to the existing walk service");
    Check(Run(engine, *context, "FaceCurrentObject") == asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),
                          "PAL4 game script service -312 is unavailable") == 0,
          "current object facing dispatches to the existing turn service");
    for (const auto& entry : std::vector<std::pair<const char*,const char*>>{
        {"ObjectOut","PAL4 game script service -302 is unavailable"},
        {"ObjectIn","PAL4 game script service -303 is unavailable"},
        {"ObjectMove","PAL4 game script service -308 is unavailable"}})
        Check(Run(engine,*context,entry.first)==asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),entry.second)==0,
              "object fade/movement dispatch requires bound owner service");
    Check(Run(engine, *context, "CurrentAction") == asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),
                          "PAL4 game script service -247 is unavailable") == 0 &&
              Run(engine, *context, "EndCurrentAction") == asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),
                          "PAL4 game script service -248 is unavailable") == 0,
          "current action start/end dispatch through the existing action service");
    Check(Run(engine, *context, "HideSkills") == asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),
                          "PAL4 game script service -180 is unavailable") == 0 &&
              Run(engine, *context, "ShowSignpost") == asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),
                          "PAL4 game script service -181 is unavailable") == 0,
          "skill state callbacks require the current scene skill service");
    Check(Run(engine, *context, "IndexedPosition") == asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),
                          "PAL4 game script service -249 is unavailable") == 0,
          "indexed player position requires a bound position service");
    Check(Run(engine,*context,"NpcPosition")==asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),"PAL4 game script service -274 is unavailable")==0,
          "NPC position requires a bound position service");
    Check(Run(engine,*context,"NpcTurn")==asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),"PAL4 game script service -276 is unavailable")==0,
          "NPC turn requires a bound turn service");
    Check(Run(engine,*context,"NpcFaceNpc")==asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),"PAL4 game script service -277 is unavailable")==0,
          "NPC facing another NPC dispatches through the existing turn service");
    Check(Run(engine,*context,"PlayerFacePlayer")==asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),"PAL4 game script service -254 is unavailable")==0,
          "player facing another player dispatches through the existing turn service");
    Check(Run(engine,*context,"NpcFacePlayer")==asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),"PAL4 game script service -278 is unavailable")==0,
          "NPC facing an explicit player dispatches through the existing turn service");
    for (const auto& entry : std::vector<std::pair<const char*, const char*>>{
        {"PlayerFaceNpc", "PAL4 game script service -256 is unavailable"},
        {"ShadowOff", "PAL4 game script service -214 is unavailable"},
        {"NpcEmotion", "PAL4 game script service -188 is unavailable"},
        {"NpcEmotionReset", "PAL4 game script service -189 is unavailable"},
        {"GainMoney", "PAL4 game script service -178 is unavailable"},
        {"SpendMoney", "PAL4 game script service -179 is unavailable"},
        {"SoundStop", "PAL4 game script service -134 is unavailable"},
        {"SoundStopId", "PAL4 game script service -135 is unavailable"},
        {"LearnStunt", "PAL4 game script service -145 is unavailable"},
        {"ForbidStunt", "PAL4 game script service -204 is unavailable"}})
        Check(Run(engine,*context,entry.first)==asEXECUTION_EXCEPTION &&
                  std::strcmp(context->GetExceptionString(),entry.second)==0,
              "hall callback requires the actual bound service");
    ForbiddenSkillRuntime forbidden;
    pal4::NativeGameScriptServices forbidden_services{};
    forbidden_services.forbidden_skill = &forbidden;
    callbacks.SetServices(forbidden_services);
    Check(Run(engine,*context,"ForbidStunt") == asEXECUTION_FINISHED &&
              Run(engine,*context,"AllowStunt") == asEXECUTION_FINISHED &&
              Run(engine,*context,"MissingForbidPlayer") == asEXECUTION_FINISHED &&
              forbidden.updates ==
                  std::vector<std::pair<std::int32_t,std::uint32_t>>{{5501,1u},{5501,0u}},
          "compiled forbidden skill callback reaches shared enable/remove and missing-player gates");
    Check(Run(engine,*context,"PlayerEmotion")==asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),"PAL4 game script service -182 is unavailable")==0,
          "player emotion requires the material service");
    Check(Run(engine,*context,"PlayerCollision")==asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),"PAL4 game script service -72 is unavailable")==0,
          "player collision requires the action service");
    for (const auto& entry : std::vector<std::pair<const char*, const char*>>{
        {"PlayerDetach", "PAL4 game script service -73 is unavailable"},
        {"LegsInjured", "PAL4 game script service -186 is unavailable"},
        {"LegsHealing", "PAL4 game script service -187 is unavailable"},
        {"ResetEmotion", "PAL4 game script service -183 is unavailable"},
        {"NpcFadeIn", "PAL4 game script service -280 is unavailable"}})
        Check(Run(engine,*context,entry.first)==asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),entry.second)==0,
              "night hunt callback requires its actual runtime service");
    struct TextureRuntime final : pal4::IScriptPlayerTextureRuntime {
        unsigned injured{}, healed{};
        std::vector<std::int32_t> reset_players;
        std::uint32_t SetFace(std::int32_t,std::string_view) override {return 0;}
        std::uint32_t ApplyFace(std::int32_t player) override {reset_players.push_back(player);return 0;}
        std::uint32_t SetLingshaLegs(std::string_view name) override {
            Check(name=="103_sst","legs callback preserves script texture name");++injured;return 1;
        }
        std::uint32_t ApplyLingshaLegs() override {++healed;return 1;}
    } textures;
    pal4::NativeGameScriptServices texture_services{};texture_services.player_material=&textures;
    callbacks.SetServices(texture_services);
    Check(Run(engine,*context,"LegsInjured")==asEXECUTION_FINISHED &&
          Run(engine,*context,"LegsHealing")==asEXECUTION_FINISHED &&
          textures.injured==1 && textures.healed==1,"compiled legs callbacks dispatch to the texture owner");
    Check(Run(engine,*context,"ResetEmotion")==asEXECUTION_FINISHED &&
          textures.reset_players==std::vector<std::int32_t>{0,-1,4},
          "compiled emotion reset preserves signed indices and delegates to the material owner");
    callbacks.SetServices({});
    Check(Run(engine,*context,"ArenaSkillOff")==asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),"PAL4 game script service -196 is unavailable")==0,
          "arena skill gate requires the UI service");
    Check(Run(engine,*context,"NpcUnhold")==asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),"PAL4 game script service -119 is unavailable")==0,
          "NPC unhold requires a bound action service");
    Check(Run(engine, *context, "CurrentVisible") == asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),
                          "PAL4 game script service -113 is unavailable") == 0,
          "current player visibility requires its own bound runtime");
    Check(Run(engine, *context, "IndexedVisible") == asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),
                          "PAL4 game script service -71 is unavailable") == 0,
          "indexed player visibility requires a bound player service");
    Check(Run(engine, *context, "StopPursuit") == asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(), "PAL4 game script service -80 is unavailable") == 0,
          "stop pursuit requires the real monster manager service");
    Check(Run(engine, *context, "MonsterHide") == asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(), "PAL4 game script service -121 is unavailable") == 0 &&
              Run(engine, *context, "MonsterVisible") == asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(), "PAL4 game script service -146 is unavailable") == 0,
          "monster visibility requires the current scene service");
    Check(Run(engine, *context, "IndexedAngle") == asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(), "PAL4 game script service -252 is unavailable") == 0,
          "indexed player angle requires a bound rotation service");
    Check(Run(engine, *context, "NpcAngle") == asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),
                          "PAL4 game script service -286 is unavailable") == 0,
          "NPC numeric angle dispatch requires the existing rotation service");
    ArenaSkillRuntime arena_skills;
    auto skill_services = distance_services;
    skill_services.arena_skill_state = &arena_skills;
    callbacks.SetServices(skill_services);
    Check(Run(engine, *context, "HideSkills") == asEXECUTION_FINISHED &&
              Run(engine, *context, "ShowSignpost") == asEXECUTION_FINISHED &&
              arena_skills.calls == std::vector<std::string>{"hide", "show"},
          "SDK skill callbacks preserve hide/show order through shared adapters");
    callbacks.SetServices(distance_services);
    Check(Run(engine, *context, "EndMove") == asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),
                          "PAL4 game script service -267 is unavailable") == 0 &&
              Run(engine, *context, "EndCurrentMove") ==
                  asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),
                          "PAL4 game script service -268 is unavailable") == 0,
          "player movement-end callbacks require a bound walk service");
    Check(Run(engine, *context, "NpcAction") == asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),
                          "PAL4 game script service -272 is unavailable") == 0 &&
              Run(engine, *context, "NpcEndAction") ==
                  asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),
                          "PAL4 game script service -273 is unavailable") == 0,
          "NPC action callbacks require a bound scene action service");

    Check(Run(engine, *context, "CombatAdds") == asEXECUTION_EXCEPTION &&
              std::strcmp(context->GetExceptionString(),
                          "PAL4 game script service -92 is unavailable") == 0,
          "combat additions require a real executor queue");
    CombatCommands combat_commands;
    pal4::NativeGameScriptServices command_services{};
    command_services.commands = &combat_commands;
    callbacks.SetServices(command_services);
    Check(Run(engine, *context, "CombatAdds") == asEXECUTION_FINISHED &&
              combat_commands.queue.Commands().size() == 2u &&
              combat_commands.queue.Commands()[0].command == 201u &&
              combat_commands.queue.Commands()[0].argument == 3u &&
              combat_commands.queue.Commands()[0].reserved == 0u &&
              combat_commands.queue.Commands()[1].command == 0xffffffffu &&
              combat_commands.queue.Commands()[1].argument == 0u &&
              combat_commands.queue.Commands()[1].reserved == 0u,
          "actual SDK callback appends exact pending combat records in order");

    TraceCallbacks trace;
    engine.SetCallbacks(&trace);
    pal4::NativeScriptFrameRunner stepped(engine);
    stepped.Queue("dispatch", "SteppedLoad");
    int steps = 0;
    while (trace.calls.empty() && steps < 32) {
        Check(stepped.TickOriginal(0.016f) ==
                  pal4::NativeScriptFrameStatus::suspended,
              "arena request is reached through original source steps");
        ++steps;
    }
    Check(trace.calls == std::vector<int>{-65} && steps > 0,
          "arena transition yields before immediate object changes");
    GateWait gate;
    Check(stepped.TickOriginal(0.016f, &gate) ==
                  pal4::NativeScriptFrameStatus::suspended &&
              trace.calls == std::vector<int>{-65},
          "blocking scene load holds the next script source step");
    gate.release = true;
    (void)stepped.TickOriginal(0.016f, &gate);
    while (stepped.Status() == pal4::NativeScriptFrameStatus::suspended &&
           steps < 64) {
        (void)stepped.TickOriginal(0.016f);
        ++steps;
    }
    Check(stepped.Status() == pal4::NativeScriptFrameStatus::finished &&
              trace.calls == std::vector<int>{-65, -55, -97},
          "subsequent source steps resume the object callbacks in order");
    engine.SetCallbacks(nullptr);
    (void)context->Release();
    return EXIT_SUCCESS;
}
