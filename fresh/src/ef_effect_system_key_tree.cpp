#include "pal4/ef_effect_system_key_tree.h"
#include "pal4/monster_manager_tree_cursor.h"

namespace pal4 {
namespace {

template <class Reader>
std::uint32_t Bound(std::uint32_t tree, std::uint32_t key,
                    std::uint32_t nil, bool upper, Reader&& read) {
    const auto head = read(tree + 4u);
    auto result = head;
    auto node = read(head + 4u);
    while (node != nil) {
        const auto value = read(node + 12u);
        if (upper ? key < value : key <= value) {
            result = node;
            node = read(node);
        } else {
            node = read(node + 8u);
        }
    }
    return result;
}

}  // namespace

std::int32_t IEfEffectSystemKeyInsertRuntime::CompareRange(
    std::uint32_t input, std::uint32_t node_data) {
    return CompareText(input, node_data);
}

std::int32_t IEfEffectSystemKeyInsertRuntime::CompareText(
    std::uint32_t first, std::uint32_t second) {
    const auto left = ReadDword(first);
    const auto right = ReadDword(second);
    return left < right ? -1 : (left > right ? 1 : 0);
}

void IEfEffectSystemKeyInsertRuntime::AssignText(
    std::uint32_t destination, std::uint32_t source) {
    WriteDword(destination, ReadDword(source));
}

void IEfEffectSystemKeyInsertRuntime::SetNodeData(
    MonsterManagerTreeInsertResult& result,
    std::uint32_t candidate) {
    result.node = candidate;
}

bool EfEffectSystemKeyTree::InsertKey(
    std::uint32_t object, std::uint32_t key,
    IEfEffectSystemKeyInsertRuntime& runtime) {
    if (runtime.ReadByte(object) == 0u) return false;
    const auto tree = object + kTreeOffset;
    const auto nil = runtime.SharedNil();
    const auto head = runtime.ReadDword(tree + 4u);
    const auto candidate = Bound(tree, key, nil, false,
                                 [&](std::uint32_t address) {
                                     return runtime.ReadDword(address);
                                 });
    if (candidate != head && runtime.ReadDword(candidate + 12u) == key)
        return false;
    const auto input = runtime.PrepareKey(key);
    return MonsterManagerTreeInsert::Insert(tree, input, runtime).inserted;
}

void EfEffectSystemKeyTree::RemoveKey(
    std::uint32_t object, std::uint32_t key, std::uint32_t nil,
    IEfEffectSystemKeyEraseRuntime& runtime) {
    if (runtime.ReadByte(object) == 0u) return;
    const auto tree = object + kTreeOffset;
    const auto head = runtime.ReadWord(tree + 4u);
    const auto read = [&](std::uint32_t address) {
        return runtime.ReadWord(address);
    };
    const auto first = Bound(tree, key, nil, false, read);
    if (first == head || runtime.ReadWord(first + 12u) != key) return;
    const auto last = Bound(tree, key, nil, true, read);
    ArenaSequenceTree::EraseRange(
        tree, runtime.ScratchResultAddress(), first, last,
        nil, runtime, kColorOffset, false);
}

bool EfEffectSystemKeyTree::ContainsKey(
    std::uint32_t object, std::uint32_t key,
    std::uint32_t nil, IArenaSequenceRuntime& runtime) {
    if (runtime.ReadByte(object) == 0u) return false;
    const auto tree = object + kTreeOffset;
    const auto head = runtime.ReadWord(tree + 4u);
    const auto candidate = Bound(tree, key, nil, false,
                                 [&](std::uint32_t address) {
                                     return runtime.ReadWord(address);
                                 });
    return candidate != head && runtime.ReadWord(candidate + 12u) == key;
}

std::uint32_t EfEffectSystemKeyTree::EraseNode(
    std::uint32_t tree, std::uint32_t result,
    std::uint32_t node, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    ArenaSequenceTree::EraseNode(tree, result, node, nil, runtime,
                                 kColorOffset, false);
    return result;
}

std::uint32_t EfEffectSystemKeyTree::DeleteSubtree(
    std::uint32_t root, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    return ArenaSequenceTree::DeleteSubtree(root, nil, runtime, false);
}

std::uint32_t EfEffectSystemKeyTree::InsertAt(
    std::uint32_t tree, std::uint32_t result,
    std::uint32_t child, std::uint32_t parent,
    std::uint32_t input, IEfEffectSystemKeyInsertRuntime& runtime) {
    const auto node = MonsterManagerTreeInsert::InsertHelper(
        tree, child, parent, input, runtime);
    runtime.WriteDword(result, node);
    return result;
}

std::uint32_t EfEffectSystemKeyTree::LowerBound(
    std::uint32_t tree, std::uint32_t key, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    return Bound(tree, key, nil, false,
                 [&](std::uint32_t address) {
                     return runtime.ReadWord(address);
                 });
}

std::uint32_t EfEffectSystemKeyTree::UpperBound(
    std::uint32_t tree, std::uint32_t key, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    return Bound(tree, key, nil, true,
                 [&](std::uint32_t address) {
                     return runtime.ReadWord(address);
                 });
}

std::uint32_t EfEffectSystemKeyTree::Previous(
    std::uint32_t& iterator, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    const auto step = MonsterManagerTreeCursor::Predecessor(
        iterator, nil,
        [&](std::uint32_t address) {
            return runtime.ReadWord(address);
        }, kColorOffset);
    iterator = step.next;
    return step.result;
}

}  // namespace pal4
