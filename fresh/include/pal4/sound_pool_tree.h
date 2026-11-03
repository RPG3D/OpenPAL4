#pragma once

#include <cstdint>

namespace pal4 {

struct SoundPoolInsertResult final {
    std::uint32_t node = 0u;
    bool inserted = false;
};

class ISoundPoolTreeRuntime {
public:
    virtual ~ISoundPoolTreeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Sentinel() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address,
                                                 std::uint32_t offset) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t offset,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address,
                                                 std::uint32_t offset) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint32_t offset,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t CreateNode(std::uint32_t parent) = 0;
};

class ISoundPoolTreeEraseRuntime : public ISoundPoolTreeRuntime {
public:
    virtual void CleanupEntry(std::uint32_t value_address) = 0;
    virtual void FreeNode(std::uint32_t node) = 0;
};

class SoundPoolTree final {
public:
    // IDA 0x65aa50: insert a CRC-keyed value, with an inline RB-tree path.
    [[nodiscard]] static SoundPoolInsertResult Insert(
        std::uint32_t pool, std::uint32_t entry,
        ISoundPoolTreeRuntime& runtime);
    // IDA 0x65b1c0: allocate, link and rebalance a resolved unique insertion.
    [[nodiscard]] static std::uint32_t InsertNodeAndRebalance(
        std::uint32_t pool, std::uint32_t child, std::uint32_t parent,
        std::uint32_t entry, ISoundPoolTreeRuntime& runtime);
    // IDA 0x65ac70: erase node, rebalance and return its successor iterator.
    [[nodiscard]] static std::uint32_t Erase(
        std::uint32_t pool, std::uint32_t node,
        ISoundPoolTreeEraseRuntime& runtime);
    // IDA 0x65b070: return an exact CRC match or the tree head.
    [[nodiscard]] static std::uint32_t FindExact(
        std::uint32_t pool, std::uint32_t key,
        ISoundPoolTreeRuntime& runtime);
    // IDA 0x65b0e0: advance a node iterator to its in-order successor.
    [[nodiscard]] static std::uint32_t Next(
        std::uint32_t node, ISoundPoolTreeRuntime& runtime);
    // IDA 0x65b130: return first node whose CRC is at least the key.
    [[nodiscard]] static std::uint32_t LowerBound(
        std::uint32_t pool, std::uint32_t key,
        ISoundPoolTreeRuntime& runtime);
    // IDA 0x65b170: destroy each right subtree before walking left spine.
    [[nodiscard]] static std::uint32_t DestroySubtree(
        std::uint32_t node, ISoundPoolTreeEraseRuntime& runtime);
    // IDA 0x65b550: move entry ownership byte while copying CRC and file.
    [[nodiscard]] static std::uint32_t CopyEntry(
        std::uint32_t destination, std::uint32_t source,
        ISoundPoolTreeRuntime& runtime);
    // IDA 0x65b430/0x65b490: rotate a subtree around its root node.
    [[nodiscard]] static std::uint32_t RotateLeft(
        std::uint32_t pool, std::uint32_t node,
        ISoundPoolTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t RotateRight(
        std::uint32_t pool, std::uint32_t node,
        ISoundPoolTreeRuntime& runtime);
    // IDA 0x65b4f0: move an iterator to its in-order predecessor.
    [[nodiscard]] static std::uint32_t Previous(
        std::uint32_t node, ISoundPoolTreeRuntime& runtime);
};

}  // namespace pal4
