#include "pal4/thunder_visual_storage.h"

#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {

void Check(bool good, const char* message) {
    if (!good) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void Float(pal4::ThunderObjectState& state,
           std::uint32_t offset, float value) {
    state.words[offset / 4u] = std::bit_cast<std::uint32_t>(value);
}

}  // namespace

int main() {
    pal4::ThunderObjectState state;
    state.words[0u] = 0xdeadbeefu;
    Float(state, 140u, 1.0f);
    Float(state, 144u, 0.5f);
    Check(pal4::ThunderVisualStorage::AdvanceTimer(state, 0.25) == 0u &&
              std::bit_cast<float>(state.words[36u]) == 0.5f,
          "inactive timer leaves stored value unchanged");
    state.words[15u] = 1u;
    Check(pal4::ThunderVisualStorage::AdvanceTimer(state, 0.25) == 0x100u &&
              std::bit_cast<float>(state.words[36u]) == 0.75f,
          "below threshold accumulates and returns x87 mask");
    Check(pal4::ThunderVisualStorage::AdvanceTimer(state, 0.25) == 0u &&
              state.words[36u] == 0u,
          "exact threshold resets timer");
    Check(pal4::ThunderVisualStorage::AdvanceTimer(state, 1.5) == 0u &&
              state.words[36u] == 0u,
          "overshoot resets without carrying remainder");

    Float(state, 140u, 0.0f);
    Float(state, 144u, 0.0f);
    Check(pal4::ThunderVisualStorage::AdvanceTimer(
              state, std::numeric_limits<double>::denorm_min()) == 0u &&
              state.words[36u] == 0u,
          "unrounded positive elapsed reaches zero interval");
    Float(state, 140u, 1.0f);
    Float(state, 144u, 1.0f);
    Check(pal4::ThunderVisualStorage::AdvanceTimer(
              state, -std::numeric_limits<double>::epsilon()) == 0x100u &&
              std::bit_cast<float>(state.words[36u]) == 1.0f,
          "unrounded elapsed below threshold despite float store at one");
    Check(pal4::ThunderVisualStorage::AdvanceTimer(
              state, std::numeric_limits<double>::quiet_NaN()) == 0x100u,
          "unordered x87 comparison retains timer");
    Check(state.words[0u] == 0xdeadbeefu,
          "outer Thunder object header is untouched");
}
