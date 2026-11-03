#pragma once

#include "pal4/ui_red_black_tree_insert.h"
#include "pal4/ui_red_black_tree_iterator.h"
#include "pal4/ui_red_black_tree_lookup.h"
#include "pal4/ui_red_black_tree_remove.h"
#include "pal4/ui_red_black_tree_storage.h"
#include "pal4/ui_red_black_tree_rotation.h"

#include <cstdint>

namespace pal4 {

class IPalGameObjectNameInsertRuntime : public IUiRedBlackTreeInsertRuntime {
public:
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
};

class IPalGameObjectFactoryClearRuntime : public IUiRedBlackTreeLinkRuntime {
public:
    virtual void FreeMemory(std::uint32_t address) = 0;
};

class IPalGameObjectFactoryInsertRuntime : public IUiRedBlackTreeLinkRuntime {
public:
    [[nodiscard]] virtual std::uint32_t AllocateMemory(
        std::uint32_t size) = 0;
};

// IDA 0x5a7d80 onward: two address-backed 32-bit STL tree layouts.
class PalGameObjectTree final {
public:
    [[nodiscard]] static std::uint32_t InsertName(
        std::uint32_t tree, std::uint32_t output, std::uint32_t key,
        IPalGameObjectNameInsertRuntime& runtime);
    [[nodiscard]] static std::uint32_t RemoveName(
        std::uint32_t tree, std::uint32_t output, std::uint32_t node,
        IUiRedBlackTreeRemoveRuntime& runtime);
    [[nodiscard]] static std::uint32_t NextName(
        std::uint32_t cursor, IUiRedBlackTreeIteratorRuntime& runtime);
    [[nodiscard]] static std::uint32_t LowerBoundName(
        std::uint32_t tree, std::uint32_t output,
        std::uint32_t query, IUiRedBlackTreeLookupRuntime& runtime);
    [[nodiscard]] static std::uint32_t ClearNameNodes(
        std::uint32_t node, IUiRedBlackTreeStorageRuntime& runtime);
    [[nodiscard]] static std::uint32_t InsertNameNode(
        std::uint32_t tree, std::uint32_t output,
        std::uint32_t child, std::uint32_t parent,
        std::uint32_t key, IUiRedBlackTreeInsertRuntime& runtime);
    [[nodiscard]] static std::uint32_t RotateNameLeft(
        std::uint32_t tree, std::uint32_t node,
        IUiRedBlackTreeLinkRuntime& runtime);
    [[nodiscard]] static std::uint32_t LeftmostName(
        std::uint32_t slot, IUiRedBlackTreeLookupRuntime& runtime);
    [[nodiscard]] static std::uint32_t RotateNameRight(
        std::uint32_t tree, std::uint32_t node,
        IUiRedBlackTreeLinkRuntime& runtime);
    [[nodiscard]] static std::uint32_t AdvanceName(
        std::uint32_t cursor, std::uint32_t output,
        IUiRedBlackTreeIteratorRuntime& runtime);
    [[nodiscard]] static std::uint32_t FindFactory(
        std::uint32_t tree, std::uint32_t output, std::uint32_t key,
        IUiRedBlackTreeLinkRuntime& runtime);
    [[nodiscard]] static std::uint32_t PreviousName(
        std::uint32_t cursor, IUiRedBlackTreeIteratorRuntime& runtime);
    [[nodiscard]] static std::uint32_t RemoveFactory(
        std::uint32_t tree, std::uint32_t output, std::uint32_t node,
        IUiRedBlackTreeRemoveRuntime& runtime);
    [[nodiscard]] static std::uint32_t ClearFactoryNodes(
        std::uint32_t node, IPalGameObjectFactoryClearRuntime& runtime);
    [[nodiscard]] static std::uint32_t AdvanceFactory(
        std::uint32_t cursor, std::uint32_t output,
        IUiRedBlackTreeIteratorRuntime& runtime);
    [[nodiscard]] static std::uint32_t NextFactory(
        std::uint32_t cursor, IUiRedBlackTreeIteratorRuntime& runtime);
    [[nodiscard]] static std::uint32_t InsertFactoryNode(
        std::uint32_t tree, std::uint32_t output,
        std::uint32_t child, std::uint32_t parent,
        std::uint32_t value, IPalGameObjectFactoryInsertRuntime& runtime);
    [[nodiscard]] static std::uint32_t PreviousFactory(
        std::uint32_t cursor, IUiRedBlackTreeIteratorRuntime& runtime);
};

}  // namespace pal4
