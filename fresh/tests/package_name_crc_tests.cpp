#include "pal4/package_name_crc.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::uint32_t ReferenceTableEntry(std::uint32_t index) {
    std::uint32_t value = index << 24u;
    for (int bit = 0; bit < 8; ++bit) {
        const bool high = (value & 0x80000000u) != 0u;
        value <<= 1u;
        if (high) value ^= 0x04c11db7u;
    }
    return value;
}

void CheckTableAndPrefix() {
    pal4::PackageNameCrc crc;
    Check(!crc.Initialized() && crc.Compute(
              reinterpret_cast<const std::uint8_t*>("abcde")) ==
              0x6263649au,
          "before manager initialization the zero table remains observable");
    crc.InitializeTable();
    Check(crc.Initialized(), "manager explicitly initializes package CRC table");
    for (std::uint32_t index = 0u; index < 256u; ++index)
        Check(crc.TableEntry(static_cast<std::uint8_t>(index)) ==
                  ReferenceTableEntry(index), "every table entry uses MSB polynomial");
    crc.InitializeTable();
    Check(crc.Compute(reinterpret_cast<const std::uint8_t*>("")) == 0u &&
          crc.Compute(reinterpret_cast<const std::uint8_t*>("a")) ==
              0x61000000u &&
          crc.Compute(reinterpret_cast<const std::uint8_t*>("abc")) ==
              0x61626300u &&
          crc.Compute(reinterpret_cast<const std::uint8_t*>("abcd")) ==
              0x61626364u,
          "empty and short names use big-endian four-byte prefix");
}

void CheckRemainingBytes() {
    pal4::PackageNameCrc crc;
    crc.InitializeTable();
    constexpr std::array<std::uint8_t, 10> name{
        'm', 'u', 's', 'i', 'c', '.', 's', 'm', 'p', 0u};
    std::uint32_t value = ~0x6d757369u;
    for (std::size_t index = 4u; index < name.size() - 1u; ++index)
        value = ReferenceTableEntry(value >> 24u) ^
                ((value << 8u) | name[index]);
    Check(crc.Compute(name.data()) == ~value,
          "remaining filename bytes use the initialized table");
    Check(crc.Compute(reinterpret_cast<const std::uint8_t*>("ABCD")) !=
          crc.Compute(reinterpret_cast<const std::uint8_t*>("abcd")),
          "lowercasing belongs to Package_GetFileIndex caller");
}

}  // namespace

int main() {
    CheckTableAndPrefix();
    CheckRemainingBytes();
    std::cout << "2 package name CRC test cases passed\n";
}
