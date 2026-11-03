#include "pal4/native_game_database.h"
#include "pal4/native_angelscript_engine.h"
#include "pal4/native_script_frame_runner.h"
#include "pal4/native_game_script_callbacks.h"
#include "pal4/game_arena_load_handler.h"
#include "pal4/game_data_object_lifecycle.h"
#include "pal4/native_package_archive.h"
#include "pal4/native_pk_file_manager_runtime.h"
#include "pal4/native_player_manager_session.h"
#include "pal4/native_player_initial_reset_activity.h"
#include "pal4/native_player_manager_reset_runtime.h"
#include "pal4/native_player_loaded_data_runtime.h"
#include "pal4/native_player_slot_process_runtime.h"
#include "pal4/native_role_record_image.h"
#include "pal4/native_game_object_animation_lifecycle.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_animation_event_queue_runtime.h"
#include "pal4/native_script_new_game_runtime.h"
#include "pal4/native_actor_control_session.h"
#include "pal4/actor_control_input_component.h"
#include "pal4/pal_actor_control_loop.h"
#include "pal4/player_manager_slot.h"
#include "pal4/script_flash_effect.h"
#include "pal4/script_global_callbacks.h"
#include "pal4/script_movie_player.h"
#include "pal4/script_music_transport.h"
#include "support/rw_dict_schema_fake.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::uint32_t Address(void* value) {
    return static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(value));
}

struct LoadHost final : pal4::PlayerManagerLoadRuntimeStub {
    std::vector<std::string>& events;
    std::uint32_t manager;
    std::uint32_t actors{};

    LoadHost(std::vector<std::string>& events, std::uint32_t manager)
        : events(events), manager(manager) {}

    std::uint32_t GameDB() override { return 0x9000u; }
    std::uint32_t ReadDword(std::uint32_t address) override {
        return *reinterpret_cast<std::uint32_t*>(address);
    }
    void ProcessScripts() override {
        throw std::logic_error("PK manager owns the player script scope");
    }
    void CallLoadActor(std::uint32_t slot, std::uint32_t virtual_slot,
                       std::uint32_t name, std::uint32_t model,
                       std::uint32_t enabled) override {
        constexpr std::string_view models[]{"101", "103", "106", "105"};
        const auto index = actors++;
        Check(index < 4u &&
                  slot == manager + index * pal4::PlayerManagerLoader::kSlotBytes &&
                  virtual_slot == 0x30u && enabled == 1u &&
                  name == pal4::PlayerManagerLoader::kNames[index] &&
                  std::string_view(reinterpret_cast<const char*>(model)) ==
                      models[index],
              "original Role model reaches actor-load boundary");
    }
    void ProcessLoadedData(std::uint32_t address) override {
        Check(address == manager, "player postprocess manager");
        throw std::logic_error("player postprocess needs native operations");
    }
    void CleanupScripts() override {
        throw std::logic_error("PK manager owns the player script cleanup");
    }
};

struct ActorAssets final : pal4::IPlayerActorAssetLoader {
    std::vector<std::string> models;
    std::vector<std::string> names;
    std::vector<std::uint32_t> loaded;
    std::vector<std::uint32_t> unloaded;
    std::string reject_model;
    bool Load(std::uint32_t slot, std::string_view name,
              std::string_view model) override {
        Check(slot != 0u, "actor asset callback receives a player slot");
        names.emplace_back(name);
        models.emplace_back(model);
        if (model == reject_model) return false;
        loaded.push_back(slot);
        return true;
    }
    bool UnloadSlot(std::uint32_t slot) noexcept override {
        unloaded.push_back(slot);
        const auto found = std::find(loaded.begin(), loaded.end(), slot);
        if (found == loaded.end()) return false;
        loaded.erase(found);
        return true;
    }
};

struct SlotRuntime final : pal4::PlayerManagerSlotRuntimeStub {
    std::uint32_t ReadDword(std::uint32_t address) override {
        return *reinterpret_cast<const std::uint32_t*>(address);
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        *reinterpret_cast<std::uint32_t*>(address) = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        *reinterpret_cast<std::uint8_t*>(address) = value;
    }
};

struct LoadedInventory final : pal4::PlayerManagerSlotInventoryRuntimeStub {
    std::uint32_t ReadDword(std::uint32_t address) override {
        return *reinterpret_cast<const std::uint32_t*>(address);
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        *reinterpret_cast<std::uint32_t*>(address) = value;
    }
    std::uint32_t InventoryManager() override {
        throw std::logic_error("NewGame fixture has no equipped items");
    }
};

