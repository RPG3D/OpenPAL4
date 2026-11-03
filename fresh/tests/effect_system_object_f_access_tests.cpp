#include "pal4/effect_system_object_f_access.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IEffectSystemObjectFAccessRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> reads;

    std::uint32_t ReadWord(std::uint32_t address) override {
        reads.push_back(address);
        return words.at(address);
    }
};

void TestAccessors() {
    Runtime runtime;
    runtime.words = {{0x5018u, 0x12345678u},
                     {0x501cu, 0x11111122u},
                     {0x5020u, 0x22222233u},
                     {0x5024u, 0x33333344u},
                     {0x5028u, 0x3f800055u},
                     {0x502cu, 0x40000066u}};
    pal4::EffectSystemObjectFState state{};
    state.words[29u] = 0x5000u;
    std::array<std::uint32_t, 3> output{9u, 8u, 7u};
    Check(pal4::EffectSystemObjectFAccess::GetScale(
              state, output, runtime) == 0u &&
              pal4::EffectSystemObjectFAccess::GetRotation(
                  state, output, 0xabu, runtime) == 0u &&
              pal4::EffectSystemObjectFAccess::GetPosition(
                  state, output, runtime) == 0u &&
              output == std::array<std::uint32_t, 3>{9u, 8u, 7u} &&
              runtime.reads.empty(),
          "inactive accessors leave all outputs untouched");

    state.words[1u] = 0xdeadbe01u;
    Check(pal4::EffectSystemObjectFAccess::GetScale(
              state, output, runtime) == 0x66u &&
              output == std::array<std::uint32_t, 3>{
                  0x3f800055u, 0x40000066u, 0u} &&
              runtime.reads == std::vector<std::uint32_t>{
                  0x5028u, 0x502cu},
          "scale reads two words and zeros third output");
    runtime.reads.clear();
    Check(pal4::EffectSystemObjectFAccess::GetRotation(
              state, output, 0xabu, runtime) == 0xabu &&
              output == std::array<std::uint32_t, 3>{
                  0x12345678u, 0u, 0u} &&
              runtime.reads == std::vector<std::uint32_t>{0x5018u},
          "rotation exposes native pointer-derived low return byte");
    runtime.reads.clear();
    Check(pal4::EffectSystemObjectFAccess::GetPosition(
              state, output, runtime) == 0x44u &&
              output == std::array<std::uint32_t, 3>{
                  0x11111122u, 0x22222233u, 0x33333344u} &&
              runtime.reads == std::vector<std::uint32_t>{
                  0x501cu, 0x5020u, 0x5024u},
          "position returns three contiguous words and last low byte");
}

}  // namespace

int main() {
    TestAccessors();
}
