#pragma once

#include <cstdint>
#include <initializer_list>

namespace pal4 {

enum class ArenaCacheEntryCall : std::uint8_t {
    DeleteChild,
    PrepareData,
    WriteData,
    GetChildValue,
    ProcessChild,
    ReadTextRecord,
    ReadDataWord,
    FindStorageObject,
    DeserializeObject,
    AllocateText,
    IterateGameObjects,
    IterateMonsters,
    CaptureMinimap,
    SnapshotGameObject,
    SnapshotMonster,
    GetChildName,
    FindGameObjectByName,
    FindMonsterByName,
    RestoreGameObject,
    RestoreMonster,
    RestoreMinimap,
    RegisterArenaCacheSingleton,
    LogCacheEvent,
    WriteDataWord,
    AssignTextRecord,
};


class IArenaCacheEntryRuntime {
public:
    virtual ~IArenaCacheEntryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual bool NamesEqualIgnoreCase(
        std::uint32_t left, std::uint32_t right) = 0;
    [[nodiscard]] virtual std::uint32_t Call(
        ArenaCacheEntryCall operation,
        std::initializer_list<std::uint32_t> arguments) = 0;
};

class ArenaCacheEntryRuntimeStub final : public IArenaCacheEntryRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t) override { return 0u; }
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    [[nodiscard]] std::uint32_t Free(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t) override { return 0u; }
    [[nodiscard]] bool NamesEqualIgnoreCase(std::uint32_t,
                                             std::uint32_t) override {
        return false;
    }
    [[nodiscard]] std::uint32_t Call(
        ArenaCacheEntryCall,
        std::initializer_list<std::uint32_t>) override { return 0u; }
};

class ArenaCacheEntry final {
public:
    static constexpr std::uint32_t kVtable = 0x84489cu;

    // IDA 0x51db80: four byte fields use the top byte of saved ECX.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t entry, IArenaCacheEntryRuntime& runtime);
    // IDA 0x51dbd0: deleting destructor.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t entry, std::uint8_t flags,
        IArenaCacheEntryRuntime& runtime);
    // IDA 0x51dbf0: releases two direct and two reference-counted buffers.
    [[nodiscard]] static std::uint32_t Cleanup(
        std::uint32_t entry, IArenaCacheEntryRuntime& runtime);
    // IDA 0x51dc90: delete children and reset both pointer arrays.
    [[nodiscard]] static std::uint32_t ClearChildren(
        std::uint32_t entry, IArenaCacheEntryRuntime& runtime);
    // IDA 0x51dd30: two case-insensitive names, with empty-string fallback.
    [[nodiscard]] static std::uint32_t MatchesNames(
        std::uint32_t entry, std::uint32_t first_name,
        std::uint32_t second_name, IArenaCacheEntryRuntime& runtime);
    // IDA 0x51dd80: process both child lists into an output data object.
    [[nodiscard]] static std::uint32_t ProcessChildren(
        std::uint32_t entry, std::uint32_t output,
        IArenaCacheEntryRuntime& runtime);
    // IDA 0x51dea0: load two child arrays and the minimap snapshot.
    [[nodiscard]] static std::uint32_t Load(
        std::uint32_t entry, std::uint32_t data,
        IArenaCacheEntryRuntime& runtime);
    // IDA 0x51dfd0: capture live objects, monsters and minimap state.
    [[nodiscard]] static std::uint32_t Capture(
        std::uint32_t entry, std::uint32_t first_name,
        std::uint32_t second_name, IArenaCacheEntryRuntime& runtime);
    // IDA 0x51e0b0: restore captured children by their live names.
    static void Restore(std::uint32_t entry, IArenaCacheEntryRuntime& runtime);
    // IDA 0x51e150 / 0x51e310: traversal callbacks.
    [[nodiscard]] static std::uint32_t CaptureGameObject(
        std::uint32_t object, std::uint32_t entry,
        IArenaCacheEntryRuntime& runtime);
    [[nodiscard]] static std::uint32_t CaptureMonster(
        std::uint32_t monster, std::uint32_t entry,
        IArenaCacheEntryRuntime& runtime);
    // IDA 0x51ee10: copy-construct a 72-byte entry.
    static void CopyConstruct(std::uint32_t destination, std::uint32_t source,
                              IArenaCacheEntryRuntime& runtime);
};

}  // namespace pal4
