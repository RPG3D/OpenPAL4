#include "pal4/effect_property_state.h"

#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace {

void Check(bool good, const char* message) {
    if (!good) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main() {
    pal4::EffectPropertyState state;
    state.words.fill(0xdeadbeefu);
    auto* result = &pal4::EffectPropertyLifecycle::Reset(state);
    Check(result == &state, "reset returns input object");
    for (std::uint32_t index = 0u; index < state.words.size(); ++index) {
        const auto expected = index >= 3u && index <= 5u
                                  ? std::bit_cast<std::uint32_t>(10.0f)
                              : index == 16u || index == 17u || index == 19u
                                  ? 0u : 0xdeadbeefu;
        Check(state.words[index] == expected,
              "reset changes only six native words");
    }
    state.words[16u] = 1u;
    state.words[17u] = 2u;
    state.words[19u] = 3u;
    state.words[3u] = 0x12345678u;
    pal4::EffectPropertyLifecycle::Clear(state);
    Check(state.words[16u] == 0u && state.words[17u] == 0u &&
              state.words[19u] == 0u &&
              state.words[18u] == 0xdeadbeefu &&
              state.words[3u] == 0x12345678u,
          "clear touches only handles and preserves scale and gap");
}
