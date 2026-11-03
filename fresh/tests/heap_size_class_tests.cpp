#include "pal4/heap_size_class.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace {

struct Case {
    std::uint32_t size;
    std::uint32_t expected;
};

void CheckObservedBins() {
    constexpr std::array<Case, 18> cases{{
        {0u, 28u},       {31u, 28u},       {32u, 29u},
        {255u, 31u},     {256u, 32u},      {319u, 32u},
        {320u, 33u},     {511u, 35u},      {512u, 36u},
        {767u, 37u},     {768u, 38u},      {1023u, 39u},
        {1024u, 40u},    {2048u, 44u},     {4096u, 48u},
        {65536u, 64u},   {0xFFFFFFu, 95u}, {0x1000000u, 95u},
    }};
    for (const Case& item : cases) {
        if (pal4::HeapSizeClass::Index(item.size) != item.expected) {
            std::cerr << "FAIL: size class for " << item.size << '\n';
            std::exit(EXIT_FAILURE);
        }
    }
}

void CheckSaturation() {
    if (pal4::HeapSizeClass::Index(0xffffffffu) != 0x5fu) {
        std::cerr << "FAIL: maximum block size saturates\n";
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main() {
    CheckObservedBins();
    CheckSaturation();
    std::cout << "2 heap size class test cases passed\n";
}