struct EmptySkillCleanup final : pal4::INativePlayerSkillCleanupCallbacks {
    std::uint32_t ResolveSkillRecord(std::uint32_t) override {
        throw std::logic_error("NewGame fixture has no active skills");
    }
    std::uint32_t InvokeVirtual(
        std::uint32_t, std::uint32_t,
        std::span<const std::uint32_t>) override {
        throw std::logic_error("NewGame fixture has no skill cleanup calls");
    }
};

struct LoadedProcessor final : pal4::PlayerManagerSlotProcessRuntimeStub {
    std::vector<std::string>& events;
    pal4::NativePlayerSlotCopyRuntime& copy;
    bool fail_after_load{};
    std::uint32_t process_count{};

    LoadedProcessor(std::vector<std::string>& calls,
                    pal4::NativePlayerSlotCopyRuntime& copier)
        : events(calls), copy(copier) {}
    std::uint32_t ReadDword(std::uint32_t address) override {
        return *reinterpret_cast<const std::uint32_t*>(address);
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        *reinterpret_cast<std::uint32_t*>(address) = value;
    }
    std::uint32_t GameDatabase() override {
        if (fail_after_load)
            throw std::runtime_error("test player postprocess failure");
        events.emplace_back("process" +
                            std::to_string(process_count++ % 4u));
        return 0x9000u;
    }
    std::uint32_t FindRecord(std::uint32_t table,
                             std::uint32_t key) override {
        (void)table;
        (void)key;
        throw std::logic_error("postprocess must reuse loaded Role records");
    }
    pal4::IPlayerManagerSlotCopyRuntime& CopyRuntime() override {
        return copy;
    }
};

struct BaseRuntime final : pal4::IBaseObjectActivityRuntime {
    std::vector<std::string>& events;
    std::uint32_t manager;

    BaseRuntime(std::vector<std::string>& events, std::uint32_t manager)
        : events(events), manager(manager) {}
    std::uint32_t Index(pal4::BaseObjectState& state) const {
        const auto slot = Address(state.Data());
        Check(slot >= manager &&
                  (slot - manager) % pal4::PlayerManagerSlot::kObjectBytes == 0u,
              "base object address follows manager layout");
        const auto index = (slot - manager) /
            pal4::PlayerManagerSlot::kObjectBytes;
        Check(index < pal4::PlayerManagerObject::kElementCount,
              "base object index is in range");
        return index;
    }
    void ApplyMaterialTexture(pal4::BaseObjectState& state) override {
        events.emplace_back("reset" + std::to_string(Index(state)));
    }
};

struct AnimationRuntime final : pal4::NativeGameObjectAnimationLifecycle {
    std::uint32_t initialized{};
    std::uint32_t cleaned{};

    explicit AnimationRuntime(pal4::NativeBaseObjectMemory& memory)
        : NativeGameObjectAnimationLifecycle(memory) {}

    void InitializeAnimationData(
        pal4::GameObjectEmbeddedAnimationState& state) override {
        NativeGameObjectAnimationLifecycle::InitializeAnimationData(state);
        ++initialized;
    }
    void CleanupAnimationData(
        pal4::GameObjectEmbeddedAnimationState& state) override {
        NativeGameObjectAnimationLifecycle::CleanupAnimationData(state);
        ++cleaned;
    }
    std::uint32_t FindHierarchy(std::uint32_t) override {
        std::abort();
    }
    void ProcessAnimationData(pal4::GameObjectEmbeddedAnimationState&,
                              std::uint32_t, std::int32_t) override {
        std::abort();
    }
};

struct QueueActivity final : pal4::IAnimationEventQueueActivityRuntime {
    void DeleteAnimation(std::uint32_t) override { std::abort(); }
    void AdvanceAnimation(std::uint32_t, float) override { std::abort(); }
    void CallAnimationEvent(std::uint32_t, std::uint32_t) override {
        std::abort();
    }
    std::uint32_t AnimationFinished(std::uint32_t) override {
        std::abort();
    }
};

