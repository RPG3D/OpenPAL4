#include "pal4/strip_aim.h"
#include "pal4/strip_properties.h"

#include <array>
#include <bit>
#include <cmath>
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

struct Runtime final : pal4::IStripAimRuntime {
    pal4::StripObjectState* state = nullptr;
    std::vector<std::string> events;
    pal4::StripAimMatrix matrix{};
    std::array<std::uint32_t, 3> scale{};
    std::array<std::uint32_t, 3> position{};
    std::array<std::uint32_t, 3> offset{};
    std::array<float, 3> angles{};

    void ReadStraightPosition(
        std::uint32_t model, std::array<float, 3>& output) override {
        Check(model == 0x3000u, "straight model position source");
        events.emplace_back("model-position");
        output = {0.0f, 0.0f, 0.0f};
    }
    void Normalize(std::array<float, 3>& vector) override {
        events.emplace_back("normalize");
        const auto length = std::sqrt(vector[0u] * vector[0u] +
                                      vector[1u] * vector[1u] +
                                      vector[2u] * vector[2u]);
        for (auto& value : vector)
            value /= length;
    }
    void UpdateMatrix(pal4::StripAimMatrix& input) override {
        events.emplace_back("matrix");
        matrix = input;
    }
    std::uint32_t GetFrame(pal4::StripObjectState& value) override {
        Check(&value == state, "frame owner");
        events.emplace_back("frame");
        return 0x4000u;
    }
    void SetFrameIdentity(std::uint32_t frame) override {
        Check(frame == 0x4000u, "identity frame");
        events.emplace_back("identity");
    }
    void ScaleFrame(std::uint32_t frame,
                    const std::array<std::uint32_t, 3>& input,
                    std::uint32_t mode) override {
        Check(frame == 0x4000u && mode == 0u, "scale mode");
        events.emplace_back("scale");
        scale = input;
    }
    void ApplyMatrix(std::uint32_t frame,
                     const pal4::StripAimMatrix& input,
                     std::uint32_t mode) override {
        Check(frame == 0x4000u && mode == 2u &&
                  input.flags == 0x20003u,
              "aim matrix application");
        events.emplace_back("apply");
    }
    void TranslateFrame(
        std::uint32_t frame,
        const std::array<std::uint32_t, 3>& input,
        std::uint32_t mode) override {
        Check(frame == 0x4000u && mode == 2u, "translation mode");
        events.emplace_back("translate");
        position = input;
    }
    void UpdateFrameRotation(
        std::uint32_t frame,
        const std::array<std::uint32_t, 3>& input,
        std::array<float, 3>& output) override {
        Check(frame == 0x4000u, "rotation helper frame");
        events.emplace_back("rotation-update");
        offset = input;
        output = {11.0f, 22.0f, 33.0f};
    }
    std::uint32_t SetRotation(
        pal4::StripObjectState& value,
        const std::array<float, 3>& input,
        std::uint32_t mode) override {
        Check(&value == state && mode == 0u,
              "rotation facet owner and mode");
        events.emplace_back("rotation");
        angles = input;
        return 0x12345678u;
    }
};

void TestAccessors() {
    pal4::StripObjectState state{};
    std::array<std::uint32_t, 3> output{1u, 2u, 3u};
    const std::array<std::uint32_t, 3> target{
        0x11111111u, 0x222222abu, 0x33333333u};
    const std::array<std::uint32_t, 3> offset{
        0x44444444u, 0x55555555u, 0x666666cdu};
    Check(pal4::StripAim::GetPosition(state, output) == 0u &&
              output == std::array<std::uint32_t, 3>{1u, 2u, 3u} &&
              pal4::StripAim::SetTarget(state, target) == 0u &&
              pal4::StripAim::SetRotationOffset(state, offset) == 0u,
          "inactive aim facet preserves caller and owner words");
    state.words[1u] = 1u;
    state.words[22u] = 0x12340001u;
    state.words[23u] = 0x12340002u;
    state.words[24u] = 0x123400f3u;
    Check(pal4::StripAim::GetPosition(state, output) == 0xf3u &&
              output == std::array<std::uint32_t, 3>{
                  0x12340001u, 0x12340002u, 0x123400f3u} &&
              pal4::StripAim::SetTarget(state, target) == 0xabu &&
              pal4::StripAim::SetRotationOffset(state, offset) == 0xcdu,
          "raw position and native low-byte setter returns");
    for (std::uint32_t i = 0u; i < 3u; ++i)
        Check(state.words[26u + i] == target[i] &&
                  state.words[29u + i] == offset[i],
              "target and rotation offset occupy separate triples");
}

