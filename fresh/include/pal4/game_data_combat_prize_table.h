#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

using GameDataCombatPrizeRecord = std::array<std::uint32_t, 32>;

class IGameDataCombatPrizeRowRuntime {
public:
    virtual ~IGameDataCombatPrizeRowRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CollectionSize(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t RowSize(std::uint32_t row) = 0;
    virtual void LogBind(std::string_view prefix, std::uint32_t name) = 0;
    virtual void AssignEffects(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void InitializeStructures(std::uint32_t binding) = 0;
    virtual void BindEffects(std::uint32_t binding) = 0;
    [[nodiscard]] virtual std::uint32_t PropertyAt(
        std::uint32_t row, std::uint32_t index) = 0;
    virtual void ConvertField(std::uint32_t field,
                              GameDataCombatPrizeRecord& record,
                              std::uint32_t property) = 0;
    [[nodiscard]] virtual std::uint32_t FindKey(
        std::uint32_t begin, std::uint32_t end, std::uint32_t key) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            const GameDataCombatPrizeRecord& record) = 0;
    virtual void InsertRecord(std::uint32_t array, std::uint32_t end,
                              const GameDataCombatPrizeRecord& record) = 0;
    virtual void InsertKey(std::uint32_t array, std::uint32_t position,
                           std::uint32_t key, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::string DumpRow(
        std::uint32_t table, std::uint32_t row) = 0;
};

class IGameDataCombatPrizeAppendRuntime {
public:
    virtual ~IGameDataCombatPrizeAppendRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CollectionSize(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t RowSize(std::uint32_t row) = 0;
    [[nodiscard]] virtual std::uint32_t PropertyAt(
        std::uint32_t row, std::uint32_t index) = 0;
    virtual void ConvertField(std::uint32_t field,
                              GameDataCombatPrizeRecord& record,
                              std::uint32_t property) = 0;
    [[nodiscard]] virtual std::uint32_t AppendRecord(
        std::uint32_t array, std::uint32_t end,
        const GameDataCombatPrizeRecord& record) = 0;
    [[nodiscard]] virtual std::string DumpRow(
        std::uint32_t table, std::uint32_t row) = 0;
};

class IGameDataCombatPrizeRefreshRuntime {
public:
    virtual ~IGameDataCombatPrizeRefreshRuntime() = default;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Refresh(std::uint32_t table) = 0;
};

class GameDataCombatPrizeTable final {
public:
    // IDA 0x4eb880, 0x4ebb70, 0x4ebcd0.
    [[nodiscard]] static std::uint8_t AddOrReplace(
        std::uint32_t table, std::uint32_t row,
        IGameDataCombatPrizeRowRuntime& runtime);
    [[nodiscard]] static std::uint32_t Append(
        std::uint32_t table, std::uint32_t row,
        IGameDataCombatPrizeAppendRuntime& runtime);
    [[nodiscard]] static std::uint32_t MarkAndRefresh(
        std::uint32_t table, IGameDataCombatPrizeRefreshRuntime& runtime);
};

}  // namespace pal4
