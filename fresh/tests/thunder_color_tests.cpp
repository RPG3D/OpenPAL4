#include "pal4/thunder_color.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main() {
    pal4::ThunderObjectState state{};
    state.words[27u] = 0x44332211u;
    std::array<std::uint8_t, 4u> output{9u, 8u, 7u, 6u};
    Check(pal4::ThunderColor::Get(state, output) == 0u &&
              output == std::array<std::uint8_t, 4u>{9u, 8u, 7u, 6u},
          "inactive getter preserves output");
    Check(pal4::ThunderColor::Set(state, {1u, 2u, 3u, 4u}) == 0u &&
              state.words[27u] == 0x44332211u,
          "inactive setter preserves packed color");
    state.words[1u] = 1u;
    Check(pal4::ThunderColor::Get(state, output) == 0x44u &&
              output == std::array<std::uint8_t, 4u>{
                  0x11u, 0x22u, 0x33u, 0x44u},
          "getter reads RGBA in native byte order and returns alpha");
    Check(pal4::ThunderColor::Set(state, {7u, 8u, 9u, 10u}) == 7u &&
              state.words[27u] == 0x0a090807u,
          "setter packs RGBA at +108 and returns red");
}
