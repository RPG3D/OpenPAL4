#include "pal4/floor_color_tree.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using std::uint32_t;

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Memory final : pal4::PalFloorAttributeRuntimeStub {
    std::uint8_t ReadByte(uint32_t address) override {
        return bytes[address];
    }
    uint32_t ReadDword(uint32_t address) override {
        uint32_t value = 0u;
        for (uint32_t i = 0u; i < 4u; ++i)
            value |= static_cast<uint32_t>(ReadByte(address + i)) << (8u * i);
        return value;
    }
    void WriteByte(uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    void WriteDword(uint32_t address, uint32_t value) override {
        for (uint32_t i = 0u; i < 4u; ++i)
            WriteByte(address + i,
                      static_cast<std::uint8_t>(value >> (8u * i)));
    }
    uint32_t EmptyTextAddress() override { return 0x7000u; }
    void SetErrorFlag(uint32_t record, std::uint8_t flag) override {
        WriteByte(record, flag);
    }
    bool AllocateText(uint32_t record, uint32_t size,
                      std::uint8_t mode) override {
        Check(mode == 1u, "color-node text allocation mode");
        if (fail_text_allocation) return false;
        next_text += 0x100u;
        WriteDword(record + 4u, next_text);
        WriteDword(record + 12u, size);
        return true;
    }
    void CopyBytes(uint32_t destination, uint32_t source,
                   uint32_t size) override {
        for (uint32_t i = 0; i < size; ++i)
            WriteByte(destination + i, ReadByte(source + i));
    }
    uint32_t AllocateColorNode(uint32_t bytes_count) override {
        Check(bytes_count == 36u, "color node allocation size");
        next_node += 0x100u;
        allocated.push_back(next_node);
        return next_node;
    }
    uint32_t FreeAddress(uint32_t address) override {
        freed.push_back(address);
        return 0x1234u;
    }
    pal4::FloorColorTreeGlobals& ColorTreeGlobals() override {
        return globals;
    }
    uint32_t Value(uint32_t index, uint32_t key,
                   std::string_view text = "wav") {
        const auto address = 0x30000u + index * 0x100u;
        const auto data = 0x50000u + index * 0x100u;
        WriteDword(address, key);
        WriteByte(address + 4u, 0x5au);
        WriteDword(address + 8u, data);
        WriteDword(address + 12u, static_cast<uint32_t>(text.size()));
        WriteDword(address + 16u, static_cast<uint32_t>(text.size()));
        for (std::size_t i = 0; i < text.size(); ++i)
            WriteByte(data + static_cast<uint32_t>(i),
                      static_cast<std::uint8_t>(text[i]));
        WriteByte(data + static_cast<uint32_t>(text.size()), 0u);
        WriteByte(data - 1u, 0u);
        return address;
    }
    static constexpr uint32_t kNil = 0x8000u;
    static constexpr uint32_t kHeader = 0x8100u;
    std::map<uint32_t, std::uint8_t> bytes;
    uint32_t next_node{0x7f00u};
    uint32_t next_text{0x70000u};
    std::vector<uint32_t> allocated;
    std::vector<uint32_t> freed;
    pal4::FloorColorTreeGlobals globals{};
    bool fail_text_allocation{};
};

int CheckSubtree(Memory& memory, uint32_t node, uint32_t parent,
                 std::vector<uint32_t>& inorder) {
    if (node == Memory::kNil) return 1;
    Check(memory.ReadDword(node + 4u) == parent, "tree parent link");
    const auto left = memory.ReadDword(node);
    const auto right = memory.ReadDword(node + 8u);
    const auto black = memory.ReadDword(node + 32u);
    Check(black <= 1u, "node color is black or red");
    if (black == 0u) {
        Check(memory.ReadDword(left + 32u) == 1u &&
                  memory.ReadDword(right + 32u) == 1u,
              "red node has black children");
    }
    const auto left_height = CheckSubtree(memory, left, node, inorder);
    inorder.push_back(memory.ReadDword(node + 12u));
    const auto right_height = CheckSubtree(memory, right, node, inorder);
    Check(left_height == right_height, "equal black height");
    return left_height + static_cast<int>(black);
}

void CheckTree(pal4::FloorColorTreeState& tree, Memory& memory,
               const std::vector<uint32_t>& expected) {
    const auto root = memory.ReadDword(tree.header + 4u);
    if (expected.empty()) {
        Check(root == Memory::kNil, "empty tree root");
        return;
    }
    Check(memory.ReadDword(root + 32u) == 1u,
          "root is black");
    std::vector<uint32_t> inorder;
    static_cast<void>(CheckSubtree(memory, root, tree.header, inorder));
    Check(inorder == expected, "in-order keys match inserted records");
    Check(tree.size == expected.size(), "node count");
    Check(memory.ReadDword(memory.ReadDword(tree.header) + 12u) ==
              expected.front() &&
              memory.ReadDword(memory.ReadDword(tree.header + 8u) + 12u) ==
                  expected.back(),
          "header points to minimum and maximum");
}

void TestInitialization() {
    Memory memory;
    pal4::PalFloorAttributeState state{};
    state.color_tree.flags = 0xaabbccddu;
    state.color_tree.reserved = 0xffffccffu;
    pal4::FloorColorTree::Initialize(state, 0x12u, 0x34u, false, memory);
    Check(state.color_tree.flags == 0xaabb3412u &&
              state.color_tree.reserved == 0xffffcc00u &&
              state.color_tree.header == Memory::kHeader,
          "0x420f90 writes only observed bytes before header setup");
    CheckTree(state.color_tree, memory, {});
    Check(memory.globals.nil == Memory::kNil &&
              memory.globals.owners == 1u &&
              memory.ReadDword(Memory::kNil) == 0u &&
              memory.ReadDword(Memory::kNil + 4u) == 0u &&
              memory.ReadDword(Memory::kNil + 8u) == 0u &&
              memory.ReadDword(Memory::kNil + 32u) == 1u &&
              memory.ReadDword(Memory::kHeader + 32u) == 0u,
          "shared nil and header preserve observed initialization writes");
}

void TestSharedNilLifetime() {
    Memory memory;
    pal4::PalFloorAttributeState first{};
    pal4::PalFloorAttributeState second{};
    pal4::FloorColorTree::Initialize(first, 0u, 0u, false, memory);
    pal4::FloorColorTree::Initialize(second, 0u, 0u, false, memory);
    Check(memory.globals.nil == Memory::kNil &&
              memory.globals.owners == 2u &&
              second.color_tree.header == 0x8300u &&
              memory.freed == std::vector<uint32_t>{0x8200u},
          "second tree frees temporary nil and shares original");
    pal4::FloorColorTree::ReleaseSharedNil(memory);
    Check(memory.globals.nil == Memory::kNil &&
              memory.globals.owners == 1u &&
              memory.freed.size() == 1u,
          "first release retains shared nil");
    pal4::FloorColorTree::ReleaseSharedNil(memory);
    Check(memory.globals.nil == 0u &&
              memory.globals.owners == 0u &&
              memory.freed == std::vector<uint32_t>{0x8200u, Memory::kNil},
          "last release clears and frees shared nil");
}

void TestDirectNodeInsertion() {
    Memory memory;
    pal4::PalFloorAttributeState state{};
    pal4::FloorColorTree::Initialize(state, 0u, 0u, false, memory);
    auto& tree = state.color_tree;
    const auto root = pal4::FloorColorTree::InsertAt(
        tree, Memory::kNil, tree.header, memory.Value(0u, 10u), memory);
    Check(root == memory.ReadDword(tree.header + 4u) &&
              memory.ReadDword(root + 4u) == tree.header,
          "direct insertion initializes root and parent");
    const auto left = pal4::FloorColorTree::InsertAt(
        tree, Memory::kNil, root, memory.Value(1u, 5u), memory);
    const auto right = pal4::FloorColorTree::InsertAt(
        tree, Memory::kNil, root, memory.Value(2u, 15u), memory);
    Check(memory.ReadDword(root) == left &&
              memory.ReadDword(root + 8u) == right &&
              memory.ReadDword(tree.header) == left &&
              memory.ReadDword(tree.header + 8u) == right,
          "direct insertion updates child and extrema links");
    CheckTree(tree, memory, {5u, 10u, 15u});
}

void TestColorValueCopy() {
    Memory memory;
    const auto source = memory.Value(0u, 0xaabbccddu, "sound");
    const auto source_data = memory.ReadDword(source + 8u);
    const auto original_size = memory.bytes.size();
    pal4::FloorColorValue::Copy(0u, source, memory);
    Check(memory.bytes.size() == original_size,
          "null color value destination is untouched");

    constexpr uint32_t shared = 0x60000u;
    pal4::FloorColorValue::Copy(shared, source, memory);
    Check(memory.ReadDword(shared) == 0xaabbccddu &&
              memory.ReadByte(shared + 4u) == 1u &&
              memory.ReadDword(shared + 8u) == source_data &&
              memory.ReadDword(shared + 12u) == 5u &&
              memory.ReadByte(source_data - 1u) == 1u,
          "color value copy shares text and increments reference byte");

    memory.WriteByte(source_data - 1u, 0xfeu);
    constexpr uint32_t deep = 0x60100u;
    pal4::FloorColorValue::Copy(deep, source, memory);
    const auto copied_data = memory.ReadDword(deep + 8u);
    Check(memory.ReadDword(deep) == 0xaabbccddu &&
              copied_data != source_data &&
              memory.ReadByte(copied_data) == 's' &&
              memory.ReadByte(copied_data + 5u) == 0u,
          "saturated color text takes deep-copy branch");

    memory.fail_text_allocation = true;
    constexpr uint32_t failed = 0x60200u;
    pal4::FloorColorValue::Copy(failed, source, memory);
    Check(memory.ReadDword(failed) == 0xaabbccddu &&
              memory.ReadDword(failed + 8u) == 0u &&
              memory.ReadDword(failed + 12u) == 0u,
          "failed text allocation leaves copied key and empty text");
}

void TestLeftRotation() {
    Memory memory;
    pal4::PalFloorAttributeState state{};
    pal4::FloorColorTree::Initialize(state, 0u, 0u, false, memory);
    const auto header = state.color_tree.header;
    constexpr uint32_t root = 0x9000u;
    constexpr uint32_t promoted = 0x9100u;
    constexpr uint32_t middle = 0x9200u;
    memory.WriteDword(header + 4u, root);
    memory.WriteDword(root, Memory::kNil);
    memory.WriteDword(root + 4u, header);
    memory.WriteDword(root + 8u, promoted);
    memory.WriteDword(promoted, middle);
    memory.WriteDword(promoted + 4u, root);
    memory.WriteDword(promoted + 8u, Memory::kNil);
    memory.WriteDword(middle + 4u, promoted);
    Check(pal4::FloorColorTree::RotateLeft(state.color_tree, root,
                                             memory) == promoted &&
              memory.ReadDword(header + 4u) == promoted &&
              memory.ReadDword(promoted + 4u) == header &&
              memory.ReadDword(promoted) == root &&
              memory.ReadDword(root + 4u) == promoted &&
              memory.ReadDword(root + 8u) == middle &&
              memory.ReadDword(middle + 4u) == root,
          "left rotation promotes root and reparents middle subtree");
    Check(pal4::FloorColorTree::RotateRight(state.color_tree, promoted,
                                              memory) == root &&
              memory.ReadDword(header + 4u) == root &&
              memory.ReadDword(root + 4u) == header &&
              memory.ReadDword(root + 8u) == promoted &&
              memory.ReadDword(promoted + 4u) == root &&
              memory.ReadDword(promoted) == middle &&
              memory.ReadDword(middle + 4u) == promoted,
          "right rotation restores root and middle subtree links");

    Memory branch;
    pal4::PalFloorAttributeState other{};
    pal4::FloorColorTree::Initialize(other, 0u, 0u, false, branch);
    const auto other_header = other.color_tree.header;
    branch.WriteDword(other_header + 4u, root);
    branch.WriteDword(root + 4u, other_header);
    branch.WriteDword(root + 8u, promoted);
    branch.WriteDword(promoted + 4u, root);
    branch.WriteDword(promoted + 8u, middle);
    branch.WriteDword(middle, Memory::kNil);
    branch.WriteDword(middle + 4u, promoted);
    Check(pal4::FloorColorTree::RotateLeft(other.color_tree, promoted,
                                             branch) == middle &&
              branch.ReadDword(other_header + 4u) == root &&
              branch.ReadDword(root + 8u) == middle &&
              branch.ReadDword(middle + 4u) == root &&
              branch.ReadDword(middle) == promoted &&
              branch.ReadDword(promoted + 4u) == middle,
          "left rotation replaces a non-root right child");
    Check(pal4::FloorColorTree::RotateRight(other.color_tree, middle,
                                              branch) == promoted &&
              branch.ReadDword(other_header + 4u) == root &&
              branch.ReadDword(root + 8u) == promoted &&
              branch.ReadDword(promoted + 4u) == root &&
              branch.ReadDword(promoted + 8u) == middle &&
              branch.ReadDword(middle + 4u) == promoted,
          "right rotation restores a non-root right child");
}

void TestLeftmost() {
    Memory memory;
    pal4::PalFloorAttributeState state{};
    pal4::FloorColorTree::Initialize(state, 0u, 0u, false, memory);
    Check(pal4::FloorColorTree::Leftmost(
              state.color_tree, Memory::kNil, memory) == Memory::kNil,
          "leftmost of nil remains nil");
    constexpr uint32_t root = 0x9000u;
    constexpr uint32_t middle = 0x9100u;
    constexpr uint32_t leaf = 0x9200u;
    memory.WriteDword(root, Memory::kNil);
    Check(pal4::FloorColorTree::Leftmost(
              state.color_tree, root, memory) == root,
          "leftmost of leaf is itself");
    memory.WriteDword(root, middle);
    memory.WriteDword(middle, leaf);
    memory.WriteDword(leaf, Memory::kNil);
    Check(pal4::FloorColorTree::Leftmost(
              state.color_tree, root, memory) == leaf,
          "leftmost follows the full left chain");
}

void TestTreeCursors() {
    Memory memory;
    pal4::PalFloorAttributeState state{};
    pal4::FloorColorTree::Initialize(state, 0u, 0u, false, memory);
    auto& tree = state.color_tree;
    const std::vector<uint32_t> insertion{10u, 5u, 15u, 3u,
                                           7u, 12u, 20u};
    const std::vector<uint32_t> sorted{3u, 5u, 7u, 10u,
                                        12u, 15u, 20u};
    for (std::size_t i = 0; i < insertion.size(); ++i)
        static_cast<void>(pal4::FloorColorTree::Insert(
            tree, memory.Value(static_cast<uint32_t>(i), insertion[i]), memory));
    for (std::size_t i = 0; i < sorted.size(); ++i) {
        const auto node = pal4::FloorColorTree::Find(tree, sorted[i], memory);
        const auto before = i == 0u ? tree.header :
            pal4::FloorColorTree::Find(tree, sorted[i - 1u], memory);
        const auto after = i + 1u == sorted.size() ? tree.header :
            pal4::FloorColorTree::Find(tree, sorted[i + 1u], memory);
        Check(pal4::FloorColorTree::Previous(tree, node, memory) == before &&
                  pal4::FloorColorTree::Next(tree, node, memory) == after,
              "tree cursors follow sorted keys in both directions");
    }
    Check(pal4::FloorColorTree::Previous(tree, tree.header, memory) ==
              memory.ReadDword(tree.header + 8u),
          "predecessor of end cursor is maximum");

    Memory singleton;
    pal4::PalFloorAttributeState only{};
    pal4::FloorColorTree::Initialize(only, 0u, 0u, false, singleton);
    const auto root = pal4::FloorColorTree::Insert(
        only.color_tree, singleton.Value(0u, 1u), singleton).node;
    Check(pal4::FloorColorTree::Previous(only.color_tree, root,
                                          singleton) == root,
          "single-node predecessor preserves observed header ascent");
}

void TestUniqueInsertionAndLookup() {
    Memory memory;
    pal4::PalFloorAttributeState state{};
    pal4::FloorColorTree::Initialize(state, 0u, 0u, false, memory);
    const std::vector<uint32_t> input{
        10u, 20u, 30u, 15u, 5u, 1u, 25u, 40u, 50u, 45u, 18u};
    std::vector<uint32_t> expected;
    uint32_t source_index = 0u;
    for (const auto key : input) {
        const auto source = memory.Value(source_index++, key);
        const auto result = pal4::FloorColorTree::Insert(
            state.color_tree, source, memory);
        Check(result.inserted && result.node != Memory::kHeader &&
                  memory.ReadDword(result.node + 12u) == key &&
                  memory.ReadByte(result.node + 16u) == 1u &&
                  memory.ReadByte(memory.ReadDword(source + 8u) - 1u) == 1u,
              "new node copies key and shares text");
        expected.push_back(key);
        std::sort(expected.begin(), expected.end());
        CheckTree(state.color_tree, memory, expected);
        Check(pal4::FloorColorTree::Find(state.color_tree, key, memory) ==
                  result.node,
              "exact color lookup");
    }
    const auto original = pal4::FloorColorTree::Find(
        state.color_tree, 20u, memory);
    const auto allocations = memory.allocated.size();
    const auto duplicate = pal4::FloorColorTree::Insert(
        state.color_tree, memory.Value(source_index++, 20u), memory);
    Check(!duplicate.inserted && duplicate.node == original &&
              memory.allocated.size() == allocations,
          "unique tree returns existing color without allocation");
    Check(pal4::FloorColorTree::Find(state.color_tree, 19u, memory) ==
              Memory::kHeader &&
              pal4::FloorColorTree::Find(state.color_tree, 99u, memory) ==
                  Memory::kHeader,
          "missing colors return header");
}

void TestDuplicateModeAndUnsignedKeys() {
    Memory memory;
    pal4::PalFloorAttributeState state{};
    pal4::FloorColorTree::Initialize(state, 0u, 0u, true, memory);
    const std::vector<uint32_t> keys{
        0xffffffffu, 0u, 0x80000000u, 0x80000000u, 1u};
    std::vector<uint32_t> expected;
    for (std::size_t i = 0; i < keys.size(); ++i) {
        const auto result = pal4::FloorColorTree::Insert(
            state.color_tree,
            memory.Value(static_cast<uint32_t>(i), keys[i]), memory);
        Check(result.inserted, "duplicate mode inserts equal keys");
        expected.push_back(keys[i]);
        std::sort(expected.begin(), expected.end());
        CheckTree(state.color_tree, memory, expected);
    }
}

void TestRangeCleanup() {
    Memory memory;
    pal4::PalFloorAttributeState state{};
    pal4::FloorColorTree::Initialize(state, 0u, 0u, false, memory);
    for (uint32_t i = 0; i < 3u; ++i)
        static_cast<void>(pal4::FloorColorTree::Insert(
            state.color_tree, memory.Value(i, i + 1u), memory));
    auto& tree = state.color_tree;
    Check(pal4::FloorColorTree::EraseRange(
              tree, memory.ReadDword(tree.header), tree.header,
              memory) == tree.header &&
              tree.size == 0u &&
              memory.freed.size() == 3u &&
              memory.ReadDword(tree.header) == tree.header &&
              memory.ReadDword(tree.header + 4u) == Memory::kNil &&
              memory.ReadDword(tree.header + 8u) == tree.header,
          "whole-tree erase releases subtree and resets header");

    Memory partial;
    pal4::PalFloorAttributeState other{};
    pal4::FloorColorTree::Initialize(other, 0u, 0u, false, partial);
    for (uint32_t i = 0; i < 3u; ++i)
        static_cast<void>(pal4::FloorColorTree::Insert(
            other.color_tree, partial.Value(i, i + 1u), partial));
    auto& partial_tree = other.color_tree;
    const auto first = pal4::FloorColorTree::Find(partial_tree, 1u, partial);
    const auto last = pal4::FloorColorTree::Find(partial_tree, 3u, partial);
    Check(pal4::FloorColorTree::EraseRange(
              partial_tree, first, last, partial) == last &&
              partial_tree.size == 1u &&
              partial.freed.size() == 2u,
          "partial erase removes two nodes through tree deletion");
    CheckTree(partial_tree, partial, {3u});
    Check(pal4::FloorColorTree::EraseRange(
              partial_tree, last, last, partial) == last &&
              partial.freed.size() == 2u,
          "empty range leaves nodes alone");
}

void TestNodeDeletion(const std::vector<uint32_t>& removal_order) {
    Memory memory;
    pal4::PalFloorAttributeState state{};
    pal4::FloorColorTree::Initialize(state, 0u, 0u, false, memory);
    const std::vector<uint32_t> insertion_order{
        10u, 5u, 15u, 2u, 7u, 12u, 20u, 1u,
        3u, 6u, 8u, 11u, 13u, 18u, 25u};
    std::map<uint32_t, uint32_t> source_data;
    std::vector<uint32_t> expected;
    for (std::size_t i = 0; i < insertion_order.size(); ++i) {
        const auto source = memory.Value(static_cast<uint32_t>(i),
                                         insertion_order[i]);
        source_data[insertion_order[i]] = memory.ReadDword(source + 8u);
        static_cast<void>(pal4::FloorColorTree::Insert(
            state.color_tree, source, memory));
        expected.push_back(insertion_order[i]);
    }
    std::sort(expected.begin(), expected.end());
    for (const auto key : removal_order) {
        const auto node = pal4::FloorColorTree::Find(
            state.color_tree, key, memory);
        Check(node != state.color_tree.header, "key exists before delete");
        const auto next_it = std::upper_bound(expected.begin(),
                                               expected.end(), key);
        const auto expected_next = next_it == expected.end()
            ? state.color_tree.header
            : pal4::FloorColorTree::Find(
                  state.color_tree, *next_it, memory);
        const auto next = pal4::FloorColorTree::Remove(
            state.color_tree, node, memory);
        Check(next == expected_next && memory.freed.back() == node &&
                  memory.ReadByte(source_data[key] - 1u) == 0u,
              "delete returns precomputed successor and releases node text");
        expected.erase(std::lower_bound(expected.begin(),
                                        expected.end(), key));
        CheckTree(state.color_tree, memory, expected);
        Check(pal4::FloorColorTree::Find(state.color_tree, key, memory) ==
                  state.color_tree.header,
              "deleted key is absent");
    }
    Check(state.color_tree.size == 0u &&
              memory.freed.size() == insertion_order.size(),
          "all inserted nodes are freed exactly once");
}

void TestSaturatedTextRelease() {
    Memory memory;
    pal4::PalFloorAttributeState state{};
    pal4::FloorColorTree::Initialize(state, 0u, 0u, false, memory);
    const auto source = memory.Value(0u, 42u, "sound");
    const auto original_data = memory.ReadDword(source + 8u);
    memory.WriteByte(original_data - 1u, 0xfeu);
    const auto inserted = pal4::FloorColorTree::Insert(
        state.color_tree, source, memory);
    const auto copied_data = memory.ReadDword(inserted.node + 20u);
    Check(copied_data != original_data &&
              memory.ReadByte(copied_data) == 's',
          "saturated reference count deep-copies node text");
    static_cast<void>(pal4::FloorColorTree::Remove(
        state.color_tree, inserted.node, memory));
    Check(memory.freed == std::vector<uint32_t>{
              copied_data - 1u, inserted.node},
          "delete frees owned text before node storage");
}

}  // namespace

int main() {
    TestInitialization();
    TestSharedNilLifetime();
    TestDirectNodeInsertion();
    TestColorValueCopy();
    TestLeftRotation();
    TestLeftmost();
    TestTreeCursors();
    TestUniqueInsertionAndLookup();
    TestDuplicateModeAndUnsignedKeys();
    TestRangeCleanup();
    TestNodeDeletion({1u, 2u, 3u, 5u, 6u, 7u, 8u, 10u,
                      11u, 12u, 13u, 15u, 18u, 20u, 25u});
    TestNodeDeletion({25u, 20u, 18u, 15u, 13u, 12u, 11u, 10u,
                      8u, 7u, 6u, 5u, 3u, 2u, 1u});
    TestNodeDeletion({10u, 5u, 15u, 2u, 7u, 12u, 20u, 1u,
                      3u, 6u, 8u, 11u, 13u, 18u, 25u});
    TestSaturatedTextRelease();
}
