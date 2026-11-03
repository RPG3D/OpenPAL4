#include "pal4/effect_system_object_f_color.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IEffectSystemObjectFColorRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;
    std::uint32_t packed = 0u;
    std::uint32_t slot = 0u;
    std::uint8_t result = 0xa5u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        events.push_back("word:" + std::to_string(address));
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        events.push_back("byte:" + std::to_string(address));
        return bytes.at(address);
    }
    std::uint8_t SetColor(std::uint32_t color_slot,
                          std::uint32_t rgba) override {
        events.push_back("set");
        slot = color_slot;
        packed = rgba;
        return result;
    }
};

}  // namespace

int main() {
    Runtime runtime;
    pal4::EffectSystemObjectFState state{};
    state.words[29u] = 0x5000u;
    std::array<std::uint8_t, 4> output{9u, 9u, 9u, 9u};
    Check(pal4::EffectSystemObjectFColor::Get(state, output, runtime) == 0u &&
              pal4::EffectSystemObjectFColor::Set(
                  state, {1u, 2u, 3u, 4u}, runtime) == 0u &&
              output == std::array<std::uint8_t, 4>{9u, 9u, 9u, 9u} &&
              runtime.events.empty(),
          "inactive color methods preserve outputs and skip callbacks");

    state.words[1u] = 0xff000001u;
    runtime.words[0x5030u] = 0x6000u;
    for (std::uint32_t index = 0u; index < 4u; ++index)
        runtime.bytes[0x601cu + index] =
            static_cast<std::uint8_t>(0x10u + index);
    Check(pal4::EffectSystemObjectFColor::Get(
              state, output, runtime) == 0x13u &&
              output == std::array<std::uint8_t, 4>{
                  0x10u, 0x11u, 0x12u, 0x13u} &&
              runtime.events == std::vector<std::string>{
                  "word:20528", "byte:24604", "byte:24605",
                  "byte:24606", "byte:24607"},
          "color getter follows pointer and reads four ordered bytes");
    runtime.events.clear();
    Check(pal4::EffectSystemObjectFColor::Set(
              state, {0x12u, 0x34u, 0x56u, 0x78u}, runtime) ==
              runtime.result &&
              runtime.slot == 0x6000u &&
              runtime.packed == 0x78563412u &&
              runtime.events == std::vector<std::string>{
                  "word:20528", "set"},
          "color setter packs byte inputs and returns helper result");
}
