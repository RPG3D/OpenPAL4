#include "pal4/effect_camera_properties.h"

#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

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

struct Runtime final : pal4::IEffectCameraPropertiesRuntime {
    std::vector<std::string> events;

    void CalculateAngles(std::uint32_t matrix,
                         pal4::EffectCameraObjectState& state,
                         std::uint32_t output_offset) override {
        Check(matrix == 0x8010u && output_offset == 88u &&
                  (state.words[11u] & 0xff000000u) == 0u &&
                  state.words[28u] == Bits(4.0f) &&
                  state.words[29u] == Bits(5.0f) &&
                  state.words[30u] == Bits(6.0f),
              "unlock clears flag and caches three angles before conversion");
        events.push_back("calculate-angles");
        state.words[22u] = Bits(10.0f);
        state.words[23u] = Bits(20.0f);
        state.words[24u] = Bits(30.0f);
    }
};

}  // namespace

int main() {
    pal4::EffectCameraObjectState state{};
    Runtime runtime;
    state.words[22u] = Bits(4.0f);
    state.words[23u] = Bits(5.0f);
    state.words[24u] = Bits(6.0f);
    state.words[25u] = Bits(1.0f);
    state.words[26u] = Bits(2.0f);
    state.words[27u] = Bits(3.0f);
    std::array<std::uint32_t, 3> output{0x111u, 0x222u, 0x333u};
    Check(pal4::EffectCameraProperties::GetPosition(state, output, 0xaau) ==
              0u && output[0u] == 0x111u &&
              pal4::EffectCameraProperties::GetRotation(
                  state, output, 0xaau) == 0u &&
              pal4::EffectCameraProperties::SetPosition(
                  state, {0u, 0u, 0u}, 0u) == 0u &&
              pal4::EffectCameraProperties::SetRotation(
                  state, {0u, 0u, 0u}, 0u, runtime) == 0u,
          "inactive camera leaves property fields and output unchanged");
    state.words[1u] = 1u;
    Check(pal4::EffectCameraProperties::GetPosition(state, output, 0xaau) ==
              0u && output[0u] == 0x111u,
          "unready camera leaves output unchanged");
    state.words[20u] = 1u;
    Check(pal4::EffectCameraProperties::GetPosition(state, output, 0xaau) ==
              static_cast<std::uint8_t>(Bits(3.0f)) &&
              output == std::array<std::uint32_t, 3>{
                  Bits(1.0f), Bits(2.0f), Bits(3.0f)} &&
              pal4::EffectCameraProperties::GetRotation(
                  state, output, 0xaau) ==
                  static_cast<std::uint8_t>(Bits(6.0f)) &&
              output == std::array<std::uint32_t, 3>{
                  Bits(4.0f), Bits(5.0f), Bits(6.0f)},
          "normal getters copy the exact three raw fields");
    state.words[11u] = 1u;
    Check(pal4::EffectCameraProperties::GetPosition(state, output, 0xaau) ==
              0xaau && output == std::array<std::uint32_t, 3>{} &&
              pal4::EffectCameraProperties::GetRotation(
                  state, output, 0xbbu) == 0xbbu &&
              output == std::array<std::uint32_t, 3>{},
          "inherit mode returns zero fields and third pointer low byte");
    const std::array<std::uint32_t, 3> inherited{
        0x12345611u, 0x87654322u, 0xabcdef33u};
    Check(pal4::EffectCameraProperties::SetPosition(
              state, inherited, 1u) == 0x22u &&
              state.words[25u] == inherited[0u] &&
              state.words[26u] == inherited[1u] &&
              state.words[27u] == inherited[2u] &&
              pal4::EffectCameraProperties::SetRotation(
                  state, inherited, 1u, runtime) == 1u &&
              state.words[22u] == Bits(4.0f) && runtime.events.empty(),
          "inherit mode forces raw position and blocks rotation setter");
    state.words[11u] = 0u;
    const std::array<std::uint32_t, 3> base_position{
        Bits(1.0f), Bits(2.0f), Bits(3.0f)};
    Check(pal4::EffectCameraProperties::SetPosition(
              state, base_position, 0u) ==
              static_cast<std::uint8_t>(Bits(3.0f)),
          "absolute position writes input words");
    Check(pal4::EffectCameraProperties::SetPosition(
              state, {Bits(4.0f), Bits(5.0f), Bits(6.0f)}, 7u) == 7u &&
              state.words[25u] == Bits(5.0f) &&
              state.words[26u] == Bits(7.0f) &&
              state.words[27u] == Bits(9.0f),
          "relative position adds all three floating-point fields");

    state.words[31u] = 0x8000u;
    state.words[11u] = 0x01000000u;
    const std::array<std::uint32_t, 3> delta{
        Bits(1.0f), Bits(2.0f), Bits(3.0f)};
    Check(pal4::EffectCameraProperties::SetRotation(
              state, delta, 9u, runtime) == 9u &&
              runtime.events ==
                  std::vector<std::string>{"calculate-angles"} &&
              state.words[22u] == Bits(11.0f) &&
              state.words[23u] == Bits(22.0f) &&
              state.words[24u] == Bits(33.0f),
          "locked relative rotation caches, converts and adds angles");
    const std::array<std::uint32_t, 3> absolute{
        0xabcdef11u, 0x12345622u, 0x876543a5u};
    Check(pal4::EffectCameraProperties::SetRotation(
              state, absolute, 0u, runtime) == 0xa5u &&
              runtime.events.size() == 1u &&
              state.words[22u] == absolute[0u] &&
              state.words[23u] == absolute[1u] &&
              state.words[24u] == absolute[2u],
          "unlocked absolute rotation stores raw words without conversion");
}
