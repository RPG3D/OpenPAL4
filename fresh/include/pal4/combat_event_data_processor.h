#pragma once

#include <cstdint>

namespace pal4 {

class ICombatEventDataRuntime {
public:
    virtual ~ICombatEventDataRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    [[nodiscard]] virtual std::uint32_t InitializeMatches(
        std::uint8_t key_low_byte) = 0;
    [[nodiscard]] virtual std::uint32_t UpgradeRecordCount(
        std::uint32_t count_holder) = 0;
    virtual void InsertMatch(std::uint32_t collection,
                             std::uint32_t entry) = 0;
    [[nodiscard]] virtual std::uint32_t MatchCount(
        std::uint32_t collection) = 0;
    [[nodiscard]] virtual std::uint32_t FirstMatch(
        std::uint32_t collection) = 0;
    [[nodiscard]] virtual std::int32_t LowerBound(
        std::uint32_t table, std::uint32_t first_key,
        std::uint32_t second_key) = 0;
    [[nodiscard]] virtual std::int32_t UpperBound(
        std::uint32_t table, std::uint32_t first_key,
        std::uint32_t second_key) = 0;
    virtual void SetValidatedProperty(std::uint32_t actor,
                                      std::uint32_t property,
                                      std::int32_t value) = 0;
    virtual void CopyBytes(std::uint32_t destination,
                           std::uint32_t source,
                           std::uint32_t count) = 0;
    virtual void CleanupMatchRange(std::uint32_t collection) = 0;
    [[nodiscard]] virtual int FreeMatches(std::uint32_t collection) = 0;
};

class CombatEventDataProcessor final {
public:
    // IDA 0x57db70.
    [[nodiscard]] static int Process(
        std::uint32_t actor, std::uint32_t first_key,
        std::uint32_t second_key, ICombatEventDataRuntime& runtime);
    // IDA 0x57dce0.
    [[nodiscard]] static bool Matches(
        std::uint32_t entry, std::uint32_t first_key,
        std::uint32_t second_key, ICombatEventDataRuntime& runtime);
};

}  // namespace pal4
