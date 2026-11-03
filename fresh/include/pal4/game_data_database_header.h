#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string_view>

#include "pal4/game_data_ai_record_sorter.h"

namespace pal4 {

class IGameDataCombatFormulaRuntime {
public:
    virtual ~IGameDataCombatFormulaRuntime() = default;
    [[nodiscard]] virtual bool OpenTable(std::string_view name) = 0;
    [[nodiscard]] virtual std::span<const GameDataAiKeyEntry> Keys() = 0;
    [[nodiscard]] virtual bool HasRow(std::uint32_t row_index) = 0;
    [[nodiscard]] virtual float ReadFloatProperty(
        std::uint32_t row_index, std::uint32_t property_index) = 0;
    virtual void LogError(std::string_view message) = 0;
    [[nodiscard]] virtual int LogRangeWarning(std::uint32_t source_line,
                                               std::string_view detail) = 0;
};

class IGameDataDatabaseFactoryRuntime {
public:
    virtual ~IGameDataDatabaseFactoryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t RegisterFactory(
        std::uint32_t registry, std::string_view name,
        std::uint32_t constructor_address) = 0;
};

class IGameDataDatabaseInitializationRuntime
    : public IGameDataCombatFormulaRuntime {
public:
    [[nodiscard]] virtual std::uint32_t FindFactory(
        std::uint32_t registry, std::string_view name) = 0;
};

class GameDataDatabaseHeader final {
public:
    using Words = std::array<std::uint32_t, 35>;

    // IDA 0x4cfa30: zero the first 128 bytes and the final member at +136.
    [[nodiscard]] GameDataDatabaseHeader& Reset();
    // IDA 0x4cfa50: load 32 keyed formula values and check two ranges.
    [[nodiscard]] int LoadCombatFormulas(IGameDataCombatFormulaRuntime& runtime);
    // IDA 0x4cfc60: signed bounds check over the first 32 float slots.
    [[nodiscard]] double FloatAt(std::int32_t index) const;
    // IDA 0x4cfc80: store and return the registry pointer at +136.
    [[nodiscard]] std::uint32_t SetFactoryRegistry(std::uint32_t registry);
    // IDA 0x4cfc90: register two named constructors in order.
    [[nodiscard]] std::uint8_t RegisterFactories(
        IGameDataDatabaseFactoryRuntime& runtime) const;
    // IDA 0x4cfcd0: load formulas then resolve two required tables.
    [[nodiscard]] std::uint8_t InitializeFromDatabase(
        IGameDataDatabaseInitializationRuntime& runtime);
    [[nodiscard]] Words& Data() { return words_; }
    [[nodiscard]] const Words& Data() const { return words_; }

private:
    Words words_{};
};

}  // namespace pal4
