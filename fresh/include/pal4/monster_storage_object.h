#pragma once

#include "pal4/monster_behavior_manager.h"
#include "pal4/script_attribute_tree_lookup.h"
#include "pal4/string_buffer.h"

#include <cstdint>
#include <optional>

namespace pal4 {

struct MonsterStorageRecordState {
    std::uint32_t vtable{};
    std::uint32_t flags{};
    std::uint32_t text_data{};
    std::uint32_t text_length{};
    std::uint32_t text_capacity{};
    std::uint32_t property{};
};
static_assert(sizeof(MonsterStorageRecordState) == 0x18u);

struct MonsterStorageValues {
    std::uint32_t name{};
    std::uint32_t property{};
};

class IMonsterStorageCaptureRuntime {
public:
    virtual ~IMonsterStorageCaptureRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t StorageRequirement(std::uint32_t document) = 0;
};

class IMonsterStorageRuntime : public IMonsterStorageCaptureRuntime {
public:
    virtual ~IMonsterStorageRuntime() = default;
    [[nodiscard]] std::int32_t StorageRequirement(std::uint32_t document) override;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CreateQuery(const char* name) = 0;
    virtual void DestroyQuery(std::uint32_t query_address) = 0;
    [[nodiscard]] virtual IScriptAttributeTreeRuntime& TreeRuntime() = 0;
    [[nodiscard]] virtual std::int32_t ParseInt(
        std::uint32_t attribute_address, std::int32_t fallback) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateObject(
        std::uint32_t bytes) = 0;
    virtual void FreeObject(std::uint32_t address) = 0;
    [[nodiscard]] virtual MonsterStorageRecordState& RecordAt(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t IncomingByte() = 0;
    [[nodiscard]] virtual std::uint32_t CStringLength(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual IStringAssignRuntime& StringRuntime() = 0;
    [[noreturn]] virtual void NullObjectDereference() = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t FreeStringStorage(
        std::uint32_t address) = 0;
};

class MonsterStorageRuntimeStub final : public IMonsterStorageRuntime {
public:
    std::uint32_t ReadDword(std::uint32_t address) override;
    std::uint32_t CreateQuery(const char* name) override;
    void DestroyQuery(std::uint32_t query_address) override;
    IScriptAttributeTreeRuntime& TreeRuntime() override;
    std::int32_t ParseInt(std::uint32_t attribute_address,
                          std::int32_t fallback) override;
    std::uint32_t AllocateObject(std::uint32_t bytes) override;
    void FreeObject(std::uint32_t address) override;
    MonsterStorageRecordState& RecordAt(std::uint32_t address) override;
    std::uint8_t IncomingByte() override;
    std::uint32_t CStringLength(std::uint32_t address) override;
    IStringAssignRuntime& StringRuntime() override;
    [[noreturn]] void NullObjectDereference() override;
    std::uint8_t ReadByte(std::uint32_t address) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    std::uint32_t FreeStringStorage(std::uint32_t address) override;

private:
    ScriptAttributeTreeRuntimeStub tree_{};
    MonsterStorageRecordState record_{};
};

class MonsterStorageObject final {
public:
    // IDA 0x433640: vtable slot +4 is a thunk returning 11.
    [[nodiscard]] static std::uint32_t GetTypeCode();
    [[nodiscard]] static std::uint32_t InitializeRecord(
        MonsterStorageRecordState& record, std::uint32_t address,
        std::uint8_t incoming_byte);
    [[nodiscard]] static std::uint32_t AllocateAndInitialize(
        IMonsterStorageRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        const MonsterBehaviorManagerState& manager, std::uint32_t address,
        IMonsterStorageRuntime& runtime);
    // Shared game gate/values before the legacy allocation and string boundary.
    [[nodiscard]] static std::optional<MonsterStorageValues> CaptureValues(
        const MonsterBehaviorManagerState& manager,
        IMonsterStorageCaptureRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        MonsterStorageRecordState& record, IMonsterStorageRuntime& runtime);
};

}  // namespace pal4
