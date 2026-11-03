#include "pal4/division_property.h"

#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void Verify(std::uint32_t divisor) {
    pal4::DivisionPropertyState state{{0x11223344u, 0x55667788u},
                                      123.0f, 99u};
    Check(pal4::DivisionProperty::SetDivisor(state, divisor) == divisor,
          "forwards unsigned divisor through EAX");
    Check(state.header[0] == 0x11223344u &&
              state.header[1] == 0x55667788u,
          "preserves unknown header words");
    Check(state.count == 0u, "resets count word at offset twelve");
    const auto expected = divisor == 0u
        ? 0.0f
        : static_cast<float>(1.0 / static_cast<double>(divisor));
    Check(std::bit_cast<std::uint32_t>(state.inverse_divisor) ==
              std::bit_cast<std::uint32_t>(expected),
          "writes exact float reciprocal at offset eight");
}

}  // namespace

int main() {
    Verify(0u);
    Verify(1u);
    Verify(3u);
    Verify(std::numeric_limits<std::uint32_t>::max());
    pal4::DivisionPropertyState state{{0x11223344u, 0x55667788u},
                                      0.5f, 77u};
    Check(pal4::DivisionProperty::SetFlag(state, 0xaau) == 0xaau &&
              state.header[0] == 0x11223344u &&
              state.header[1] == 0x556677aau &&
              state.inverse_divisor == 0.5f && state.count == 77u,
          "flag setter writes only byte four and returns it");
    Check(pal4::DivisionProperty::SetFlag(state, 0u) == 0u &&
              state.header[1] == 0x55667700u,
          "zero flag clears only byte four");
    std::cout << "division property tests passed\n";
}
