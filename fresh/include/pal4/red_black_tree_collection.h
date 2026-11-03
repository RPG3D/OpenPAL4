#pragma once

#include <cstdint>

namespace pal4 {

class IRedBlackTreeCollectionRuntime {
public:
    virtual ~IRedBlackTreeCollectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNil() = 0;
    virtual void SetSharedNil(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNilUsers() = 0;
    virtual void SetSharedNilUsers(std::uint32_t users) = 0;
    virtual void FreeNodes(std::uint32_t tree, std::uint32_t root) = 0;
    virtual void NextNode(std::uint32_t& node) = 0;
    virtual void DeleteNode(std::uint32_t tree, std::uint32_t node) = 0;
    virtual void ConstructTree(std::uint32_t tree) = 0;
    [[nodiscard]] virtual std::uint32_t FreeMemory(std::uint32_t address) = 0;
};

class IRedBlackTreeConstructorRuntime
    : public IRedBlackTreeCollectionRuntime {
public:
    [[nodiscard]] virtual std::uint32_t AllocateMemory(
        std::uint32_t bytes) = 0;
};

class RedBlackTreeCollection final {
public:
    explicit RedBlackTreeCollection(std::uint32_t address) : address_(address) {}

    // IDA 0x5c3de0: releases the tree header and its shared nil reference.
    [[nodiscard]] std::uint32_t Destroy(
        IRedBlackTreeCollectionRuntime& runtime) const;
    // IDA 0x5c42e0: copy three one-byte arguments before native tree setup.
    [[nodiscard]] std::uint32_t ConstructWithArgs(
        std::uint32_t first_source, std::uint8_t third_value,
        std::uint32_t second_source,
        IRedBlackTreeCollectionRuntime& runtime) const;
    // IDA 0x5c4310: erase the iterator range [first, last).
    [[nodiscard]] std::uint32_t& DeleteRange(
        std::uint32_t& output, std::uint32_t first,
        std::uint32_t last, IRedBlackTreeCollectionRuntime& runtime) const;
    // IDA 0x5cea80: same range algorithm with the dialog tree's shared nil.
    [[nodiscard]] std::uint32_t& ClearRange(
        std::uint32_t& output, std::uint32_t first,
        std::uint32_t last, IRedBlackTreeCollectionRuntime& runtime) const;
    // IDA 0x5c43e0: returns the leftmost node or the input slot if empty.
    [[nodiscard]] static std::uint32_t LeftmostNode(
        std::uint32_t root_slot, IRedBlackTreeCollectionRuntime& runtime);
    // IDA 0x5ceb50: advance a dialog tree iterator in node order.
    [[nodiscard]] static std::uint32_t AdvanceIterator(
        std::uint32_t& node, IRedBlackTreeCollectionRuntime& runtime);
    // IDA 0x5cf0f0: free each right subtree before walking the left spine.
    [[nodiscard]] static std::uint32_t FreeSubtree(
        std::uint32_t root, IRedBlackTreeCollectionRuntime& runtime);
    // IDA 0x5cf130: create a header and retain the shared nil node.
    [[nodiscard]] std::uint32_t ConstructExtended(
        IRedBlackTreeConstructorRuntime& runtime) const;

private:
    std::uint32_t address_;
};

}  // namespace pal4