struct PositionRuntime final : pal4::IStripPropertyRuntime {
    std::uint32_t calls = 0u;
    std::uint32_t UpdateStraight(
        std::uint32_t model, pal4::StripTransformKind kind,
        const std::array<float, 3>& values) override {
        Check(model == 0x3000u &&
                  kind == pal4::StripTransformKind::Position &&
                  values == std::array{2.0f, 3.0f, 4.0f},
              "aim position delegates straight model update");
        ++calls;
        return 0x12345678u;
    }
    std::uint8_t UpdateFrame(pal4::StripObjectState&) override {
        Check(false, "straight aim position must not rebuild frame");
        return 0u;
    }
};

void TestPositionSetter() {
    pal4::StripObjectState state{};
    PositionRuntime runtime;
    Check(pal4::StripAim::SetPosition(
              state, {2.0f, 3.0f, 4.0f}, false, runtime) == 0u &&
              runtime.calls == 0u,
          "inactive aim position setter is gated");
    state.words[1u] = 1u;
    state.words[35u] = 1u;
    state.words[38u] = 0x3000u;
    Check(pal4::StripAim::SetPosition(
              state, {2.0f, 3.0f, 4.0f}, false, runtime) ==
              0x12345678u && state.words[22u] == Bits(2.0f) &&
              state.words[24u] == Bits(4.0f) && runtime.calls == 1u,
          "aim position setter uses shared native transform path");
}

void TestFrame(bool straight) {
    pal4::StripObjectState state{};
    state.words[32u] = 0x12345678u;
    Runtime runtime;
    runtime.state = &state;
    Check(pal4::StripAim::UpdateFrame(state, 1u, runtime) == 0u &&
              state.words[32u] == 0x12345678u && runtime.events.empty(),
          "inactive aim update preserves state");
    state.words[1u] = 1u;
    Check(pal4::StripAim::UpdateFrame(state, 0u, runtime) == 0u &&
              state.words[32u] == 0x12345600u && runtime.events.empty(),
          "disabled aim update only clears mode byte");

    state.words[16u] = Bits(2.0f);
    state.words[17u] = Bits(3.0f);
    state.words[18u] = Bits(4.0f);
    state.words[22u] = 0u;
    state.words[23u] = 0u;
    state.words[24u] = 0u;
    state.words[26u] = 0u;
    state.words[27u] = 0u;
    state.words[28u] = Bits(10.0f);
    state.words[29u] = Bits(5.0f);
    state.words[30u] = Bits(6.0f);
    state.words[31u] = Bits(7.0f);
    state.words[35u] = straight ? 1u : 0u;
    state.words[38u] = 0x3000u;
    Check(pal4::StripAim::UpdateFrame(state, 1u, runtime) == 0x12345678u &&
              (state.words[32u] & 0xffu) == 1u,
          "enabled aim callback result and mode byte");
    const std::vector<std::string> expected = straight
        ? std::vector<std::string>{
              "model-position", "normalize", "normalize", "normalize",
              "matrix", "frame", "identity", "scale", "apply",
              "translate", "rotation-update", "rotation"}
        : std::vector<std::string>{
              "normalize", "normalize", "normalize", "matrix", "frame",
              "identity", "scale", "apply", "translate",
              "rotation-update", "rotation"};
    Check(runtime.events == expected,
          "straight/cylinder position source and frame callback order");
    Check(runtime.matrix.right == std::array{1.0f, 0.0f, 0.0f} &&
              runtime.matrix.up == std::array{0.0f, 1.0f, 0.0f} &&
              runtime.matrix.at == std::array{0.0f, 0.0f, 1.0f} &&
              runtime.matrix.flags == 0x20003u &&
              runtime.scale == std::array{
                  Bits(2.0f), Bits(3.0f), Bits(4.0f)} &&
              runtime.position == std::array<std::uint32_t, 3>{0u, 0u, 0u} &&
              runtime.offset == std::array{
                  Bits(5.0f), Bits(6.0f), Bits(7.0f)} &&
              runtime.angles == std::array{11.0f, 22.0f, 33.0f},
          "normalized aim basis and native transform arguments");
}

}  // namespace

int main() {
    TestAccessors();
    TestPositionSetter();
    TestFrame(false);
    TestFrame(true);
}
