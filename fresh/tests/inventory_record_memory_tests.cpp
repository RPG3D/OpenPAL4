#include "pal4/inventory_record_memory.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IInventoryRecordMemoryRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> writes;
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        writes.push_back(address);
        words[address] = value;
    }
};

void Seed(Runtime& runtime, std::uint32_t address,
          std::uint32_t first) {
    runtime.words[address] = first;
    runtime.words[address + 4u] = first + 1u;
    runtime.words[address + 8u] = first + 2u;
}

void CheckConstructionCopies() {
    Runtime runtime;
    Seed(runtime, 0x9000u, 10u);
    Seed(runtime, 0x900cu, 20u);
    Check(pal4::InventoryRecordMemory::CopyConstruct(
              0xa000u, 0x9000u, runtime) == 0xa000u &&
              runtime.words[0xa000u] == 10u &&
              runtime.words[0xa004u] == 11u &&
              runtime.words[0xa008u] == 12u &&
              runtime.writes == std::vector<std::uint32_t>{
                  0xa000u, 0xa004u, 0xa000u, 0xa004u, 0xa008u},
          "single copy preserves IDA's repeated first two field writes");
    runtime.writes.clear();
    Check(pal4::InventoryRecordMemory::CopyConstruct(
              0u, 0x9000u, runtime) == 0u &&
              runtime.writes.empty(),
          "null destination leaves source untouched");
    Check(pal4::InventoryRecordMemory::CopyRange(
              0x9000u, 0x9018u, 0xb000u, runtime) == 0xb018u &&
              runtime.words[0xb000u] == 10u &&
              runtime.words[0xb00cu] == 20u &&
              pal4::InventoryRecordMemory::CopyRange(
                  0x9000u, 0x9000u, 0xc000u, runtime) == 0xc000u,
          "range construction copies both records and handles empty range");
    Check(pal4::InventoryRecordMemory::FillConstruct(
              0xd000u, 2, 0x9000u, runtime) == 0xd00cu &&
              runtime.words[0xd000u] == 10u &&
              runtime.words[0xd00cu] == 10u &&
              pal4::InventoryRecordMemory::FillConstruct(
                  0xe000u, 0, 0x9000u, runtime) == 0u,
          "repeated construction returns final record address");
}

void CheckAssignments() {
    Runtime runtime;
    Seed(runtime, 0x9000u, 30u);
    Check(pal4::InventoryRecordMemory::FillAssign(
              0xa000u, 0xa024u, 0x9000u, runtime) == 0xa024u &&
              runtime.words[0xa000u] == 30u &&
              runtime.words[0xa00cu] == 30u &&
              runtime.words[0xa018u] == 30u &&
              pal4::InventoryRecordMemory::FillAssign(
                  0xb000u, 0xb000u, 0x9000u, runtime) == 0xb000u,
          "assignment fills three records and preserves empty-range return");
    Seed(runtime, 0xc000u, 40u);
    Seed(runtime, 0xc00cu, 50u);
    Check(pal4::InventoryRecordMemory::CopyBackward(
              0xc000u, 0xc018u, 0xc024u, runtime) == 0xc00cu &&
              runtime.words[0xc00cu] == 40u &&
              runtime.words[0xc010u] == 41u &&
              runtime.words[0xc014u] == 42u &&
              runtime.words[0xc018u] == 50u &&
              runtime.words[0xc01cu] == 51u &&
              runtime.words[0xc020u] == 52u &&
              pal4::InventoryRecordMemory::CopyBackward(
                  0xc000u, 0xc000u, 0xd000u, runtime) == 0xd000u,
          "backward copy supports overlapping right shift and empty range");
}

}  // namespace

int main() {
    try {
        CheckConstructionCopies();
        CheckAssignments();
        std::cout << "inventory_record_memory_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
