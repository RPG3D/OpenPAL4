#include "pal4/strip_properties.h"

#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::uint32_t Bits(float value) {
    return std::bit_cast<std::uint32_t>(value);
}

struct Runtime final : pal4::IStripPropertyRuntime {
    pal4::StripObjectState* state = nullptr;
    std::vector<std::string> events;
    pal4::StripTransformKind last_kind = pal4::StripTransformKind::Scale;
    std::array<float, 3> last_values{};
    std::uint32_t UpdateStraight(
        std::uint32_t model, pal4::StripTransformKind kind,
        const std::array<float, 3>& values) override {
        Check(model == 0x3000u, "straight transform model");
        events.emplace_back("straight");
        last_kind = kind;
        last_values = values;
        return 0x12345678u;
    }
    std::uint8_t UpdateFrame(pal4::StripObjectState& value) override {
        Check(&value == state, "cylinder frame owner");
        events.emplace_back("frame");
        return 0x5au;
    }
};

void TestGetters() {
    pal4::StripObjectState state{};
    std::array<std::uint32_t, 3> output{7u, 8u, 9u};
    Check(pal4::StripProperties::GetRotation(state, output) == 0u &&
              output == std::array<std::uint32_t, 3>{7u, 8u, 9u} &&
              pal4::StripProperties::GetScale(state, output) == 0u &&
              pal4::StripProperties::GetPosition(state, output) == 0u,
          "inactive getters preserve caller output");
    state.words[1u] = 1u;
    for (std::uint32_t i = 0u; i < 3u; ++i) {
        state.words[19u + i] = 0x12340010u + i;
        state.words[16u + i] = 0x23450020u + i;
        state.words[22u + i] = 0x34560030u + i;
    }
    Check(pal4::StripProperties::GetRotation(state, output) == 0x12u &&
              output == std::array<std::uint32_t, 3>{
                  0x12340010u, 0x12340011u, 0x12340012u},
          "rotation getter returns raw words and final low byte");
    Check(pal4::StripProperties::GetScale(state, output) == 0x22u &&
              output == std::array<std::uint32_t, 3>{
                  0x23450020u, 0x23450021u, 0x23450022u},
          "scale getter reads native offsets");
    Check(pal4::StripProperties::GetPosition(state, output) == 0x32u &&
              output == std::array<std::uint32_t, 3>{
                  0x34560030u, 0x34560031u, 0x34560032u},
          "position getter reads native offsets");
}

void TestSetters() {
    pal4::StripObjectState state{};
    Runtime runtime;
    runtime.state = &state;
    Check(pal4::StripProperties::SetScale(
              state, {1.0f, 2.0f, 3.0f}, false, runtime) == 0u &&
              runtime.events.empty(), "inactive setter skips writes");
    state.words[1u] = 1u;
    Check(pal4::StripProperties::SetScale(
              state, {1.0f, 2.0f, 3.0f}, false, runtime) == 0x5au &&
              state.words[16u] == Bits(1.0f) &&
              state.words[18u] == Bits(3.0f) &&
              runtime.events == std::vector<std::string>{"frame"},
          "absolute scale writes then rebuilds cylinder frame");
    runtime.events.clear();
    Check(pal4::StripProperties::SetRotation(
              state, {4.0f, 5.0f, 6.0f}, false, runtime) == 0x5au &&
              state.words[19u] == Bits(4.0f) &&
              state.words[21u] == Bits(6.0f) &&
              runtime.events == std::vector<std::string>{"frame"},
          "absolute rotation uses own triple");

    state.words[35u] = 1u;
    state.words[38u] = 0x3000u;
    runtime.events.clear();
    Check(pal4::StripProperties::SetPosition(
              state, {7.0f, 8.0f, 9.0f}, false, runtime) == 0x12345678u &&
              runtime.last_kind == pal4::StripTransformKind::Position &&
              runtime.last_values == std::array{7.0f, 8.0f, 9.0f} &&
              state.words[22u] == Bits(7.0f) &&
              runtime.events == std::vector<std::string>{"straight"},
          "straight position calls model updater");
    Check(pal4::StripProperties::SetPosition(
              state, {1.0f, -2.0f, 3.0f}, true, runtime) == 0x12345678u &&
              runtime.last_values == std::array{8.0f, 6.0f, 12.0f} &&
              state.words[24u] == Bits(12.0f),
          "relative position adds each axis");
    Check(pal4::StripProperties::SetScale(
              state, {1.0f, 1.0f, 1.0f}, true, runtime) == 0x12345678u &&
              runtime.last_kind == pal4::StripTransformKind::Scale &&
              runtime.last_values == std::array{2.0f, 3.0f, 4.0f},
          "straight relative scale selects scale updater");
    Check(pal4::StripProperties::SetRotation(
              state, {1.0f, 1.0f, 1.0f}, true, runtime) == 0x12345678u &&
              runtime.last_kind == pal4::StripTransformKind::Rotation &&
              runtime.last_values == std::array{5.0f, 6.0f, 7.0f},
          "straight relative rotation selects rotation updater");
}

}  // namespace

int main() {
    TestGetters();
    TestSetters();
}
