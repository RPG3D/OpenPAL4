#include "pal4/thunder_properties.h"

#include <array>
#include <bit>
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

std::uint32_t Bits(const float value) {
    return std::bit_cast<std::uint32_t>(value);
}

struct Runtime final : pal4::IThunderPropertyRuntime {
    std::uint32_t updates = 0u;
    std::uint32_t UpdateFrame(pal4::ThunderObjectState&) override {
        ++updates;
        return 0x12345678u;
    }
};

void TestGetters() {
    pal4::ThunderObjectState state{};
    std::array<std::uint32_t, 3u> output{7u, 8u, 9u};
    Check(pal4::ThunderProperties::GetScale(state, output) == 0u &&
              pal4::ThunderProperties::GetRotation(state, output) == 0u &&
              pal4::ThunderProperties::GetPosition(state, output) == 0u &&
              output == std::array<std::uint32_t, 3u>{7u, 8u, 9u},
          "inactive getters preserve outputs");
    state.words[1u] = 2u;
    state.words[18u] = 0x11u;
    state.words[19u] = 0x22u;
    state.words[20u] = 0x33u;
    state.words[21u] = 0x44u;
    state.words[22u] = 0x55u;
    state.words[23u] = 0x66u;
    state.words[24u] = 0x77u;
    state.words[25u] = 0x88u;
    state.words[26u] = 0x99u;
    Check(pal4::ThunderProperties::GetScale(state, output) == 0x33u &&
              output == std::array<std::uint32_t, 3u>{
                  0x11u, 0x22u, 0x33u},
          "scale getter reads +72 and returns last low byte");
    Check(pal4::ThunderProperties::GetRotation(state, output) == 0x66u &&
              output == std::array<std::uint32_t, 3u>{
                  0x44u, 0x55u, 0x66u},
          "rotation getter reads +84");
    Check(pal4::ThunderProperties::GetPosition(state, output) == 0x99u &&
              output == std::array<std::uint32_t, 3u>{
                  0x77u, 0x88u, 0x99u},
          "position getter reads +96");
}

void TestSetters() {
    pal4::ThunderObjectState state{};
    Runtime runtime;
    Check(pal4::ThunderProperties::SetScale(state, {1, 2, 3}, false,
                                            runtime) == 0u &&
              runtime.updates == 0u,
          "inactive setter does not update frame");
    state.words[1u] = 1u;
    Check(pal4::ThunderProperties::SetScale(state, {1, 2, 3}, false,
                                            runtime) == 0x12345678u &&
              state.words[18u] == Bits(1.0f) &&
              state.words[20u] == Bits(3.0f),
          "absolute scale writes +72..+80");
    Check(pal4::ThunderProperties::SetScale(state, {2, 3, 4}, true,
                                            runtime) == 0x12345678u &&
              state.words[18u] == Bits(3.0f) &&
              state.words[19u] == Bits(5.0f) &&
              state.words[20u] == Bits(7.0f),
          "relative scale adds components");
    Check(pal4::ThunderProperties::SetRotation(state, {4, 5, 6}, false,
                                               runtime) == 0x12345678u &&
              state.words[21u] == Bits(4.0f) &&
              state.words[23u] == Bits(6.0f),
          "absolute rotation writes +84..+92");
    Check(pal4::ThunderProperties::SetRotation(state, {1, 2, 3}, true,
                                               runtime) == 0x12345678u &&
              state.words[21u] == Bits(5.0f) &&
              state.words[22u] == Bits(7.0f) &&
              state.words[23u] == Bits(9.0f),
          "relative rotation adds components");
    Check(pal4::ThunderProperties::SetPosition(state, {7, 8, 9}, false,
                                               runtime) == 0x12345678u &&
              state.words[24u] == Bits(7.0f) &&
              state.words[26u] == Bits(9.0f),
          "absolute position writes +96..+104");
    Check(pal4::ThunderProperties::SetPosition(state, {3, 2, 1}, true,
                                               runtime) == 0x12345678u &&
              state.words[24u] == Bits(10.0f) &&
              state.words[25u] == Bits(10.0f) &&
              state.words[26u] == Bits(10.0f) &&
              runtime.updates == 6u,
          "relative position adds components and updates frame");
}

}  // namespace

int main() {
    TestGetters();
    TestSetters();
}
