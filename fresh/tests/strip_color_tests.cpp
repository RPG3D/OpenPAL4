#include "pal4/strip_color.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IStripColorRuntime {
    std::vector<std::uint32_t> colors;
    std::uint8_t SetClumpMaterialColor(
        std::uint32_t clump, std::uint32_t rgba) override {
        Check(clump == 0x3000u, "material update clump handle");
        colors.push_back(rgba);
        return 0x5au;
    }
};

void TestColor() {
    pal4::StripObjectState state{};
    Runtime runtime;
    std::array<std::uint8_t, 4> output{7u, 8u, 9u, 10u};
    Check(pal4::StripColor::Get(state, output) == 0u &&
              output == std::array<std::uint8_t, 4>{7u, 8u, 9u, 10u} &&
              pal4::StripColor::Set(
                  state, {1u, 2u, 3u, 4u}, runtime) == 0u &&
              runtime.colors.empty(),
          "inactive color facet preserves output and state");
    state.words[1u] = 1u;
    state.words[34u] = 0x3000u;
    state.words[25u] = 0x44332211u;
    Check(pal4::StripColor::Get(state, output) == 0x44u &&
              output == std::array<std::uint8_t, 4>{
                  0x11u, 0x22u, 0x33u, 0x44u},
          "getter returns native RGBA bytes and alpha");
    Check(pal4::StripColor::Set(
              state, {0x11u, 0x22u, 0x33u, 0xffu}, runtime) == 0x5au &&
              state.words[25u] == 0xfe332211u &&
              runtime.colors == std::vector<std::uint32_t>{0xfe332211u},
          "full alpha clamps before material callback");
    Check(pal4::StripColor::Set(
              state, {1u, 2u, 3u, 0u}, runtime) == 0x5au &&
              state.words[25u] == 0x00030201u &&
              runtime.colors.back() == 0x00030201u,
          "other alpha values preserve native byte packing");
}

}  // namespace

int main() {
    TestColor();
}
