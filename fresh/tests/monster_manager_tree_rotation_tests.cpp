#include "pal4/monster_manager_tree_rotation.h"
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

struct Memory {
    static constexpr std::uint32_t kTree = 0x1000u;
    static constexpr std::uint32_t kHeader = 0x2000u;
    static constexpr std::uint32_t kNil = 0x3000u;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    Memory() { words[kTree + 4u] = kHeader; }
    std::uint32_t SharedNil() const { return kNil; }
    std::uint32_t ReadDword(std::uint32_t address) const {
        return words.at(address);
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) {
        words[address] = value;
    }
    void Node(std::uint32_t address, std::uint32_t left,
              std::uint32_t parent, std::uint32_t right) {
        words[address] = left;
        words[address + 4u] = parent;
        words[address + 8u] = right;
    }
};

void TestRootRotations() {
    Memory memory;
    constexpr std::uint32_t a = 0x4000u;
    constexpr std::uint32_t b = 0x5000u;
    constexpr std::uint32_t moved = 0x4800u;
    memory.words[Memory::kHeader + 4u] = a;
    memory.Node(a, Memory::kNil, Memory::kHeader, b);
    memory.Node(b, moved, a, Memory::kNil);
    memory.Node(moved, Memory::kNil, b, Memory::kNil);
    Check(pal4::MonsterManagerTreeRotation::RotateRight(
        Memory::kTree, a, memory) == b, "IDA RotateRight returns right child");
    Check(memory.ReadDword(Memory::kHeader + 4u) == b &&
          memory.ReadDword(b + 4u) == Memory::kHeader &&
          memory.ReadDword(b) == a && memory.ReadDword(a + 4u) == b &&
          memory.ReadDword(a + 8u) == moved &&
          memory.ReadDword(moved + 4u) == a,
          "right child promoted at root");
    Check(pal4::MonsterManagerTreeRotation::RotateLeft(
        Memory::kTree, b, memory) == a, "IDA RotateLeft returns left child");
    Check(memory.ReadDword(Memory::kHeader + 4u) == a &&
          memory.ReadDword(a + 8u) == b && memory.ReadDword(b) == moved &&
          memory.ReadDword(moved + 4u) == b,
          "left child promoted at root");
}

void TestNonRootLinks() {
    Memory left;
    constexpr std::uint32_t root = 0x7000u;
    constexpr std::uint32_t child = 0x5000u;
    constexpr std::uint32_t promoted = 0x6000u;
    left.words[Memory::kHeader + 4u] = root;
    left.Node(root, child, Memory::kHeader, Memory::kNil);
    left.Node(child, Memory::kNil, root, promoted);
    left.Node(promoted, Memory::kNil, child, Memory::kNil);
    (void)pal4::MonsterManagerTreeRotation::RotateRight(
        Memory::kTree, child, left);
    Check(left.ReadDword(root) == promoted &&
          left.ReadDword(promoted + 4u) == root &&
          left.ReadDword(promoted) == child,
          "right-child promotion updates left parent link");

    Memory right;
    right.words[Memory::kHeader + 4u] = root;
    right.Node(root, Memory::kNil, Memory::kHeader, child);
    right.Node(child, promoted, root, Memory::kNil);
    right.Node(promoted, Memory::kNil, child, Memory::kNil);
    (void)pal4::MonsterManagerTreeRotation::RotateLeft(
        Memory::kTree, child, right);
    Check(right.ReadDword(root + 8u) == promoted &&
          right.ReadDword(promoted + 4u) == root &&
          right.ReadDword(promoted + 8u) == child,
          "left-child promotion updates right parent link");
}

void TestNpcRotationEntries() {
    Memory memory;
    memory.words[Memory::kHeader + 4u] = 0x4000u;
    memory.Node(0x4000u, Memory::kNil, Memory::kHeader, 0x5000u);
    memory.Node(0x5000u, Memory::kNil, 0x4000u, Memory::kNil);
    Check(pal4::NpcManagerTree::RotateRight(
              Memory::kTree, 0x4000u, memory) == 0x5000u &&
          memory.ReadDword(Memory::kHeader + 4u) == 0x5000u,
          "NPC right-symbol rotation promotes right child");
    Check(pal4::NpcManagerTree::RotateLeft(
              Memory::kTree, 0x5000u, memory) == 0x4000u &&
          memory.ReadDword(Memory::kHeader + 4u) == 0x4000u,
          "NPC left-symbol rotation restores original root");
}

}  // namespace

int main() {
    TestRootRotations();
    TestNonRootLinks();
    TestNpcRotationEntries();
}
