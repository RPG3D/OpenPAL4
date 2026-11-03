#pragma once

#include <cstdint>
#include <array>
#include <string>
#include <string_view>

namespace pal4 {

class IUtilitiesIntroductionFactoryRuntime {
public:
    virtual ~IUtilitiesIntroductionFactoryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t size) = 0;
    virtual void ConstructEffect(std::uint32_t object,
                                 std::uint32_t source) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
};

class IUtilitiesIntroductionLifetimeRuntime {
public:
    virtual ~IUtilitiesIntroductionLifetimeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void ClearEffect(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t DestructEffect(
        std::uint32_t object) = 0;
    virtual void ReleaseText(std::uint32_t address) = 0;
    virtual void ProcessElements(std::uint32_t address) = 0;
    virtual void DestroyBinding(std::uint32_t address,
                                std::uint32_t flag) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class IUtilitiesIntroductionArrayRuntime {
public:
    virtual ~IUtilitiesIntroductionArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void DestroyRecord(std::uint32_t record) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class IUtilitiesIntroductionClearRuntime {
public:
    virtual ~IUtilitiesIntroductionClearRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ClearEffect(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyRecord(
        std::uint32_t record) = 0;
};

using UtilitiesIntroductionRecord = std::array<std::uint32_t, 9>;

class IUtilitiesIntroductionRowRuntime {
public:
    virtual ~IUtilitiesIntroductionRowRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t CollectionSize(
        std::uint32_t collection) = 0;
    virtual void LogBind(std::string_view prefix,
                         std::uint32_t name) = 0;
    virtual void AssignEffects(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void RegisterIntroductionFields(
        std::uint32_t binding) = 0;
    virtual void BindEffects(std::uint32_t binding) = 0;
    virtual void InitializeRecord(UtilitiesIntroductionRecord& record) = 0;
    virtual void DestroyTemporaryRecord(
        UtilitiesIntroductionRecord& record) = 0;
    [[nodiscard]] virtual bool ParseBoundRow(
        std::uint32_t binding, std::uint32_t row,
        UtilitiesIntroductionRecord& record) = 0;
    virtual void InsertRecord(std::uint32_t array, std::uint32_t end,
                              const UtilitiesIntroductionRecord& record) = 0;
    virtual void InsertKey(std::uint32_t array, std::uint32_t position,
                           std::uint32_t key, std::uint32_t index) = 0;
    virtual void AssignRecord(std::uint32_t destination,
                              const UtilitiesIntroductionRecord& record) = 0;
    [[nodiscard]] virtual std::string DumpRow(
        std::uint32_t table, std::uint32_t row) = 0;
};

class IUtilitiesIntroductionAppendRuntime {
public:
    virtual ~IUtilitiesIntroductionAppendRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t RowSize(
        std::uint32_t row) = 0;
    [[nodiscard]] virtual std::uint32_t CollectionSize(
        std::uint32_t collection) = 0;
    virtual void InitializeRecord(UtilitiesIntroductionRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupTemporaryRecord(
        UtilitiesIntroductionRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t PropertyAt(
        std::uint32_t row, std::uint32_t index) = 0;
    virtual void ConvertField(std::uint32_t binding,
                              UtilitiesIntroductionRecord& record,
                              std::uint32_t property) = 0;
    virtual void AppendRecord(std::uint32_t array, std::uint32_t end,
                              const UtilitiesIntroductionRecord& record) = 0;
    [[nodiscard]] virtual std::string DumpRow(
        std::uint32_t table, std::uint32_t row) = 0;
};

class IUtilitiesIntroductionRefreshRuntime {
public:
    virtual ~IUtilitiesIntroductionRefreshRuntime() = default;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Refresh(
        std::uint32_t object) = 0;
};

class IUtilitiesIntroductionCapacityRuntime {
public:
    virtual ~IUtilitiesIntroductionCapacityRuntime() = default;
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

class UtilitiesIntroductionArray final {
public:
    // IDA 0x503060: destroy 36-byte rows and reset array pointers.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t array,
        IUtilitiesIntroductionArrayRuntime& runtime);
};

class UtilitiesIntroductionTable final {
public:
    // IDA 0x502e50: create the Introduction Effect-derived table.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t source,
        IUtilitiesIntroductionFactoryRuntime& runtime);
    // IDA 0x502f20: release 36-byte rows and all three buffers.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object,
        IUtilitiesIntroductionLifetimeRuntime& runtime);
    // IDA 0x5030a0: clear active rows while retaining storage.
    [[nodiscard]] static std::uint32_t ClearRows(
        std::uint32_t object,
        IUtilitiesIntroductionClearRuntime& runtime);
    // IDA 0x503110: parse and insert or replace a 36-byte row.
    [[nodiscard]] static std::uint8_t AddOrReplace(
        std::uint32_t object, std::uint32_t row,
        IUtilitiesIntroductionRowRuntime& runtime);
    // IDA 0x503390: convert bound fields and append a 36-byte row.
    [[nodiscard]] static std::uint32_t Append(
        std::uint32_t object, std::uint32_t row,
        IUtilitiesIntroductionAppendRuntime& runtime);
    // IDA 0x503530: set dirty byte before forwarding refresh.
    [[nodiscard]] static std::uint32_t MarkAndRefresh(
        std::uint32_t object,
        IUtilitiesIntroductionRefreshRuntime& runtime);
    // IDA 0x503540: independently reserve 36-byte rows and 8-byte keys.
    [[nodiscard]] static std::uint32_t Reserve(
        std::uint32_t object, std::int32_t requested,
        IUtilitiesIntroductionCapacityRuntime& runtime);
};

}  // namespace pal4
