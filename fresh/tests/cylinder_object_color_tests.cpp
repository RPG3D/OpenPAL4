#include "pal4/cylinder_object_color.h"

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

struct Runtime final : pal4::ICylinderObjectColorRuntime {
    std::vector<std::uint32_t> reads;
    std::uint32_t model = 0u;
    std::array<std::uint8_t, 4> color{};
    std::uint32_t set_count = 0u;
    std::uint8_t ReadByte(std::uint32_t address) override {
        reads.push_back(address);
        return static_cast<std::uint8_t>(address);
    }
    std::uint8_t SetColor(std::uint32_t value,
                          const std::array<std::uint8_t, 4>& input) override {
        model = value;
        color = input;
        ++set_count;
        return 0xa5u;
    }
};

void TestColor() {
    pal4::CylinderObjectState state{};
    state.words[22u] = 0x2080u;
    std::array<std::uint8_t, 4> output{1u, 2u, 3u, 4u};
    const std::array<std::uint8_t, 4> input{0u, 127u, 254u, 255u};
    Runtime runtime;
    using pal4::CylinderObjectColor;
    Check(CylinderObjectColor::Get(state, output, runtime) == 0u &&
              output == std::array<std::uint8_t, 4>{1u, 2u, 3u, 4u} &&
              runtime.reads.empty() &&
              CylinderObjectColor::Set(state, input, runtime) == 0u &&
              runtime.set_count == 0u,
          "inactive color facet preserves output and skips native calls");
    state.words[1u] = 0x01000001u;
    Check(CylinderObjectColor::Get(state, output, runtime) == 0xb3u &&
              output == std::array<std::uint8_t, 4>{
                  0xb0u, 0xb1u, 0xb2u, 0xb3u} &&
              runtime.reads == std::vector<std::uint32_t>{
                  0x20b0u, 0x20b1u, 0x20b2u, 0x20b3u},
          "active getter reads four model color bytes and returns alpha");
    Check(CylinderObjectColor::Set(state, input, runtime) == 0xa5u &&
              runtime.set_count == 1u && runtime.model == 0x2080u &&
              runtime.color == input,
          "setter forwards all four raw bytes, including alpha 255");
}

}  // namespace

int main() {
    TestColor();
}
