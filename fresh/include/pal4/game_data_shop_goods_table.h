#pragma once

#include "pal4/game_data_field_schema.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

using GameDataShopGoodsRecord = std::array<std::uint32_t, 9>;

class IGameDataShopGoodsRowRuntime : public IGameDataFieldSchemaRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CollectionSize(
        std::uint32_t address) = 0;
    virtual void LogBind(std::string_view prefix,
                         std::uint32_t name) = 0;
    virtual void AssignEffects(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void BindEffects(std::uint32_t binding) = 0;
    virtual void InitializeRecord(GameDataShopGoodsRecord& record) = 0;
    virtual void DestroyTemporaryRecord(GameDataShopGoodsRecord& record) = 0;
    [[nodiscard]] virtual bool ParseBoundRow(
        std::uint32_t binding, std::uint32_t row,
        GameDataShopGoodsRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t FindKey(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t key) = 0;
    virtual void InsertRecord(std::uint32_t array, std::uint32_t end,
                              const GameDataShopGoodsRecord& record) = 0;
    virtual void InsertKey(std::uint32_t array, std::uint32_t position,
                           std::uint32_t key, std::uint32_t index) = 0;
    virtual void AssignRecord(std::uint32_t destination,
                              const GameDataShopGoodsRecord& record) = 0;
    [[nodiscard]] virtual std::string DumpRow(
        std::uint32_t table, std::uint32_t row) = 0;
};

class IGameDataShopGoodsAppendRuntime {
public:
    virtual ~IGameDataShopGoodsAppendRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CollectionSize(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t RowSize(std::uint32_t row) = 0;
    virtual void InitializeRecord(GameDataShopGoodsRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupTemporaryRecord(
        GameDataShopGoodsRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t PropertyAt(
        std::uint32_t row, std::uint32_t index) = 0;
    virtual void ConvertField(std::uint32_t field,
                              GameDataShopGoodsRecord& record,
                              std::uint32_t property) = 0;
    virtual void AppendRecord(std::uint32_t array, std::uint32_t end,
                              const GameDataShopGoodsRecord& record) = 0;
    [[nodiscard]] virtual std::string DumpRow(
        std::uint32_t table, std::uint32_t row) = 0;
};

class IGameDataShopGoodsRefreshRuntime {
public:
    virtual ~IGameDataShopGoodsRefreshRuntime() = default;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Refresh(
        std::uint32_t table) = 0;
};

class IGameDataShopGoodsBindingRefreshRuntime
    : public IGameDataFieldSchemaRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void LogBind(std::string_view prefix,
                         std::uint32_t name) = 0;
    virtual void AssignEffects(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void BindEffects(std::uint32_t binding) = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseLogBuffer() = 0;
};

class GameDataShopGoodsTable final {
public:
    // IDA 0x4effd0: bind, parse, validate and insert or replace a Good row.
    [[nodiscard]] static std::uint8_t AddOrReplace(
        std::uint32_t table, std::uint32_t row,
        IGameDataShopGoodsRowRuntime& runtime);
    // IDA 0x4f0270: convert bound fields into an appended Good record.
    [[nodiscard]] static std::uint32_t Append(
        std::uint32_t table, std::uint32_t row,
        IGameDataShopGoodsAppendRuntime& runtime);
    // IDA 0x4f03e0: mark the table dirty and run its refresh path.
    [[nodiscard]] static std::uint32_t MarkAndRefresh(
        std::uint32_t table, IGameDataShopGoodsRefreshRuntime& runtime);
    // IDA 0x4f1610: refresh Good field bindings on count mismatch.
    [[nodiscard]] static std::uint32_t RefreshBindings(
        std::uint32_t table,
        IGameDataShopGoodsBindingRefreshRuntime& runtime);
};

}  // namespace pal4
