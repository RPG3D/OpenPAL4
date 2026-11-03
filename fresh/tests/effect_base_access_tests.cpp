#include "pal4/effect_base_access.h"

#include <bit>
#include <cstdlib>
#include <iostream>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main() {
    pal4::EffectSystemObjectFState state{};
    state.words[1u] = 0x11223344u;
    state.words[2u] = 0x55667788u;
    state.words[4u] = 0xabcdef12u;
    Check(pal4::EffectBaseAccess::Byte7(state) == 0x11u &&
              pal4::EffectBaseAccess::Byte8(state) == 0x88u &&
              pal4::EffectBaseAccess::Byte9(state) == 0x77u &&
              pal4::EffectBaseAccess::Byte10(state) == 0x66u &&
              pal4::EffectBaseAccess::Byte11(state) == 0x55u,
          "five base accessors read successive native bytes");
    Check(pal4::EffectBaseAccess::DefaultFactor() ==
              static_cast<double>(std::bit_cast<float>(0x3f4ccccdu)) &&
              pal4::EffectBaseAccess::System(state) == 0xabcdef12u,
          "factor preserves x87 float source and system word offset");
}
