#include "pal4/tga_header_record.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Memory final : pal4::ITgaHeaderRecordMemory {
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::pair<std::uint32_t, std::uint8_t>> writes;
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        bytes[address] = value;
        writes.emplace_back(address, value);
    }
};

}  // namespace

int main() {
    Memory memory;
    constexpr std::uint32_t address = 0x8f60b0u;
    memory.bytes[address + 0x4fu] = 0x41u;
    memory.bytes[address + 0x54u] = 0x42u;
    Check(pal4::TgaHeaderRecord::Initialize(address, memory) == address,
          "initializer returns its receiver");
    Check(memory.writes ==
              std::vector<std::pair<std::uint32_t, std::uint8_t>>({
                  {address + 0x50u, 0xc4u}, {address + 0x51u, 0xc4u},
                  {address + 0x52u, 0u}, {address + 0x53u, 0xffu}}),
          "initializer writes four bytes in address order");
    Check(memory.bytes[address + 0x4fu] == 0x41u &&
              memory.bytes[address + 0x54u] == 0x42u,
          "adjacent fields remain unchanged");
    return EXIT_SUCCESS;
}
