#include "pal4/monster_manager_tree_cursor.h"
#include "pal4/npc_manager_tree.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <unordered_map>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void TestSuccessor() {
    constexpr std::uint32_t nil = 0x3000u;
    constexpr std::uint32_t header = 0x2000u;
    constexpr std::uint32_t root = 0x5000u;
    constexpr std::uint32_t left = 0x4000u;
    constexpr std::uint32_t right = 0x6000u;
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {header, left}, {header + 4u, root}, {header + 8u, right},
        {root, left}, {root + 4u, header}, {root + 8u, right},
        {left, nil}, {left + 4u, root}, {left + 8u, nil},
        {right, nil}, {right + 4u, root}, {right + 8u, nil},
        {header + 0x20u, 0u}, {root + 0x20u, 1u},
        {left + 0x20u, 1u}, {right + 0x20u, 1u},
    };
    const auto read = [&](std::uint32_t address) { return words.at(address); };
    const auto from_left = pal4::MonsterManagerTreeCursor::Successor(
        left, nil, read);
    Check(from_left.next == root && from_left.result == root,
          "left child ascends to parent");
    const auto from_root = pal4::MonsterManagerTreeCursor::Successor(
        root, nil, read);
    Check(from_root.next == right && from_root.result == nil,
          "right subtree successor and raw EAX");
    const auto from_right = pal4::MonsterManagerTreeCursor::Successor(
        right, nil, read);
    Check(from_right.next == header && from_right.result == header,
          "rightmost successor is header");

    const auto before_root = pal4::MonsterManagerTreeCursor::Predecessor(
        root, nil, read);
    Check(before_root.next == left && before_root.result == nil,
          "predecessor descends left subtree right spine");
    const auto before_left = pal4::MonsterManagerTreeCursor::Predecessor(
        left, nil, read);
    Check(before_left.next == header && before_left.result == header,
          "leftmost predecessor is header");
    const auto before_right = pal4::MonsterManagerTreeCursor::Predecessor(
        right, nil, read);
    Check(before_right.next == root && before_right.result == root,
          "right child predecessor ascends to parent");
    const auto before_header = pal4::MonsterManagerTreeCursor::Predecessor(
        header, nil, read);
    Check(before_header.next == right && before_header.result == right,
          "red header advances to rightmost");

    constexpr std::uint32_t middle = 0x5800u;
    constexpr std::uint32_t inner = 0x5400u;
    words[right] = middle;
    words[middle] = inner;
    words[inner] = nil;
    const auto deep = pal4::MonsterManagerTreeCursor::Successor(root, nil, read);
    Check(deep.next == inner && deep.result == nil,
          "right subtree descends left spine");
    Check(pal4::NpcManagerTree::Minimum(right, nil, read) == inner,
          "NPC minimum follows the deep left spine");
    Check(pal4::NpcManagerTree::Predecessor(
              left, nil, read).next == header &&
          pal4::NpcManagerTree::Predecessor(
              header, nil, read).next == right,
          "NPC predecessor handles leftmost and header cases");
}

}  // namespace

int main() {
    TestSuccessor();
}
