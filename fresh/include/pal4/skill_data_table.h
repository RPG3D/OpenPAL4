#pragma once

#include "pal4/game_data_table_binding_refresh.h"

#include <cstdint>
#include <array>
#include <string>
#include <string_view>

namespace pal4 {

class ISkillDataTableRuntime {
public:
    virtual ~ISkillDataTableRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void ConstructBase(std::uint32_t object,
                               std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ClearEffectData(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t DestroySkillRecord(
        std::uint32_t record) = 0;
    virtual void DestroyPropertyRecord(std::uint32_t record) = 0;
    virtual void ProcessElements(std::uint32_t address) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyBase(
        std::uint32_t object) = 0;
};

using SkillDataRecord = std::array<std::uint32_t, 33>;

class ISkillDataRowRuntime {
public:
    virtual ~ISkillDataRowRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t CollectionSize(
        std::uint32_t collection) = 0;
    virtual void LogBind(std::string_view prefix,
                         std::uint32_t name) = 0;
    virtual void AssignEffects(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void ProcessPropertyRegistry(std::uint32_t binding) = 0;
    virtual void BindEffects(std::uint32_t binding) = 0;
    virtual void InitializeRecord(SkillDataRecord& record) = 0;
    virtual void DestroyTemporaryRecord(SkillDataRecord& record) = 0;
    [[nodiscard]] virtual bool ParseBoundRow(
        std::uint32_t binding, std::uint32_t row,
        SkillDataRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t FindKey(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t key) = 0;
    virtual void InsertRecord(std::uint32_t array, std::uint32_t end,
                              const SkillDataRecord& record) = 0;
    virtual void InsertKey(std::uint32_t array, std::uint32_t position,
                           std::uint32_t key, std::uint32_t index) = 0;
    virtual void AssignRecord(std::uint32_t destination,
                              const SkillDataRecord& record) = 0;
    [[nodiscard]] virtual std::string DumpRow(
        std::uint32_t table, std::uint32_t row) = 0;
};

class ISkillDataAppendRuntime {
public:
    virtual ~ISkillDataAppendRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CollectionSize(
        std::uint32_t collection) = 0;
    [[nodiscard]] virtual std::uint32_t RowSize(std::uint32_t row) = 0;
    virtual void InitializeRecord(SkillDataRecord& record) = 0;
    [[nodiscard]] virtual std::uint8_t CleanupTemporaryRecord(
        SkillDataRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t PropertyAt(
        std::uint32_t row, std::uint32_t index) = 0;
    virtual void ConvertField(std::uint32_t field,
                              SkillDataRecord& record,
                              std::uint32_t property) = 0;
    virtual void AppendRecord(std::uint32_t array, std::uint32_t end,
                              const SkillDataRecord& record) = 0;
    [[nodiscard]] virtual std::string DumpRow(
        std::uint32_t table, std::uint32_t row) = 0;
};

class ISkillDataRefreshRuntime {
public:
    virtual ~ISkillDataRefreshRuntime() = default;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Refresh(std::uint32_t table) = 0;
};

class ISkillDataIndexRuntime {
public:
    virtual ~ISkillDataIndexRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void AssignRecord(std::uint32_t destination,
                              std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyRecord(
        std::uint32_t record) = 0;
    virtual void InsertKey(std::uint32_t array, std::uint32_t end,
                           std::uint32_t key, std::uint32_t index) = 0;
};

class ISkillDataCapacityRuntime {
public:
    virtual ~ISkillDataCapacityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void DestroyRecord(std::uint32_t record) = 0;
    virtual void CopyIndex(std::uint32_t destination,
                           std::uint32_t source) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class SkillDataTable final {
public:
    static constexpr std::uint32_t kObjectBytes = 140;
    static constexpr std::uint32_t kVtable = 0x8446ec;
    static constexpr std::uint32_t kSkillRecordBytes = 132;
    static constexpr std::uint32_t kPropertyRecordBytes = 60;

    // IDA 0x4f3d90: allocate and initialize an Effect-derived Skill table.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t name, ISkillDataTableRuntime& runtime);
    // IDA 0x4f3e60: destroy record arrays and the Effect base.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, ISkillDataTableRuntime& runtime);
    // IDA 0x4f3fa0: destroy and release a standalone Skill record array.
    [[nodiscard]] static std::uint32_t ReleaseRecordArray(
        std::uint32_t array, ISkillDataTableRuntime& runtime);
    // IDA 0x4f3fe0: clear active Skill records while retaining capacity.
    [[nodiscard]] static std::uint32_t ClearRows(
        std::uint32_t object, ISkillDataTableRuntime& runtime);
    // IDA 0x4f40c0: bind, parse, validate and insert/replace one Skill row.
    [[nodiscard]] static std::uint8_t AddOrReplace(
        std::uint32_t table, std::uint32_t row,
        ISkillDataRowRuntime& runtime);
    // IDA 0x4f43d0: convert bound fields and append one Skill record.
    [[nodiscard]] static std::uint8_t Append(
        std::uint32_t table, std::uint32_t row,
        ISkillDataAppendRuntime& runtime);
    // IDA 0x4f45c0: mark the Skill table dirty and refresh it.
    [[nodiscard]] static std::uint32_t MarkAndRefresh(
        std::uint32_t table, ISkillDataRefreshRuntime& runtime);
    // IDA 0x4f45d0: compact adjacent duplicate IDs and rebuild signed index.
    static void RebuildIndex(std::uint32_t table,
                             ISkillDataIndexRuntime& runtime);
    // IDA 0x4f4810: destroy a half-open range of 132-byte Skill records.
    [[nodiscard]] static std::uint32_t DestroyRange(
        std::uint32_t begin, std::uint32_t end,
        ISkillDataIndexRuntime& runtime);
    // IDA 0x4f4840: reserve Skill records and key/index pairs independently.
    [[nodiscard]] static std::uint32_t Reserve(
        std::uint32_t table, std::int32_t requested,
        ISkillDataCapacityRuntime& runtime);
    // IDA 0x4f76e0: refresh Skill property bindings on a count mismatch.
    [[nodiscard]] static std::uint32_t RefreshBindings(
        std::uint32_t table, IGameDataTableBindingRefreshRuntime& runtime);
};

}  // namespace pal4
