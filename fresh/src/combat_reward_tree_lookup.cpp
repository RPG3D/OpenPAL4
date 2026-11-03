#include "pal4/combat_reward_tree_lookup.h"

namespace pal4 {

std::uint32_t CombatRewardTreeLookup::FindOrInsert(
    std::uint32_t tree, std::uint32_t result,
    std::uint32_t key, ICombatRewardTreeLookupRuntime& runtime) {
    const auto nil = runtime.Nil();
    const auto head = runtime.ReadWord(tree + 4u);
    auto node = runtime.ReadWord(head + 4u);
    auto parent = head;
    bool left = true;
    const auto value = static_cast<std::int32_t>(runtime.ReadWord(key));
    while (node != nil) {
        parent = node;
        left = value < static_cast<std::int32_t>(
            runtime.ReadWord(node + 12u));
        node = runtime.ReadWord(node + (left ? 0u : 8u));
    }

    auto candidate = parent;
    bool insert = runtime.ReadByte(tree + 8u) != 0u;
    if (!insert && left) {
        if (parent == runtime.ReadWord(head)) {
            insert = true;
        } else {
            candidate = runtime.Predecessor(parent);
        }
    }
    if (!insert)
        insert = static_cast<std::int32_t>(
            runtime.ReadWord(candidate + 12u)) < value;
    if (insert)
        candidate = runtime.InsertAndRebalance(tree, node, parent, key);
    runtime.WriteWord(result, candidate);
    runtime.WriteByte(result + 4u, insert ? 1u : 0u);
    return result;
}

std::uint32_t CombatRewardTreeLookup::SetPair(
    std::uint32_t destination, std::uint32_t first,
    std::uint32_t second, ICombatRewardTreeLookupRuntime& runtime) {
    runtime.WriteWord(destination, runtime.ReadWord(first));
    runtime.WriteWord(destination + 4u, runtime.ReadWord(second));
    return destination;
}

std::uint32_t CombatRewardTreeLookup::FindOrInsertAlternate(
    std::uint32_t tree, std::uint32_t result,
    std::uint32_t key, ICombatRewardTreeLookupRuntime& runtime) {
    const auto nil = runtime.Nil();
    const auto head = runtime.ReadWord(tree + 4u);
    auto node = runtime.ReadWord(head + 4u);
    auto parent = head;
    bool left = true;
    const auto value = static_cast<std::int32_t>(runtime.ReadWord(key));
    while (node != nil) {
        parent = node;
        left = value < static_cast<std::int32_t>(
            runtime.ReadWord(node + 12u));
        node = runtime.ReadWord(node + (left ? 0u : 8u));
    }
    if (runtime.ReadByte(tree + 8u) != 0u) {
        const auto inserted = runtime.InsertAndRebalance(
            tree, node, parent, key);
        runtime.WriteWord(result, inserted);
        runtime.WriteByte(result + 4u, 1u);
        return result;
    }

    auto candidate = parent;
    bool insert = false;
    if (left) {
        if (parent == runtime.ReadWord(head))
            insert = true;
        else
            candidate = runtime.Predecessor(parent);
    }
    if (!insert)
        insert = static_cast<std::int32_t>(
            runtime.ReadWord(candidate + 12u)) < value;
    if (insert) {
        candidate = runtime.InsertAndRebalance(tree, node, parent, key);
        runtime.WriteByte(result + 4u, 1u);
        runtime.WriteWord(result, candidate);
    } else {
        runtime.WriteByte(result + 4u, 0u);
        runtime.WriteWord(result, candidate);
    }
    return result;
}

std::uint32_t CombatRewardTreeIterator::Predecessor(
    std::uint32_t iterator, ICombatRewardTreeLookupRuntime& runtime) {
    auto node = runtime.ReadWord(iterator);
    const auto parent = runtime.ReadWord(node + 4u);
    if (runtime.ReadWord(node + 20u) == 0u &&
        runtime.ReadWord(parent + 4u) == node) {
        const auto right = runtime.ReadWord(node + 8u);
        runtime.WriteWord(iterator, right);
        return right;
    }

    auto left = runtime.ReadWord(node);
    if (left != runtime.Nil()) {
        auto next = runtime.ReadWord(left + 8u);
        while (next != runtime.Nil()) {
            left = next;
            next = runtime.ReadWord(left + 8u);
        }
        runtime.WriteWord(iterator, left);
        return next;
    }

    auto current_parent = parent;
    while (node == runtime.ReadWord(current_parent)) {
        runtime.WriteWord(iterator, current_parent);
        node = current_parent;
        current_parent = runtime.ReadWord(current_parent + 4u);
    }
    runtime.WriteWord(iterator, current_parent);
    return current_parent;
}

}  // namespace pal4
