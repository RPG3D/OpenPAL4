#pragma once

#include "pal4/arena_sequence_resources.h"
#include "pal4/monster_manager_tree_insert.h"

#include <cstdint>

namespace pal4 {

class IEfEffectSystemKeyInsertRuntime
    : public IMonsterManagerTreeInsertRuntime {
public:
    ~IEfEffectSystemKeyInsertRuntime() override = default;
    [[nodiscard]] std::uint32_t NodeSize() const final { return 20u; }
    [[nodiscard]] std::uint32_t ColorOffset() const final { return 16u; }
    [[nodiscard]] std::int32_t CompareRange(
        std::uint32_t input, std::uint32_t node_data) final;
    [[nodiscard]] std::int32_t CompareText(
        std::uint32_t first, std::uint32_t second) final;
    void AssignText(std::uint32_t destination,
                    std::uint32_t source) final;
    void SetNodeData(MonsterManagerTreeInsertResult& result,
                     std::uint32_t candidate) final;
    // Models the original stack-local key passed to the insertion helper.
    [[nodiscard]] virtual std::uint32_t PrepareKey(
        std::uint32_t key) = 0;
};

class IEfEffectSystemKeyEraseRuntime : public IArenaSequenceRuntime {
public:
    ~IEfEffectSystemKeyEraseRuntime() override = default;
    [[nodiscard]] virtual std::uint32_t ScratchResultAddress() = 0;
};

class EfEffectSystemKeyTree final {
public:
    static constexpr std::uint32_t kTreeOffset = 148u;
    static constexpr std::uint32_t kNodeSize = 20u;
    static constexpr std::uint32_t kColorOffset = 16u;

    // IDA 0x6362a0: insert an absent key while the singleton is enabled.
    [[nodiscard]] static bool InsertKey(
        std::uint32_t object, std::uint32_t key,
        IEfEffectSystemKeyInsertRuntime& runtime);
    // IDA 0x6363b0: erase the complete equal-key range.
    static void RemoveKey(
        std::uint32_t object, std::uint32_t key,
        std::uint32_t nil, IEfEffectSystemKeyEraseRuntime& runtime);
    // IDA 0x6364d0: exact unsigned-key membership test.
    [[nodiscard]] static bool ContainsKey(
        std::uint32_t object, std::uint32_t key,
        std::uint32_t nil, IArenaSequenceRuntime& runtime);
    // IDA 0x6377f0: unlink one 20-byte key node and repair red-black links.
    [[nodiscard]] static std::uint32_t EraseNode(
        std::uint32_t tree, std::uint32_t result,
        std::uint32_t node, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    // IDA 0x637bd0: release a key-tree branch right-first.
    [[nodiscard]] static std::uint32_t DeleteSubtree(
        std::uint32_t root, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    // IDA 0x637c90: allocate and insert at a positioned parent/child link.
    [[nodiscard]] static std::uint32_t InsertAt(
        std::uint32_t tree, std::uint32_t result,
        std::uint32_t child, std::uint32_t parent,
        std::uint32_t input, IEfEffectSystemKeyInsertRuntime& runtime);
    // IDA 0x6389c0/0x638a90: unsigned lower and upper key bounds.
    [[nodiscard]] static std::uint32_t LowerBound(
        std::uint32_t tree, std::uint32_t key, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    [[nodiscard]] static std::uint32_t UpperBound(
        std::uint32_t tree, std::uint32_t key, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    // IDA 0x6389f0: move a key cursor in reverse order.
    [[nodiscard]] static std::uint32_t Previous(
        std::uint32_t& iterator, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
};

}  // namespace pal4