struct ActorLoop final : pal4::IPalActorControlLoopRuntime {
    std::vector<std::string>& events;
    explicit ActorLoop(std::vector<std::string>& calls) : events(calls) {}
    void ProcessInput(pal4::PalActorControlState& state) override {
        events.emplace_back("clear_actor");
        IPalActorControlLoopRuntime::ProcessInput(state);
    }
};

struct ActorRuntime final : pal4::PalActorControlRuntimeStub {
    std::vector<std::string>& events;
    std::uint32_t event_context;

    ActorRuntime(std::vector<std::string>& events,
                 std::uint32_t event_context)
        : events(events), event_context(event_context) {}
    void ShutdownComponent(std::uint32_t*) override {
        events.emplace_back("actor_shutdown");
    }
    void ShutdownDatabase(std::uint32_t*, std::uint32_t context) override {
        Check(context == event_context, "actor shutdown event context");
    }
    void ShutdownMotion(std::uint32_t*) override {}
    std::uint8_t ResetController(pal4::PalActorControlState&) override {
        return 0u;
    }
    void InitializeComponent(std::uint32_t*) override {
        events.emplace_back("actor_initialize");
    }
    void InitializeDatabase(std::uint32_t*) override {}
    void InitializeMotion(std::uint32_t*) override {}
    std::uint32_t Log(std::uint32_t, std::string_view,
                      std::string_view, std::uint32_t line) override {
        return line == 108u ? 0x77u : 0u;
    }
};

struct FlashRuntime final : pal4::IScriptFlashRuntime {
    std::uint32_t ScreenEffect() override { return 0x1000u; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x1030u, "script fade phase address");
        return 0u;
    }
    void StartFade(std::uint32_t, float, std::uint32_t,
                   pal4::FlashColor, float, float, float) override {
        Check(false, "test fixture expects an inactive fade");
    }
    pal4::FlashWaitState* Allocate(std::uint32_t) override { return nullptr; }
    std::uint32_t ScriptExecutor() override { return 0u; }
    void Construct(pal4::FlashWaitState&, std::uint8_t,
                   std::uint32_t) override {}
    void Flush(pal4::FlashWaitState&) override {}
    void Close(pal4::FlashWaitState&) override {}
    std::uint32_t AddHandler(std::uint32_t,
                             pal4::FlashWaitState*) override { return 0u; }
};

struct MusicRuntime final : pal4::IScriptMusicTransportRuntime {
    std::uint32_t pauses{};
    std::uint32_t resumes{};
    std::uint32_t BackgroundMusicPlayer() override { return 0x1000u; }
    std::uint8_t StopSound(std::uint32_t slot) override {
        Check(slot == 0x1030u, "script music pause slot");
        ++pauses;
        return 1u;
    }
    std::uint8_t LoadAndPlay(std::uint32_t player) override {
        Check(player == 0x1000u, "script music resume player");
        ++resumes;
        return 1u;
    }
};

struct MovieRuntime final : pal4::IScriptMoviePlayer {
    std::vector<std::string> names;
    std::uint32_t Play(std::string_view name) override {
        names.emplace_back(name);
        return 1u;
    }
    std::uint32_t PlayFinal(std::string_view) override { return 0u; }
};

struct MovieFlagRuntime final : pal4::IScriptMovieFlagRuntime {
    std::uint32_t flags{};
    std::uint32_t saves{};
    std::uint32_t GlobalData() override { return 0x4000u; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x4018u, "movie flag address");
        return flags;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        Check(address == 0x4018u, "movie flag write address");
        flags = value;
    }
    void SaveDialogSettings(std::uint32_t address) override {
        Check(address == 0x4000u, "movie flag save address");
        ++saves;
    }
};

struct ArenaRuntime final : pal4::IGameArenaLoadRuntime {
    static constexpr std::array<std::string_view, 4> keys{
        "PALGAMEIV_LOADING_TYPE", "PALGAMEIV_LOADING_ARENA",
        "PALGAMEIV_LOADING_AREA", "PALGAMEIV_LOADING_AREA_RES"};
    std::array<std::string, 4> values{};
    std::array<bool, 4> added{};
    std::uint32_t arena_state{1u};
    std::uint32_t requested_state{};
    std::uint32_t time_56c{};
    std::uint32_t time_570{};
    std::uint32_t time_574{};
    std::uint32_t actor;

