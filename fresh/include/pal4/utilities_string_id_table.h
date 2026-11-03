#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IUtilitiesStringIdFactoryRuntime {
public:
    virtual ~IUtilitiesStringIdFactoryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void ConstructEffect(std::uint32_t object,
                                 std::uint32_t source) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
};

class IUtilitiesStringIdLifetimeRuntime {
public:
    virtual ~IUtilitiesStringIdLifetimeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void ClearEffect(std::uint32_t object) = 0;
    virtual void ProcessElements(std::uint32_t address) = 0;
    virtual void DestroyBinding(std::uint32_t address,
                                std::uint32_t flag) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DestructEffect(
        std::uint32_t object) = 0;
};

class IUtilitiesStringIdClearRuntime {
public:
    virtual ~IUtilitiesStringIdClearRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void ClearEffect(std::uint32_t object) = 0;
};

using UtilitiesStringIdRecord = std::array<std::uint32_t, 2>;

class IUtilitiesStringIdRowRuntime {
public:
    virtual ~IUtilitiesStringIdRowRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t RowSize(
        std::uint32_t row) = 0;
    [[nodiscard]] virtual std::int32_t CollectionSize(
        std::uint32_t collection) = 0;
    virtual void LogBind(std::string_view prefix,
                         std::uint32_t name) = 0;
    virtual void AssignEffects(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void RegisterStringIdFields(std::uint32_t binding) = 0;
    virtual void BindEffects(std::uint32_t binding) = 0;
    [[nodiscard]] virtual std::uint32_t PropertyAt(
        std::uint32_t row, std::uint32_t index) = 0;
    virtual void ConvertField(std::uint32_t binding,
                              UtilitiesStringIdRecord& record,
                              std::uint32_t property) = 0;
    virtual void InsertRecord(std::uint32_t array, std::uint32_t end,
                              const UtilitiesStringIdRecord& record) = 0;
    virtual void InsertKey(std::uint32_t array, std::uint32_t position,
                           std::uint32_t key, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::string DumpRow(
        std::uint32_t table, std::uint32_t row) = 0;
};

class IUtilitiesStringIdAppendRuntime {
public:
    virtual ~IUtilitiesStringIdAppendRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t RowSize(
        std::uint32_t row) = 0;
    [[nodiscard]] virtual std::int32_t CollectionSize(
        std::uint32_t collection) = 0;
    [[nodiscard]] virtual std::uint32_t PropertyAt(
        std::uint32_t row, std::uint32_t index) = 0;
    virtual void ConvertField(std::uint32_t binding,
                              UtilitiesStringIdRecord& record,
                              std::uint32_t property) = 0;
    [[nodiscard]] virtual std::uint32_t AppendRecord(
        std::uint32_t array, std::uint32_t end,
        const UtilitiesStringIdRecord& record) = 0;
    [[nodiscard]] virtual std::string DumpRow(
        std::uint32_t table, std::uint32_t row) = 0;
};

class IUtilitiesStringIdRefreshRuntime {
public:
    virtual ~IUtilitiesStringIdRefreshRuntime() = default;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Refresh(
        std::uint32_t object) = 0;
};

class IUtilitiesStringIdIndexRuntime {
public:
    virtual ~IUtilitiesStringIdIndexRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void InsertKey(std::uint32_t array, std::uint32_t at,
                           std::uint32_t key, std::uint32_t index) = 0;
};

class IUtilitiesStringIdCapacityRuntime {
public:
    virtual ~IUtilitiesStringIdCapacityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class UtilitiesStringIdTable final {
public:
    // IDA 0x504d00: create the StringId Effect-derived table.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t source, IUtilitiesStringIdFactoryRuntime& runtime);
    // IDA 0x504dd0: release bindings and three buffers without row cleanup.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IUtilitiesStringIdLifetimeRuntime& runtime);
    // IDA 0x504ed0: clear Effect data and reset active row end.
    [[nodiscard]] static std::uint32_t ClearRows(
        std::uint32_t object, IUtilitiesStringIdClearRuntime& runtime);
    // IDA 0x504f00: bind, convert and insert or replace an 8-byte row.
    [[nodiscard]] static std::uint8_t AddOrReplace(
        std::uint32_t object, std::uint32_t row,
        IUtilitiesStringIdRowRuntime& runtime);
    // IDA 0x5051d0: convert bound fields and append an 8-byte row.
    [[nodiscard]] static std::uint32_t Append(
        std::uint32_t object, std::uint32_t row,
        IUtilitiesStringIdAppendRuntime& runtime);
    // IDA 0x505320: mark dirty and forward refresh.
    [[nodiscard]] static std::uint32_t MarkAndRefresh(
        std::uint32_t object,
        IUtilitiesStringIdRefreshRuntime& runtime);
    // IDA 0x505330: compact adjacent equal IDs and rebuild sorted index.
    static void DeduplicateAndRebuildIndex(
        std::uint32_t object,
        IUtilitiesStringIdIndexRuntime& runtime);
    // IDA 0x505540: reserve both 8-byte value arrays independently.
    [[nodiscard]] static std::uint32_t Reserve(
        std::uint32_t object, std::int32_t requested,
        IUtilitiesStringIdCapacityRuntime& runtime);
};

}  // namespace pal4
