#include "pal4/memory_allocator_function_table.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>

int main() {
    pal4::MemoryAllocatorFunctionTable table;
    if (&table.Initialize() != &table ||
        table.Entries() != std::array<std::uint32_t, 4>{
            0x408140u, 0x5fc6c0u, 0x408150u, 0x408170u}) {
        std::cerr << "allocator callback addresses or table identity differ\n";
        return EXIT_FAILURE;
    }
    std::cout << "allocator callback table test passed\n";
}