    explicit ArenaRuntime(std::uint32_t actor) : actor(actor) {}
    std::uint32_t ArenaManager() override { return 0x6000u; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0x60c4u) return arena_state;
        Check(address == 0x7574u, "arena timestamp read address");
        return time_574;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        switch (address) {
        case 0x60c4u: arena_state = value; break;
        case 0x756cu: time_56c = value; break;
        case 0x7570u: time_570 = value; break;
        case 0x7574u: time_574 = value; break;
        default: Check(false, "arena write address");
        }
    }
    std::uint32_t InitializeXmlDocument() override { return 0x2000u; }
    std::optional<std::uint32_t> FindAttribute(
        std::uint32_t document, std::string_view key) override {
        Check(document == 0x2000u, "arena document lookup");
        for (std::uint32_t i = 0u; i < keys.size(); ++i)
            if (key == keys[i]) return std::nullopt;
        Check(false, "arena attribute key");
        return std::nullopt;
    }
    void SetInt(std::uint32_t, std::int32_t) override {
        Check(false, "arena fixture has no existing attributes");
    }
    void SetString(std::uint32_t, std::string_view) override {
        Check(false, "arena fixture has no existing attributes");
    }
    std::uint32_t InitializeIntAttribute(
        std::string_view key, std::int32_t value) override {
        Check(key == keys[0] && value == 0, "arena loading type");
        values[0] = "0";
        return 1u;
    }
    std::uint32_t InitializeStringAttribute(
        std::string_view key, std::string_view value) override {
        for (std::uint32_t i = 1u; i < keys.size(); ++i) {
            if (key == keys[i]) {
                values[i] = value;
                return i + 1u;
            }
        }
        Check(false, "arena string attribute key");
        return 0u;
    }
    void AddAttribute(std::uint32_t document,
                      std::uint32_t attribute) override {
        Check(document == 0x2000u && attribute >= 1u &&
                  attribute <= keys.size(), "arena add attribute");
        added[attribute - 1u] = true;
    }
    void DestroyAttribute(std::uint32_t attribute) override {
        Check(attribute >= 1u && attribute <= keys.size(),
              "arena temporary attribute cleanup");
    }
    std::uint32_t GameInstance() override { return 0x5000u; }
    void InitializeStateAndUi(std::uint32_t game,
                              std::uint32_t state,
                              std::uint32_t document) override {
        Check(game == 0x5000u && state == 6u && document == 0x2000u &&
                  added == std::array<bool, 4>{true, true, true, true},
              "arena state transition receives loading document");
        requested_state = state;
    }
    std::uint32_t ActorControl() override { return actor; }
    void ProcessActorInput(std::uint32_t address) override {
        Check(address == actor, "arena actor input address");
    }
    void ProcessActorInputAndClearVector(std::uint32_t address) override {
        Check(address == actor, "arena actor vector address");
    }
    std::uint32_t InputManager() override { return 0x6800u; }
    void InitializeInputState(std::uint32_t input) override {
        Check(input == 0x6800u, "arena input address");
    }
    std::uint32_t Milliseconds() override { return 0x123456u; }
    std::uint32_t GameState() override { return 0x7000u; }
    std::int32_t DestroyXmlDocument(std::uint32_t document) override {
        Check(document == 0x2000u, "arena document cleanup");
        return 0;
    }
};

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) return EXIT_FAILURE;
    {
        struct InputRuntime final : pal4::IActorControlInputComponentRuntime {
            std::uint32_t animation = 0u;
            std::uint32_t value = 0u;
            std::int32_t GameConfigPrimary() override { return 60; }
            void SetAnimationFlag(std::uint32_t member,
                                  std::uint32_t flag) override {
                animation = member;
                value = flag;
            }
            void HandleInputEvents(
                pal4::ActorControlInputComponentState&,
                std::uint32_t, float) override {
                throw std::logic_error("unexpected ActorControl input event");
            }
            void UpdateInputObject(
                pal4::ActorControlInputComponentState&,
                std::uint32_t, float) override {
                throw std::logic_error("unexpected ActorControl input update");
            }
        } input_runtime;
        pal4::NativeBaseObjectMemory actor_memory;
        pal4::test::RwDictSchemaFake actor_schema;
        pal4::NativeActorControlSession actor_session(
            actor_memory, actor_schema, {60, 60});
        const auto actor_address = actor_session.Construct();
        const auto& words = actor_session.State().words;
        Check(actor_session.Constructed() && actor_address != 0u &&
                  words[23] == pal4::ActorControlInputComponent::kVtable &&
                  words[35] == pal4::ActorControlDatabase::kVtable &&
                  words[186] == 0xffffffffu &&
                  words[203] == 0xffffffffu &&
                  words[279] == 0xffffffffu &&
                  words[209] == pal4::ActorControlNamedNode::kDerivedVtable &&
                  words[216] == pal4::ActorControlNamedNode::kDerivedVtable &&
                  std::strcmp(reinterpret_cast<const char*>(words[211]),
                              "FL") == 0 &&
                  std::strcmp(reinterpret_cast<const char*>(words[218]),
                              "FR") == 0 &&
                  actor_memory.OutstandingAllocations() == 2u,
              "native ActorControl constructs owned members and names");
        auto& bound_words = actor_session.State().words;
        bound_words[27u] = 4u;
        bound_words[32u] = 9u;
        bound_words[166u] = 1u;
        for (const auto [index, slot] :
             {std::pair{23u, 16u}, std::pair{35u, 16u},
              std::pair{165u, 16u}, std::pair{209u, 12u},
              std::pair{216u, 12u}})
            actor_session.BindActorSubobject(bound_words.data() + index,
                                              0x7000u, slot, input_runtime);
        Check(input_runtime.animation == 0x7170u &&
                  input_runtime.value == 0x3f800000u &&
                  bound_words[27u] == 0u && bound_words[32u] == 0u &&
                  bound_words[35u + 127u] == 0x7000u &&
                  bound_words[166u] == 0u &&
                  bound_words[209u + 5u] == 0x7000u &&
                  bound_words[216u + 5u] == 0x7000u,
              "native ActorControl binds five original subobjects");
        bool wrong_slot_rejected = false;
        try {
            actor_session.BindActorSubobject(bound_words.data() + 23u,
                                              0x7000u, 12u, input_runtime);
        } catch (const std::invalid_argument&) {
            wrong_slot_rejected = true;
        }
        Check(wrong_slot_rejected,
              "native ActorControl rejects an unrelated virtual slot");
        bool script_scope_missing = false;
        try {
            (void)pal4::PalActorControl::Initialize(
                actor_session.State(), actor_address, actor_session.Runtime());
        } catch (const std::logic_error&) {
            script_scope_missing = true;
        }
        Check(script_scope_missing,
              "native ActorControl rejects unbound script scope");
        actor_session.Destroy();
        actor_session.Destroy();
        Check(!actor_session.Constructed() &&
                  actor_memory.OutstandingAllocations() == 0u &&
                  words[211] == 0u && words[218] == 0u,
              "native ActorControl releases name buffers");
    }
    pal4::NativeGameDatabase database;
    Check(database.Load(argv[1]), "load original game database");
    pal4::NativeRoleRecordCatalog roles(database);

    pal4::PalActorControlState actor;
    actor.words[0] = 1u;
    std::vector<std::string> events;
    SlotRuntime slots;
    BaseRuntime base(events, 0u);
    pal4::test::RwDictSchemaFake schema;
    pal4::NativeBaseObjectMemory member_storage;
    AnimationRuntime animation(member_storage);
    QueueActivity queue_activity;
    pal4::NativePlayerManagerSession player_session(
        member_storage, schema, animation, queue_activity, slots, base);
    auto& players = player_session.State();
    const auto manager = player_session.Address();
    base.manager = manager;
    LoadHost load_host(events, manager);
    auto& copy_runtime = player_session.CopyRuntime();
    LoadedInventory loaded_inventory;
    EmptySkillCleanup skill_cleanup;
    LoadedProcessor loaded_host(events, copy_runtime);
    pal4::NativePlayerSlotProcessRuntime loaded_processor(
        loaded_host, skill_cleanup, &roles);
    pal4::NativePlayerLoadedDataRuntime loaded_operations(
        manager, loaded_inventory, loaded_processor);
    Check(player_session.Construct() ==
              manager &&
              player_session.Constructed() &&
              animation.initialized == 4u &&
              member_storage.OutstandingAllocations() == 16u,
          "new-game manager constructs four player slots and members");
    pal4::NativePlayerInitialResetActivity initial_reset(
        animation, 0x5000u);
    auto& first_base = players.slots[0].base;
    first_base.Write(0x564u + 4u, 0x1234u);
    first_base.Write(0x564u + 24u, 0x5678u);
    Check(pal4::BaseObjectActivity::Reset(first_base, initial_reset) == 0u &&
              first_base.Read(0x564u + 4u) == 0u &&
              first_base.Read(0x564u + 24u) == 0u &&
              member_storage.OutstandingAllocations() == 16u,
          "initial player reset clears collision binding and retains members");
    first_base.Write(0x550u, 0x1234u);
    bool rejected_loaded_material = false;
    try {
        (void)pal4::BaseObjectActivity::Reset(first_base, initial_reset);
    } catch (const std::logic_error&) {
        rejected_loaded_material = true;
    }
    first_base.Write(0x550u, 0u);
    Check(rejected_loaded_material,
          "initial reset rejects loaded material without texture services");
    first_base.Write(0x14u, 0x1234u);
    bool rejected_loaded_clump = false;
    try {
        (void)pal4::BaseObjectActivity::Reset(first_base, initial_reset);
    } catch (const std::logic_error&) {
        rejected_loaded_clump = true;
    }
    first_base.Write(0x14u, 0u);
    Check(rejected_loaded_clump,
          "initial reset rejects loaded Clump without scene services");
    bool rejected_duplicate = false;
    try {
        (void)player_session.Construct();
    } catch (const std::logic_error&) {
        rejected_duplicate = true;
    }
    Check(rejected_duplicate && animation.initialized == 4u,
          "native player owner rejects duplicate construction");
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const auto slot = index * pal4::PlayerManagerSlot::kObjectBytes;
        Check(players.Word((slot + 0x218u) / 4u) == 0x84b8c8u &&
                  players.Word((slot + 0x388u) / 4u) == 0x84b8c8u &&
                  players.Word((slot + 0x520u) / 4u) == 0xffffffffu &&
                  players.Word((slot + 0x544u) / 4u) != 0u &&
                  players.Word((slot + 0x564u + 21u * 4u) / 4u) != 0u &&
                  (players.Word((slot + 0x170u + 20u * 4u) / 4u) & 0xffu) ==
                      (((manager + slot + 0x1c0u) >> 8u) & 0xffu) &&
                  players.Word((slot + 0x170u + 21u * 4u) / 4u) == 0u &&
                  players.Word((slot + 0x654u + 4u) / 4u) == 7u &&
                  players.Word((slot + 0x654u + 0x154u) / 4u) == 7u &&
                  players.Word((slot + 0x654u + 680u) / 4u) == 0u,
              "each player owns resource, text, collision and script members");
        const auto head = players.Word((slot + 0xaacu) / 4u);
        Check(head != 0u &&
                  *reinterpret_cast<const std::uint32_t*>(head) == head &&
                  *reinterpret_cast<const std::uint32_t*>(head + 4u) == head &&
                  players.Word((slot + 0xaa8u + 0x3cu) / 4u) ==
                      0x3e4ccccdu,
              "each player owns a constructed animation event queue");
    }
    pal4::PlayerManagerOperationsRuntimeStub operation_host;
    ActorLoop actor_loop(events);
    pal4::NativePlayerManagerResetRuntime operations(
        players, actor, player_session.SlotBaseRuntime(), actor_loop,
        operation_host);
    for (auto& slot : players.slots)
        slot.base.Write(0x1f0u, 1u);
    Check(pal4::PlayerManagerOperations::ResetData(manager, operations) == 0u,
          "native manager resets all four player state flags");
    for (const auto& slot : players.slots)
        Check(slot.base.Read(0x1f0u) == 0u,
              "native player state-flag setter writes the original field");
    actor.words[6] = 7u;
    actor.words[184] = 8u;
    actor.words[223] = 9u;
    Check(operations.ClearActor(operations.ActorControl()) == 0u &&
              actor.words[6] == 0u && actor.words[184] == 0u &&
              actor.words[223] == 0u &&
              events == std::vector<std::string>{"clear_actor"},
          "native manager runs the rewritten actor clear on owned state");
    events.clear();
    ActorRuntime actor_runtime(events, 0x1234u);
    ActorAssets actor_assets;
    pal4::NativePkFileManagerRuntime packages;
    const auto package_manager = packages.InitializePackFileManager();
    std::array<std::uint32_t, 20> file_callbacks{};
    const auto file_address = Address(file_callbacks.data());
    packages.ProcessPackFiles(package_manager, file_address);
    const auto prior_directory = std::filesystem::current_path();
    std::filesystem::current_path(argv[1]);
    const auto scripts_loaded = packages.LoadAllScripts();
    std::filesystem::current_path(prior_directory);
    Check(scripts_loaded == 1u && packages.LoadedPackageCount() == 9u,
          "player script scope has the nine original packages");
    {
        pal4::NativeBaseObjectMemory actor_memory;
        pal4::NativeActorControlSession actor_session(
            actor_memory, schema, {60, 60}, &packages);
        (void)actor_session.Construct();
        bool missing_dummy_clump = false;
        try {
            (void)pal4::PalActorControl::Initialize(
                actor_session.State(), actor_session.Address(),
                actor_session.Runtime());
        } catch (const std::logic_error& error) {
            missing_dummy_clump = std::string_view(error.what()).find(
                "JDumy") != std::string_view::npos;
        }
        Check(missing_dummy_clump &&
                  packages.Scripts().ReadWord(package_manager + 120u) == 0u &&
                  file_callbacks[10] == 0u && file_callbacks[19] == 0u,
              "ActorControl script scope cleans up after JDumy boundary");
        actor_session.Destroy();
        Check(actor_memory.OutstandingAllocations() == 0u,
              "ActorControl failure leaves no owned name buffers");
    }
    pal4::NativeScriptNewGameRuntime new_game(
        roles, players, actor, operations, load_host, copy_runtime,
        actor_runtime, 0x1234u, &actor_assets, nullptr, &packages,
        &loaded_operations);

    for (std::uint32_t index = 0u; index < 4u; ++index)
        players.Word((index * pal4::PlayerManagerSlot::kObjectBytes +
                      0xaecu) / 4u) = 0xffffffffu;
    Check(pal4::ScriptNewGameHandler::Run(new_game) == 0x77 &&
              load_host.actors == 4u &&
              actor_assets.models ==
                  std::vector<std::string>{"101", "103", "106", "105"} &&
              actor_assets.names == std::vector<std::string>{
                  "Player-TIANHE", "Player-LINSHA", "Player-MENGLI",
                  "Player-ZIYING"} &&
              actor_assets.unloaded.empty() &&
              events == std::vector<std::string>{
                  "reset0", "reset1", "reset2", "reset3", "clear_actor",
                  "process0", "process1", "process2",
                  "process3", "actor_shutdown", "actor_initialize"} &&
              packages.Scripts().ReadWord(package_manager + 120u) == 0u &&
              file_callbacks[10] == 0u && file_callbacks[19] == 0u,
          "new-game callback composes player and actor operations in IDA order");
    constexpr std::uint32_t hp[]{90u, 68u, 45u, 81u};
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const auto offset = index * pal4::PlayerManagerSlot::kObjectBytes;
        Check(players.Word((offset + 0xaecu) / 4u) == 0u &&
                  players.Word((offset + 0x658u + 4u) / 4u) == hp[index] &&
                  players.Word((offset + 0xb08u) / 4u) ==
                      (index == 0u ? 1u : 0u) &&
                  players.Word((offset + 0x9f8u + 84u) / 4u) == 1001u + index,
              "original Role HP, ID and team state survive postprocessing");
    }
    Check(actor.words[206] == Address(&actor) + 140u &&
              actor.words[208] == Address(&actor) + 140u &&
              actor.words[215] == pal4::PalActorControl::kFloorSoundCallback,
          "actor control initializes embedded links and callback");
    load_host.actors = 0u;
    loaded_host.fail_after_load = true;
    bool load_failed = false;
    try {
        new_game.LoadAndProcessPlayerData(manager);
    } catch (const std::runtime_error&) {
        load_failed = true;
    }
    loaded_host.fail_after_load = false;
    Check(load_failed &&
              actor_assets.unloaded ==
                  std::vector<std::uint32_t>{
                      manager + 3u * pal4::PlayerManagerSlot::kObjectBytes,
                      manager + 2u * pal4::PlayerManagerSlot::kObjectBytes,
                      manager + pal4::PlayerManagerSlot::kObjectBytes,
                      manager} &&
              packages.Scripts().ReadWord(package_manager + 120u) == 0u &&
              file_callbacks[10] == 0u && file_callbacks[19] == 0u,
          "failed player postprocess releases actors and script scope");
    actor_assets.reject_model = "106";
    load_host.actors = 0u;
    bool actor_load_failed = false;
    try {
        new_game.LoadAndProcessPlayerData(manager);
    } catch (const std::runtime_error&) {
        actor_load_failed = true;
    }
    actor_assets.reject_model.clear();
    Check(actor_load_failed &&
              actor_assets.unloaded.size() == 6u &&
              actor_assets.unloaded[4] ==
                  manager + pal4::PlayerManagerSlot::kObjectBytes &&
              actor_assets.unloaded[5] == manager &&
              packages.Scripts().ReadWord(package_manager + 120u) == 0u &&
              file_callbacks[10] == 0u && file_callbacks[19] == 0u,
          "failed third actor load releases earlier slots and script scope");

    pal4::NativePackageArchive archive;
    const std::string package = std::string(argv[1]) +
                                "\\gamedata\\script.cpk";
    Check(archive.Open(package.c_str()), "open original script package");
    const auto script = archive.ReadMember("script.csb");
    Check(script && script->size() > sizeof(std::uint32_t),
          "read original NewGame bytecode");
    std::uint32_t size{};
    std::memcpy(&size, script->data(), sizeof(size));
    Check(size == script->size() - sizeof(size), "script bytecode size");
    pal4::NativeAngelScriptEngine engine;
    constexpr char module[] = "gamedata\\editData\\script\\script.csb";
    const auto loaded = engine.LoadBytecode(
        module, {reinterpret_cast<const std::byte*>(script->data() + 4u),
                 size});
    Check(loaded.code == 0 && loaded.bytes_read == size,
          "restore original NewGame bytecode");
    pal4::NativeScriptFrameRunner runner(engine);
    FlashRuntime flash_runtime;
    pal4::ScriptFlashEffect flash(flash_runtime);
    MusicRuntime music;
    MovieRuntime movies;
    MovieFlagRuntime flags;
    ArenaRuntime arena(Address(&actor));
    pal4::NativeGameScriptCallbacks callbacks(
        {&flash, &music, &movies, &flags, &new_game, &arena});
    engine.SetCallbacks(&callbacks);
    events.clear();
    load_host.actors = 0u;
    actor.words[0] = 1u;
    runner.Queue(module, "NewGame");
    Check(runner.Status() == pal4::NativeScriptFrameStatus::queued &&
              movies.names.empty(),
          "original NewGame is queued until a frame runs");
    Check(runner.Tick(0.016f) == pal4::NativeScriptFrameStatus::finished &&
              runner.Error().empty(),
          "original NewGame frame completes through giArenaLoad");
    Check(events == std::vector<std::string>{
              "reset0", "reset1", "reset2", "reset3", "clear_actor",
              "process0", "process1", "process2", "process3",
              "actor_shutdown",
              "actor_initialize"} &&
              load_host.actors == 4u &&
              movies.names == std::vector<std::string>{
                  "1A.bik", "Pal4A.bik"} &&
              music.pauses == 1u && music.resumes == 1u &&
              flags.flags == 1u && flags.saves == 1u &&
              engine.GlobalStoryVariable() == 10100 &&
              arena.values ==
                  std::array<std::string, 4>{"0", "Q01", "N01", ""} &&
              arena.requested_state == 6u && arena.arena_state == 0u &&
              arena.time_56c == 0x123456u &&
              arena.time_570 == 0x123456u &&
              arena.time_574 == 0x123456u &&
              packages.Scripts().ReadWord(package_manager + 120u) == 0u &&
              file_callbacks[10] == 0u && file_callbacks[19] == 0u,
          "original script requests Q01/N01 arena with state and time");
    engine.SetCallbacks(nullptr);
    Check(copy_runtime.OwnedTextCount() != 0u,
          "original Role text remains owned until manager destruction");
    player_session.Destroy();
    player_session.Destroy();
    Check(!player_session.Constructed() && animation.cleaned == 4u &&
              member_storage.OutstandingAllocations() == 0u &&
              copy_runtime.OwnedTextCount() == 0u &&
              players.Word(0x654u / 4u) ==
                  pal4::GameDataObjectLifecycle::kBaseVtable &&
              players.Word((0x9f8u + 36u) / 4u) == 0u,
          "new-game manager releases Role text in reverse slot order");
    std::cout << "PASS: original NewGame completes native player, actor "
                 "and arena request lifecycle\n";
}
