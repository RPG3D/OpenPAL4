#include "pal4/effect_name_count_tree.h"

#include "pal4/effect_object_pool.h"
#include "pal4/arena_sequence_resources.h"
#include "pal4/monster_manager_tree_cursor.h"
#include "pal4/monster_manager_tree_rotation.h"
#include "pal4/ui_text_buffer_record.h"

namespace pal4 {

std::uint32_t EffectNameCountTree::EraseRange(
    std::uint32_t tree, std::uint32_t result,
    std::uint32_t first, std::uint32_t last, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    if (runtime.ReadWord(tree + 12u) != 0u &&
        first == runtime.ReadWord(head) && last == head) {
        auto node = runtime.ReadWord(head + 4u);
        while (node != nil) {
            (void)DeleteSubtree(runtime.ReadWord(node + 8u), nil, runtime);
            const auto left = runtime.ReadWord(node);
            ArenaSequenceResources::ReleaseText(node + 12u, runtime);
            (void)runtime.Free(node);
            node = left;
        }
        runtime.WriteWord(head + 4u, nil);
        runtime.WriteWord(tree + 12u, 0u);
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
        runtime.WriteWord(result, runtime.ReadWord(head));
        return result;
    }

    auto node = first;
    while (node != last) {
        const auto current = node;
        node = MonsterManagerTreeCursor::Successor(
            node, nil, [&](std::uint32_t address) {
                return runtime.ReadWord(address);
            }).next;
        RemoveNode(tree, result, current, nil, runtime);
    }
    runtime.WriteWord(result, node);
    return result;
}

std::uint32_t EffectNameCountTree::MinimumFromLink(
    std::uint32_t link, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    auto minimum = link;
    for (auto node = runtime.ReadWord(link); node != nil;
         node = runtime.ReadWord(node))
        minimum = node;
    return minimum;
}

void EffectNameCountRecord::CopyTextAndCount(
    std::uint32_t destination, std::uint32_t source,
    IUiTextBufferRecordRuntime& runtime) {
    UiTextBufferRecord::InitializeAndCopy(destination, source, runtime);
}

namespace {

class NameCountInsertAdapter final : public IMonsterManagerTreeInsertRuntime {
public:
    explicit NameCountInsertAdapter(IEffectObjectPoolNameCountRuntime& runtime)
        : runtime_(runtime) {}

    [[nodiscard]] std::uint32_t SharedNil() const override {
        return runtime_.SharedNameNil();
    }
    [[nodiscard]] std::uint32_t ReadDword(
        std::uint32_t address) const override {
        return runtime_.ReadWord(address);
    }
    [[nodiscard]] std::uint8_t ReadByte(
        std::uint32_t address) const override {
        return runtime_.ReadByte(address);
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        runtime_.WriteWord(address, value);
    }
    [[nodiscard]] std::int32_t CompareRange(
        std::uint32_t input, std::uint32_t node_text) override {
        return runtime_.CompareText(input, node_text);
    }
    [[nodiscard]] std::int32_t CompareText(
        std::uint32_t first, std::uint32_t second) override {
        return runtime_.CompareText(first, second);
    }
    [[nodiscard]] std::uint32_t AllocateMemory(
        std::uint32_t bytes) override {
        return runtime_.AllocateNameCountNode(bytes);
    }
    void AssignText(std::uint32_t destination,
                    std::uint32_t source) override {
        runtime_.AssignText(destination, source);
        runtime_.WriteWord(destination + 16u,
                           runtime_.ReadWord(source + 16u));
    }
    void SetNodeData(MonsterManagerTreeInsertResult& result,
                     std::uint32_t candidate) override {
        result.node = candidate;
    }

private:
    IEffectObjectPoolNameCountRuntime& runtime_;
};

}  // namespace

MonsterManagerTreeInsertResult EffectNameCountTree::FindOrCreate(
    std::uint32_t tree, std::uint32_t record,
    IEffectObjectPoolNameCountRuntime& runtime) {
    NameCountInsertAdapter adapter(runtime);
    return MonsterManagerTreeInsert::Insert(tree, record, adapter);
}

void EffectNameCountTree::RemoveNode(
    std::uint32_t tree, std::uint32_t result, std::uint32_t node,
    std::uint32_t nil, IArenaSequenceRuntime& runtime) {
    ArenaSequenceTree::EraseNode(tree, result, node, nil, runtime, 32u);
}

std::uint32_t EffectNameCountTree::DeleteSubtree(
    std::uint32_t root, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    return ArenaSequenceTree::DeleteSubtree(root, nil, runtime);
}

std::uint32_t EffectNameCountTree::InsertNode(
    std::uint32_t tree, std::uint32_t child,
    std::uint32_t parent, std::uint32_t record,
    IEffectObjectPoolNameCountRuntime& runtime) {
    NameCountInsertAdapter adapter(runtime);
    return MonsterManagerTreeInsert::InsertHelper(
        tree, child, parent, record, adapter);
}

std::uint32_t EffectNameCountTree::PromoteRightChild(
    std::uint32_t tree, std::uint32_t node,
    IEffectObjectPoolNameCountRuntime& runtime) {
    NameCountInsertAdapter adapter(runtime);
    return MonsterManagerTreeRotation::RotateRight(tree, node, adapter);
}

std::uint32_t EffectNameCountTree::PromoteLeftChild(
    std::uint32_t tree, std::uint32_t node,
    IEffectObjectPoolNameCountRuntime& runtime) {
    NameCountInsertAdapter adapter(runtime);
    return MonsterManagerTreeRotation::RotateLeft(tree, node, adapter);
}

std::uint32_t EffectNameCountTree::Previous(
    std::uint32_t& iterator,
    IEffectObjectPoolNameCountRuntime& runtime) {
    const auto step = MonsterManagerTreeCursor::Predecessor(
        iterator, runtime.SharedNameNil(),
        [&](std::uint32_t address) {
            return runtime.ReadWord(address);
        });
    iterator = step.next;
    return step.result;
}

std::uint32_t EffectNameCountTree::Advance(
    std::uint32_t& iterator,
    IEffectObjectPoolNameCountRuntime& runtime) {
    const auto step = MonsterManagerTreeCursor::Successor(
        iterator, runtime.SharedNameNil(),
        [&](std::uint32_t address) {
            return runtime.ReadWord(address);
        });
    iterator = step.next;
    return step.result;
}

std::uint32_t EffectNameCountTree::LowerBound(
    std::uint32_t tree, std::uint32_t key,
    IEffectObjectPoolNameCountRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    auto candidate = head;
    auto node = runtime.ReadWord(head + 4u);
    while (node != runtime.SharedNameNil()) {
        if (runtime.CompareText(node + 12u, key) >= 0) {
            candidate = node;
            node = runtime.ReadWord(node);
        } else {
            node = runtime.ReadWord(node + 8u);
        }
    }
    return candidate;
}

}  // namespace pal4
