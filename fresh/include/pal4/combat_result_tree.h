#pragma once

#include <cstdint>

namespace pal4 {

class ICombatResultTreeRuntime {
public:
    virtual ~ICombatResultTreeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Nil() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void Free(std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t CreateNode(
        std::uint32_t parent, std::uint32_t value) = 0;
    virtual void CopyData(std::uint32_t destination,
                          std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
};

class CombatResultTree final {
public:
    // IDA CombatResultTree_Assign at 0x549460.
    [[nodiscard]] static std::uint32_t Assign(
        std::uint32_t tree, std::uint32_t source,
        ICombatResultTreeRuntime& runtime);
    // IDA CombatResultTree_EraseRange at 0x5495e0.
    [[nodiscard]] static std::uint32_t EraseRange(
        std::uint32_t tree, std::uint32_t result,
        std::uint32_t first, std::uint32_t last,
        ICombatResultTreeRuntime& runtime);
    // IDA CombatResultTree_Next at 0x5496b0.
    [[nodiscard]] static std::uint32_t Next(
        std::uint32_t iterator, ICombatResultTreeRuntime& runtime);
    // IDA CombatResultTree_CreateListNode at 0x549700.
    [[nodiscard]] static std::uint32_t CreateListNode(
        std::uint32_t previous, std::uint32_t next,
        ICombatResultTreeRuntime& runtime);
    // IDA CombatResultTree_EraseNode at 0x549730.
    [[nodiscard]] static std::uint32_t EraseNode(
        std::uint32_t tree, std::uint32_t result,
        std::uint32_t node, ICombatResultTreeRuntime& runtime);
    // IDA CombatResultTree_DeleteSubtree at 0x549b10.
    [[nodiscard]] static std::uint32_t DeleteSubtree(
        std::uint32_t root, ICombatResultTreeRuntime& runtime);
    // IDA CombatResultTree_FindMin at 0x549b50.
    [[nodiscard]] static std::uint32_t FindMin(
        std::uint32_t slot, ICombatResultTreeRuntime& runtime);
    // IDA CombatResultTree_Advance at 0x549b70.
    [[nodiscard]] static std::uint32_t Advance(
        std::uint32_t iterator, std::uint32_t previous,
        ICombatResultTreeRuntime& runtime);
    // IDA CombatResultTree_CopyTree at 0x549bd0.
    [[nodiscard]] static std::uint32_t CopyTree(
        std::uint32_t root, std::uint32_t parent,
        ICombatResultTreeRuntime& runtime);
};

}  // namespace pal4
