#pragma once

#include "pal4/monster_manager_tree_insert.h"
#include "pal4/monster_manager_tree_lookup.h"
#include "pal4/monster_manager_tree_remove.h"
#include "pal4/monster_manager_tree_cursor.h"
#include "pal4/monster_manager_tree_cleanup.h"
#include "pal4/monster_manager_tree_rotation.h"
#include "pal4/monster_manager_tree_text_assignment.h"

namespace pal4 {

class NpcManagerTree final {
public:
    // IDA 0x43d100: name-keyed 36-byte red-black node insertion.
    [[nodiscard]] static MonsterManagerTreeInsertResult Insert(
        std::uint32_t tree, std::uint32_t name,
        IMonsterManagerTreeInsertRuntime& runtime);
    // IDA 0x43d360: remove a node and return its saved successor.
    [[nodiscard]] static std::uint32_t Remove(
        std::uint32_t tree, std::uint32_t node,
        IMonsterManagerTreeRemoveRuntime& runtime);
    // IDA 0x43d770: candidate search with final byte/length check.
    [[nodiscard]] static std::uint32_t Search(
        std::uint32_t tree, std::uint32_t name,
        IMonsterManagerTreeLookupRuntime& runtime);
    // IDA 0x43d820: advance an in-order cursor.
    [[nodiscard]] static MonsterManagerTreeStep Successor(
        std::uint32_t node, std::uint32_t nil,
        IMonsterManagerTreeLookupRuntime& runtime);
    // IDA 0x43d870: lower-bound candidate without the final byte check.
    [[nodiscard]] static std::uint32_t LowerBoundAlternative(
        std::uint32_t tree, std::uint32_t name,
        IMonsterManagerTreeLookupRuntime& runtime);
    // IDA 0x43d8d0: recurse right, then iteratively free the left chain.
    [[nodiscard]] static std::uint32_t DestroySubtree(
        std::uint32_t root, IMonsterManagerTreeCleanupRuntime& runtime);
    // IDA 0x43d920: raw-node insertion and red-black repair.
    [[nodiscard]] static std::uint32_t InsertHelper(
        std::uint32_t tree, std::uint32_t child,
        std::uint32_t parent, std::uint32_t name,
        IMonsterManagerTreeInsertRuntime& runtime);
    // IDA 0x43db90: promotes the right child, despite the symbol name.
    template <class Memory>
    [[nodiscard]] static std::uint32_t RotateRight(
        std::uint32_t tree, std::uint32_t node, Memory& memory) {
        return MonsterManagerTreeRotation::RotateRight(tree, node, memory);
    }
    // IDA 0x43dbf0: descend to the leftmost node of a subtree.
    template <class Reader>
    [[nodiscard]] static std::uint32_t Minimum(
        std::uint32_t node, std::uint32_t nil, Reader&& read) {
        auto left = read(node);
        while (left != nil) {
            node = left;
            left = read(node);
        }
        return node;
    }
    // IDA 0x43dc10: promotes the left child, despite the symbol name.
    template <class Memory>
    [[nodiscard]] static std::uint32_t RotateLeft(
        std::uint32_t tree, std::uint32_t node, Memory& memory) {
        return MonsterManagerTreeRotation::RotateLeft(tree, node, memory);
    }
    // IDA 0x43dc70: advance to the predecessor, including the header case.
    template <class Reader>
    [[nodiscard]] static MonsterManagerTreeStep Predecessor(
        std::uint32_t node, std::uint32_t nil, Reader&& read) {
        return MonsterManagerTreeCursor::Predecessor(
            node, nil, read);
    }
    // IDA 0x43dcd0: copy/share a 20-byte string-key record.
    static void AssignText(std::uint32_t destination,
                           std::uint32_t source,
                           IMonsterManagerTreeTextRuntime& runtime);
};

}  // namespace pal4
