#pragma once

#include "pal4/arena_actor_resource.h"

#include <cstdint>
#include <functional>
#include <string_view>

namespace pal4 {

enum class ArenaSkillCall : std::uint8_t {
    NeedsContentUpdate,
    UpdateContent,
    GetGameObjectManager,
    ProcessMinimapOne,
    ProcessMinimapThree,
    ResetEffectHandle,
    InterpolateAlpha,
    SetMaterialFlags,
    GetGameArenaManager,
    RemoveClump,
    DestroyActor,
    GetPlayerManager,
    GetPlayerByIndex,
    InitializeRecordName,
    LoadSnareResource,
    SetActorPosition,
    SetActorRotation,
    SetAnimationFlag,
    AddClump,
    CreateEnthrallEffect,
    InsertActiveRecord,
    CreateSignpostEffect,
    LoadSignpostResource,
    CreateSignpostResumeEffect,
    IsObjectVisible,
    GetMinimapManager,
    ConvertGridCoordinates,
    AddTreasureIcon,
    AddOreIcon,
    RemoveMinimapIcon,
};

class IArenaSkillObjectReader {
public:
    virtual ~IArenaSkillObjectReader() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
};

class IArenaSkillRuntime : public IArenaActorRuntime,
                           public IArenaSkillObjectReader {
public:
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) override = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t pointer) = 0;
    // Native owners may supply the same original492-byte object with a typed
    // SDK resource owner. The default retains the original factory recipe.
    [[nodiscard]] virtual std::uint32_t CreateArenaActor();
    [[nodiscard]] virtual std::uint32_t RegisterSingleton(
        std::string_view name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t deleter) = 0;
    [[nodiscard]] virtual std::uint32_t SkillCall(
        ArenaSkillCall operation,
        std::initializer_list<std::uint32_t> arguments) = 0;
    virtual void PlayActionSound(std::string_view filename) = 0;
};

class ArenaSkillRuntimeStub final : public IArenaSkillRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    [[nodiscard]] std::uint32_t Call(
        ArenaActorCall,
        std::initializer_list<std::uint32_t>) override { return 0u; }
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t Free(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t RegisterSingleton(
        std::string_view, std::uint32_t,
        std::uint32_t, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t SkillCall(
        ArenaSkillCall,
        std::initializer_list<std::uint32_t>) override { return 0u; }
    void PlayActionSound(std::string_view) override {}
};

class ArenaSkillManager final {
public:
    static constexpr std::uint32_t kObjectSize = 56u;
    static constexpr std::uint32_t kSingletonId = 1052u;
    static constexpr std::string_view kSingletonName = "palGameArenaSkillMgr";

    // IDA 0x520f30: register the guarded singleton.
    [[nodiscard]] static std::uint32_t Register(
        std::uint8_t& guard, std::uint32_t& registered,
        IArenaSkillRuntime& runtime);
    // IDA 0x520f70/0x520fd0: factory and deletion callback.
    [[nodiscard]] static std::uint32_t Create(IArenaSkillRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t manager, IArenaSkillRuntime& runtime);
    // IDA 0x520ff0/0x521030: initialize and release three pointer arrays.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t manager, IArenaSkillRuntime& runtime);
    [[nodiscard]] static std::uint32_t Cleanup(
        std::uint32_t manager, IArenaSkillRuntime& runtime);
    // IDA 0x521080: render the actor at +32 of each first-array entry.
    [[nodiscard]] static std::uint32_t UpdateObjectList(
        std::uint32_t manager, std::uint32_t pass,
        IArenaSkillRuntime& runtime);
    // IDA 0x5210c0: update timed records in the first array.
    [[nodiscard]] static std::uint32_t UpdateTimedObjects(
        std::uint32_t manager, float delta, IArenaSkillRuntime& runtime);
    // IDA 0x521220: reset a record's embedded effect handle.
    [[nodiscard]] static std::uint32_t ResetContent(
        std::uint32_t record, IArenaSkillRuntime& runtime);
    // IDA 0x521230: advance and retire 12-byte fading records.
    [[nodiscard]] static std::uint32_t UpdateFadeState(
        std::uint32_t manager, float delta, IArenaSkillRuntime& runtime);
    // IDA 0x521370: clear live, queued and fading records.
    [[nodiscard]] static std::uint32_t ClearActiveObjects(
        std::uint32_t manager, IArenaSkillRuntime& runtime);
    // IDA 0x521510: visit the first array until callback zero or end.
    [[nodiscard]] static std::uint32_t ForEachObject(
        std::uint32_t manager,
        const std::function<std::uint32_t(std::uint32_t, std::uint32_t)>& callback,
        std::uint32_t argument, IArenaSkillObjectReader& runtime);
    // IDA 0x521550: dispatch player arena actions 0-3.
    [[nodiscard]] static std::uint32_t HandlePlayerAction(
        std::uint32_t manager, std::uint32_t player_index,
        IArenaSkillRuntime& runtime);
    // IDA 0x521c00: keep up to ten signposts in the second pointer array.
    [[nodiscard]] static std::uint32_t ShowSignpost(
        std::uint32_t manager, std::uint32_t player_index,
        IArenaSkillRuntime& runtime);
    // IDA 0x522020/0x522190: detach and restore retained arena records.
    [[nodiscard]] static std::uint32_t CleanupPlayerData(
        std::uint32_t manager, IArenaSkillRuntime& runtime);
    [[nodiscard]] static std::uint32_t UpdatePlayerData(
        std::uint32_t manager, IArenaSkillRuntime& runtime);
    // IDA 0x522220/0x5222d0: treasure and ore minimap callbacks.
    [[nodiscard]] static std::uint32_t UpdateTreasureMinimap(
        std::uint32_t object, bool enabled, IArenaSkillRuntime& runtime);
    [[nodiscard]] static std::uint32_t UpdateOreMinimap(
        std::uint32_t object, bool enabled, IArenaSkillRuntime& runtime);
};

}  // namespace pal4
