#include "pal4/clump_object_color.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IClumpObjectColorRuntime {
    std::vector<std::uint32_t> colors;
    std::uintptr_t SetColor(std::uint32_t manager,
                            std::uint32_t rgba) override {
        Check(manager == 0x1000u, "color manager at object +72");
        colors.push_back(rgba);
        return 0xdeadbeefu;
    }
};

void TestColorFacet() {
    pal4::ClumpObjectState state{};
    state.words[17u] = 0x7f302010u;
    state.words[18u] = 0x1000u;
    Runtime runtime;
    std::array<std::uint8_t, 4> output{9u, 8u, 7u, 6u};
    Check(pal4::ClumpObject::GetColor(state, output) == 0u &&
              output == std::array<std::uint8_t, 4>{9u, 8u, 7u, 6u},
          "inactive getter preserves output bytes");
    Check(pal4::ClumpObject::SetColor(state, {1u, 2u, 3u, 255u},
                                      runtime) ==
              reinterpret_cast<std::uintptr_t>(&state) + 36u &&
              state.words[17u] == 0x7f302010u && runtime.colors.empty(),
          "inactive setter returns native facet pointer and preserves color");

    state.words[1u] = 1u;
    Check(pal4::ClumpObject::GetColor(state, output) == 0x7fu &&
              output == std::array<std::uint8_t, 4>{
                  0x10u, 0x20u, 0x30u, 0x7fu},
          "getter copies RGBA bytes and returns alpha");
    Check(pal4::ClumpObject::SetColor(state, {1u, 2u, 3u, 255u},
                                      runtime) == 0xdeadbeefu &&
              state.words[17u] == 0xfe030201u &&
              runtime.colors == std::vector<std::uint32_t>{0xfe030201u},
          "setter clamps alpha 255 to 254 before manager callback");
    Check(pal4::ClumpObject::SetColor(state, {4u, 5u, 6u, 254u},
                                      runtime) == 0xdeadbeefu &&
              state.words[17u] == 0xfe060504u,
          "alpha 254 remains at the native cap");
    Check(pal4::ClumpObject::SetColor(state, {7u, 8u, 9u, 253u},
                                      runtime) == 0xdeadbeefu &&
              state.words[17u] == 0xfd090807u &&
              runtime.colors == std::vector<std::uint32_t>{
                  0xfe030201u, 0xfe060504u, 0xfd090807u},
          "alpha below cap is forwarded without change");
}

}  // namespace

int main() {
    TestColorFacet();
}
