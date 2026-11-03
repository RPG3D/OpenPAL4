#pragma once

#include <cstdint>

namespace pal4 {

class ICombatRewardTreeInsertionRuntime {
public:
    virtual ~ICombatRewardTreeInsertionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Nil() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
};

class CombatRewardTreeInsertion final {
public:
    // IDA 0x573be0. Writes the inserted node pointer to result.
    [[nodiscard]] static std::uint32_t InsertAndRebalance(
        std::uint32_t tree, std::uint32_t result,
        std::uint32_t child, std::uint32_t parent,
        std::uint32_t key, ICombatRewardTreeInsertionRuntime& runtime);
    // IDA 0x573e50/0x573eb0.
    [[nodiscard]] static std::uint32_t RotateLeft(
        std::uint32_t tree, std::uint32_t node,
        ICombatRewardTreeInsertionRuntime& runtime);
    [[nodiscard]] static std::uint32_t RotateRight(
        std::uint32_t tree, std::uint32_t node,
        ICombatRewardTreeInsertionRuntime& runtime);
};

}  // namespace pal4
