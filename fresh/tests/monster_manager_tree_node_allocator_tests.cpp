#include "pal4/monster_manager_tree_node_allocator.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Memory {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {0x4000u, 0xaaaaaaaau}, {0x4008u, 0xbbbbbbbbu},
        {0x400cu, 0xccccccccu},
    };
    std::vector<std::uint32_t> writes;
    std::uint32_t AllocateMemory(std::uint32_t bytes) {
        Check(bytes == 0x24u, "original node size");
        return 0x4000u;
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) {
        writes.push_back(address);
        words[address] = value;
    }
};

}  // namespace

int main() {
    Memory memory;
    Check(pal4::MonsterManagerTreeNodeAllocator::Allocate(
        0x5000u, 0u, memory) == 0x4000u,
        "allocator returns raw node");
    Check(memory.writes == std::vector<std::uint32_t>{0x4004u, 0x4020u} &&
          memory.words.at(0x4004u) == 0x5000u &&
          memory.words.at(0x4020u) == 0u,
          "only parent and color are initialized");
    Check(memory.words.at(0x4000u) == 0xaaaaaaaau &&
          memory.words.at(0x4008u) == 0xbbbbbbbbu &&
          memory.words.at(0x400cu) == 0xccccccccu,
          "other node bytes are untouched");
}
