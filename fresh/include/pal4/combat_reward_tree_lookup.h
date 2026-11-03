#pragma once

#include <cstdint>

namespace pal4 {

class ICombatRewardTreeLookupRuntime {
public:
    virtual ~ICombatRewardTreeLookupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Nil() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Predecessor(
        std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t InsertAndRebalance(
        std::uint32_t tree, std::uint32_t child,
        std::uint32_t parent, std::uint32_t key) = 0;
};

class CombatRewardTreeLookup final {
public:
    // IDA 0x573760. Writes node pointer and inserted byte to result.
    [[nodiscard]] static std::uint32_t FindOrInsert(
        std::uint32_t tree, std::uint32_t result,
        std::uint32_t key, ICombatRewardTreeLookupRuntime& runtime);
    // IDA 0x5739a0.
    [[nodiscard]] static std::uint32_t SetPair(
        std::uint32_t destination, std::uint32_t first,
        std::uint32_t second, ICombatRewardTreeLookupRuntime& runtime);
    // IDA 0x5739c0. Same signed lookup with branch-specific result order.
    [[nodiscard]] static std::uint32_t FindOrInsertAlternate(
        std::uint32_t tree, std::uint32_t result,
        std::uint32_t key, ICombatRewardTreeLookupRuntime& runtime);
};

class CombatRewardTreeIterator final {
public:
    // IDA 0x573f10. Updates the node pointer stored at iterator.
    [[nodiscard]] static std::uint32_t Predecessor(
        std::uint32_t iterator, ICombatRewardTreeLookupRuntime& runtime);
};

}  // namespace pal4
