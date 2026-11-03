#pragma once

#include "pal4/utilities_record_array_runtime.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IUtilitiesSceneFactoryRuntime {
public:
    virtual ~IUtilitiesSceneFactoryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void ConstructEffect(std::uint32_t object,
                                 std::uint32_t source) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
};

class IUtilitiesSceneLifetimeRuntime {
public:
    virtual ~IUtilitiesSceneLifetimeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void ClearEffect(std::uint32_t object) = 0;
    virtual void ReleaseText(std::uint32_t address) = 0;
    virtual void ProcessElements(std::uint32_t address) = 0;
    virtual void DestroyBinding(std::uint32_t address,
                                std::uint32_t flag) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DestructEffect(
        std::uint32_t object) = 0;
};

class IUtilitiesSceneArrayRuntime {
public:
    virtual ~IUtilitiesSceneArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void DestroyRecord(std::uint32_t record) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class IUtilitiesSceneClearRuntime {
public:
    virtual ~IUtilitiesSceneClearRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ClearEffect(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyRecord(
        std::uint32_t record) = 0;
};

using UtilitiesSceneRecord = std::array<std::uint32_t, 15>;

class IUtilitiesSceneRowRuntime {
public:
    virtual ~IUtilitiesSceneRowRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t CollectionSize(
        std::uint32_t collection) = 0;
    virtual void LogBind(std::string_view prefix,
                         std::uint32_t name) = 0;
    virtual void AssignEffects(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void RegisterSceneFields(std::uint32_t binding) = 0;
    virtual void BindEffects(std::uint32_t binding) = 0;
    virtual void InitializeRecord(UtilitiesSceneRecord& record) = 0;
    virtual void DestroyTemporaryRecord(
        UtilitiesSceneRecord& record) = 0;
    [[nodiscard]] virtual bool ParseBoundRow(
        std::uint32_t binding, std::uint32_t row,
        UtilitiesSceneRecord& record) = 0;
    virtual void UppercaseText(UtilitiesSceneRecord& record,
                               std::uint32_t text_offset) = 0;
    virtual void InsertRecord(std::uint32_t array, std::uint32_t end,
                              const UtilitiesSceneRecord& record) = 0;
    virtual void InsertKey(std::uint32_t array, std::uint32_t position,
                           std::uint32_t key, std::uint32_t index) = 0;
    virtual void AssignRecord(std::uint32_t destination,
                              const UtilitiesSceneRecord& record) = 0;
    [[nodiscard]] virtual std::string DumpRow(
        std::uint32_t table, std::uint32_t row) = 0;
};

class IUtilitiesSceneAppendRuntime {
public:
    virtual ~IUtilitiesSceneAppendRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t RowSize(
        std::uint32_t row) = 0;
    [[nodiscard]] virtual std::uint32_t CollectionSize(
        std::uint32_t collection) = 0;
    virtual void InitializeRecord(UtilitiesSceneRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupTemporaryRecord(
        UtilitiesSceneRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t PropertyAt(
        std::uint32_t row, std::uint32_t index) = 0;
    virtual void ConvertField(std::uint32_t binding,
                              UtilitiesSceneRecord& record,
                              std::uint32_t property) = 0;
    virtual void UppercaseText(UtilitiesSceneRecord& record,
                               std::uint32_t text_offset) = 0;
    virtual void AppendRecord(std::uint32_t array, std::uint32_t end,
                              const UtilitiesSceneRecord& record) = 0;
    [[nodiscard]] virtual std::string DumpRow(
        std::uint32_t table, std::uint32_t row) = 0;
};

class IUtilitiesSceneRefreshRuntime {
public:
    virtual ~IUtilitiesSceneRefreshRuntime() = default;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Refresh(
        std::uint32_t object) = 0;
};

class IUtilitiesSceneIndexRuntime {
public:
    virtual ~IUtilitiesSceneIndexRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyRecord(
        std::uint32_t record) = 0;
    virtual void InsertKey(std::uint32_t array, std::uint32_t at,
                           std::uint32_t key, std::uint32_t index) = 0;
};

class IUtilitiesSceneCapacityRuntime {
public:
    virtual ~IUtilitiesSceneCapacityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void DestroyRecord(std::uint32_t record) = 0;
};

class UtilitiesSceneArray final {
public:
    // IDA 0x5038c0: destroy 60-byte rows and reset the array header.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t array, IUtilitiesSceneArrayRuntime& runtime);
    // IDA 0x504070: clean a half-open range of 60-byte rows.
    [[nodiscard]] static std::uint32_t DestroyRange(
        std::uint32_t begin, std::uint32_t end,
        IUtilitiesSceneIndexRuntime& runtime);
    // IDA 0x506a00: insert one 60-byte Scene record.
    [[nodiscard]] static std::uint32_t Insert(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t source, IUtilitiesRecordArrayRuntime& runtime);
    // IDA 0x506bf0: copy-construct a half-open 60-byte record range.
    [[nodiscard]] static std::uint32_t CopyRange(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination, IUtilitiesRecordArrayRuntime& runtime);
    // IDA 0x507be0: insert repeated 60-byte Scene records.
    static void InsertCopies(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t count, std::uint32_t source,
        IUtilitiesRecordArrayRuntime& runtime);
    // IDA 0x508680: construct repeated 60-byte Scene records.
    static void Fill(std::uint32_t destination, std::uint32_t count,
                     std::uint32_t source,
                     IUtilitiesRecordArrayRuntime& runtime);
};

class UtilitiesSceneTable final {
public:
    // IDA 0x5036b0: create the Scene Effect-derived table.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t source, IUtilitiesSceneFactoryRuntime& runtime);
    // IDA 0x503780: release text, bindings and all three buffers.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IUtilitiesSceneLifetimeRuntime& runtime);
    // IDA 0x503900: clear active rows while retaining their storage.
    [[nodiscard]] static std::uint32_t ClearRows(
        std::uint32_t object, IUtilitiesSceneClearRuntime& runtime);
    // IDA 0x503980: parse, uppercase and insert or replace a Scene row.
    [[nodiscard]] static std::uint8_t AddOrReplace(
        std::uint32_t object, std::uint32_t row,
        IUtilitiesSceneRowRuntime& runtime);
    // IDA 0x503c50: convert, uppercase and append a Scene row.
    [[nodiscard]] static std::uint32_t Append(
        std::uint32_t object, std::uint32_t row,
        IUtilitiesSceneAppendRuntime& runtime);
    // IDA 0x503e20: mark the Scene table dirty and forward refresh.
    [[nodiscard]] static std::uint32_t MarkAndRefresh(
        std::uint32_t object,
        IUtilitiesSceneRefreshRuntime& runtime);
    // IDA 0x503e30: compact adjacent duplicate rows and rebuild key index.
    static void DeduplicateAndRebuildIndex(
        std::uint32_t object,
        IUtilitiesSceneIndexRuntime& runtime);
    // IDA 0x5040a0: reserve 60-byte rows and 8-byte keys independently.
    [[nodiscard]] static std::uint32_t Reserve(
        std::uint32_t object, std::int32_t requested,
        IUtilitiesSceneCapacityRuntime& runtime);
};

}  // namespace pal4
