#include "pal4/native_gob_name_tree.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main() {
    pal4::NativeBaseObjectMemory memory;
    const auto before = memory.OutstandingAllocations();
    {
        pal4::NativeGobNameTree tree(memory);
        const auto address = tree.Address();
        const auto head = memory.ReadDword(address + 4u);
        const auto nil = memory.ReadDword(head + 4u);
        Check(address != 0u && head != 0u && nil != 0u &&
                  tree.Size() == 0u && tree.Find("absent") == 0u,
              "empty original GOB name tree");
        constexpr std::array names{"MO001", "effect001", "MO003", "MO002"};
        for (std::size_t index = 0u; index < names.size(); ++index) {
            Check(tree.Insert(names[index],
                              static_cast<std::uint32_t>(0x1000u + index)) &&
                      tree.Size() == index + 1u,
                  "GOB tree insert and count");
        }
        for (std::size_t index = 0u; index < names.size(); ++index)
            Check(tree.Find(names[index]) == 0x1000u + index,
                  "GOB tree resolves the inserted object");
        Check(!tree.Insert("MO002", 0x9999u) && tree.Size() == names.size() &&
                  tree.Find("MO002") == 0x1003u &&
                  tree.Find("MO004") == 0u &&
                  tree.Find("MO") == 0u &&
                  memory.ReadDword(head + 4u) != nil &&
                  memory.ReadDword(
                      memory.ReadDword(head + 4u) + 32u) == 1u,
              "GOB tree duplicate, miss and black root");
        std::vector<std::uint32_t> visited;
        tree.Visit([&](const std::uint32_t object) {
            visited.push_back(object);
            return true;
        });
        Check(visited == std::vector<std::uint32_t>{0x1000u, 0x1003u,
                                                     0x1002u, 0x1001u},
              "GOB tree visits objects in name order");
        visited.clear();
        tree.Visit([&](const std::uint32_t object) {
            visited.push_back(object);
            return false;
        });
        Check(visited == std::vector<std::uint32_t>{0x1000u},
              "GOB tree stops when callback returns zero");
    }
    Check(memory.OutstandingAllocations() == before,
          "GOB tree releases nodes, strings, head and nil");
}
