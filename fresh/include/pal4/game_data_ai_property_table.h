#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

using GameDataAiPropertyRecord = std::array<std::uint32_t, 17>;

class IGameDataAiPropertyTableRuntime {
public:
    virtual ~IGameDataAiPropertyTableRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CollectionSize(std::uint32_t collection) = 0;
    [[nodiscard]] virtual std::uint32_t RowSize(std::uint32_t row) = 0;
    virtual void LogBind(std::string_view prefix, std::uint32_t table_name) = 0;
    virtual void AssignEffects(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void InitializeStructures(std::uint32_t object) = 0;
    virtual void BindEffects(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t PropertyAt(std::uint32_t row,
                                                    std::uint32_t index) = 0;
    virtual void ConvertField(std::uint32_t field,
                              GameDataAiPropertyRecord& record,
                              std::uint32_t property) = 0;
    [[nodiscard]] virtual std::uint32_t FindKey(std::uint32_t begin,
                                                std::uint32_t end,
                                                std::uint32_t key) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            const GameDataAiPropertyRecord& record) = 0;
    virtual void InsertRecord(std::uint32_t array, std::uint32_t position,
                              const GameDataAiPropertyRecord& record) = 0;
    virtual void InsertKey(std::uint32_t array, std::uint32_t position,
                           std::uint32_t key, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t AppendRecord(
        std::uint32_t array, std::uint32_t end,
        const GameDataAiPropertyRecord& record) = 0;
    [[nodiscard]] virtual std::string DumpRow(std::uint32_t table,
                                               std::uint32_t row) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Refresh(std::uint32_t table) = 0;
};

class GameDataAiPropertyTable final {
public:
    // IDA 0x4ccd70: bind, check values, then replace or insert by key.
    [[nodiscard]] static std::uint8_t AddOrReplace(
        std::uint32_t table, std::uint32_t row,
        IGameDataAiPropertyTableRuntime& runtime);
    // IDA 0x4cd070: check values and append to the 68-byte array.
    [[nodiscard]] static std::uint32_t Append(
        std::uint32_t table, std::uint32_t row,
        IGameDataAiPropertyTableRuntime& runtime);
    // IDA 0x4cd1d0: set byte +88 and refresh the property table.
    [[nodiscard]] static std::uint32_t MarkAndRefresh(
        std::uint32_t table, IGameDataAiPropertyTableRuntime& runtime);
};

}  // namespace pal4
