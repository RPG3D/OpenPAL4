#pragma once

#include "pal4/ui_red_black_tree_remove.h"
#include "pal4/ui_red_black_tree_iterator.h"
#include "pal4/pal_game_object_tree.h"

#include <cstdint>

namespace pal4 {

struct PkScriptNodeTag final {
    std::uint32_t value{};
    std::uint8_t flag{};
};

class IPalScriptTreeClearRuntime : public IUiRedBlackTreeRemoveRuntime,
                                  public IUiRedBlackTreeIteratorRuntime {
public:
    using IUiRedBlackTreeRemoveRuntime::ReadWord;
    using IUiRedBlackTreeRemoveRuntime::WriteWord;
    using IUiRedBlackTreeRemoveRuntime::SharedNil;
    virtual void FirstElement(std::uint32_t tree) = 0;
    [[nodiscard]] virtual std::uint32_t TemporaryCursor() = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseMemory(
        std::uint32_t address) = 0;
};

class IPalScriptTreeInsertRuntime : public IPalGameObjectFactoryInsertRuntime {
};

class PalScriptTree final {
public:
    // IDA 0x672070 writes the first five bytes of a tagged value.
    [[nodiscard]] static PkScriptNodeTag& CopyPkNodeTag(
        PkScriptNodeTag& output, std::uint32_t value,
        std::uint8_t flag);
    // IDA 0x600ee0. The node colour is at +20, and the shared nil is
    // supplied by the caller's script-tree runtime.
    [[nodiscard]] static std::uint32_t RemoveNode(
        std::uint32_t tree, std::uint32_t output, std::uint32_t node,
        IUiRedBlackTreeRemoveRuntime& runtime);
    // IDA 0x671c90. Same 24-byte node algorithm with the PK tree's nil.
    [[nodiscard]] static std::uint32_t RemovePkNode(
        std::uint32_t tree, std::uint32_t output, std::uint32_t node,
        IUiRedBlackTreeRemoveRuntime& runtime);
    // IDA 0x6012c0.
    [[nodiscard]] static std::uint32_t NextNode(
        std::uint32_t cursor, IUiRedBlackTreeIteratorRuntime& runtime);
    // IDA 0x672780 uses the PK tree's shared nil.
    [[nodiscard]] static std::uint32_t NextPkNode(
        std::uint32_t cursor, IUiRedBlackTreeIteratorRuntime& runtime);
    // IDA 0x601310.
    [[nodiscard]] static std::uint32_t FreeNodesRecursive(
        std::uint32_t root, IUiRedBlackTreeRemoveRuntime& runtime);
    // IDA 0x672090 uses the PK tree's shared nil.
    [[nodiscard]] static std::uint32_t FreePkNodesRecursive(
        std::uint32_t root, IUiRedBlackTreeRemoveRuntime& runtime);
    // IDA 0x601350.
    [[nodiscard]] static std::uint32_t InsertNode(
        std::uint32_t tree, std::uint32_t output,
        std::uint32_t child, std::uint32_t parent,
        std::uint32_t value, IPalGameObjectFactoryInsertRuntime& runtime);
    // IDA 0x6720d0 copies two DWORDs (key and handle) into the PK node.
    [[nodiscard]] static std::uint32_t InsertPkNode(
        std::uint32_t tree, std::uint32_t output,
        std::uint32_t child, std::uint32_t parent,
        std::uint32_t value, IPalScriptTreeInsertRuntime& runtime);
    // IDA 0x6015c0.
    [[nodiscard]] static std::uint32_t LowerBound(
        std::uint32_t tree, std::uint32_t key,
        IUiRedBlackTreeIteratorRuntime& runtime);
    // IDA 0x672670 compares unsigned PK script keys.
    [[nodiscard]] static std::uint32_t LowerBoundPk(
        std::uint32_t tree, std::uint32_t key,
        IUiRedBlackTreeIteratorRuntime& runtime);
    // IDA 0x6015f0.
    [[nodiscard]] static std::uint32_t PreviousNode(
        std::uint32_t cursor, IUiRedBlackTreeIteratorRuntime& runtime);
    // IDA 0x6726a0 uses the PK tree nil and colour at +20.
    [[nodiscard]] static std::uint32_t PreviousPkNode(
        std::uint32_t cursor, IUiRedBlackTreeIteratorRuntime& runtime);
    // IDA 0x601830. Returns output with the first retained iterator.
    [[nodiscard]] static std::uint32_t ClearRange(
        std::uint32_t tree, std::uint32_t output,
        std::uint32_t first, std::uint32_t last,
        IUiRedBlackTreeRemoveRuntime& runtime);
    // IDA 0x601900.
    [[nodiscard]] static std::uint32_t LeftmostNode(
        std::uint32_t root, IUiRedBlackTreeRemoveRuntime& runtime);
    // IDA 0x601920.
    [[nodiscard]] static std::uint32_t ClearAll(
        std::uint32_t tree, std::uint32_t& instance_count,
        std::uint32_t& shared_nil,
        IPalScriptTreeClearRuntime& runtime);
};

}  // namespace pal4
