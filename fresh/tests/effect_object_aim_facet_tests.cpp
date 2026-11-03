#include "pal4/effect_object_aim_facet.h"

#include <array>
#include <bit>
#include <cmath>
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

struct Runtime final : pal4::IEffectObjectAimFacetRuntime {
    std::uint32_t calls = 0u;
    std::uint32_t set_calls = 0u;
    std::uint8_t GetPosition(
        const pal4::EffectObjectState& state,
        std::array<std::uint32_t, 3u>& output) override {
        Check((state.words[1u] & 0xffu) != 0u,
              "getter dispatches only while active");
        ++calls;
        output = {4u, 5u, 6u};
        return 0x7bu;
    }
    std::uint8_t SetPosition(
        pal4::EffectObjectState& state,
        const std::array<std::uint32_t, 3u>& values,
        const std::uint32_t mode) override {
        Check((state.words[1u] & 0xffu) != 0u &&
                  values == std::array<std::uint32_t, 3u>{4u, 5u, 6u} &&
                  mode == 7u,
              "position setter forwarding arguments");
        ++set_calls;
        return 0x5au;
    }
};

void TestAimFacet() {
    pal4::EffectObjectState state{};
    state.words.fill(0xaaaaaaaau);
    state.words[1u] &= 0xffffff00u;
    Runtime runtime;
    std::array<std::uint32_t, 3u> output{1u, 2u, 3u};
    const std::array<std::uint32_t, 3u> target{
        0x11111111u, 0x222222abu, 0x33333333u};
    const std::array<std::uint32_t, 3u> rotation{
        0x44444444u, 0x55555555u, 0x666666cdu};
    Check(pal4::EffectObjectAimFacet::GetPosition(
              state, output, runtime) == 0u &&
              output == std::array<std::uint32_t, 3u>{1u, 2u, 3u} &&
              pal4::EffectObjectAimFacet::SetTarget(state, target) == 0u &&
              pal4::EffectObjectAimFacet::SetRotationOffset(
                  state, rotation) == 0u &&
              pal4::EffectObjectAimFacet::SetPositionWithMode(
                  state, {4u, 5u, 6u}, 7u, runtime) == 0u &&
              runtime.calls == 0u && state.words[26u] == 0xaaaaaaaau &&
              state.words[29u] == 0xaaaaaaaau,
          "inactive aim facet preserves output and cached words");

    state.words[1u] |= 1u;
    Check(pal4::EffectObjectAimFacet::GetPosition(
              state, output, runtime) == 0x7bu &&
              output == std::array<std::uint32_t, 3u>{4u, 5u, 6u} &&
              runtime.calls == 1u,
          "position getter forwards through owner +28 virtual slot +4");
    Check(pal4::EffectObjectAimFacet::SetTarget(state, target) == 0xabu &&
              pal4::EffectObjectAimFacet::SetRotationOffset(
                  state, rotation) == 0xcdu,
          "setters preserve native low-byte return sources");
    Check(pal4::EffectObjectAimFacet::SetPositionWithMode(
              state, {4u, 5u, 6u}, 7u, runtime) == 0x5au &&
              runtime.set_calls == 1u,
          "active position setter forwards through owner +28 slot +8");
    for (std::uint32_t i = 0u; i != 3u; ++i) {
        Check(state.words[26u + i] == target[i] &&
                  state.words[29u + i] == rotation[i],
              "target and rotation cache offsets are separate");
    }
}

struct FrameRuntime final : pal4::IEffectObjectAimRuntime {
    std::vector<std::string> calls;
    pal4::EffectObjectAimMatrix matrix{};
    std::array<std::uint32_t, 3u> scale{};
    std::array<std::uint32_t, 3u> position{};
    std::array<std::uint32_t, 3u> offset{};
    std::array<float, 3u> angles{};

    void GetPosition(const pal4::EffectObjectState&,
                     std::array<std::uint32_t, 3u>& output) override {
        calls.emplace_back("position");
        output = {0u, 0u, 0u};
    }
    void Normalize(std::array<float, 3u>& vector) override {
        calls.emplace_back("normalize");
        const auto length = std::sqrt(vector[0u] * vector[0u] +
                                      vector[1u] * vector[1u] +
                                      vector[2u] * vector[2u]);
        for (auto& value : vector)
            value /= length;
    }
    void UpdateMatrix(pal4::EffectObjectAimMatrix& input) override {
        calls.emplace_back("matrix_update");
        matrix = input;
    }
    std::uint32_t GetFrame(const pal4::EffectObjectState&) override {
        calls.emplace_back("frame");
        return 0x4000u;
    }
    void SetFrameIdentity(const std::uint32_t frame) override {
        Check(frame == 0x4000u, "aim identity frame");
        calls.emplace_back("identity");
    }
    void ScaleFrame(const std::uint32_t frame,
                    const std::array<std::uint32_t, 3u>& input,
                    const std::uint32_t mode) override {
        Check(frame == 0x4000u && mode == 0u, "aim scale mode");
        calls.emplace_back("scale");
        scale = input;
    }
    void ApplyMatrix(const std::uint32_t frame,
                     const pal4::EffectObjectAimMatrix& input,
                     const std::uint32_t mode) override {
        Check(frame == 0x4000u && mode == 2u &&
                  input.flags == 0x20003u,
              "aim matrix application");
        calls.emplace_back("apply");
    }
    void TranslateFrame(const std::uint32_t frame,
                        const std::array<std::uint32_t, 3u>& input,
                        const std::uint32_t mode) override {
        Check(frame == 0x4000u && mode == 2u, "aim translation mode");
        calls.emplace_back("translate");
        position = input;
    }
    void UpdateFrameRotation(
        const std::uint32_t frame,
        const std::array<std::uint32_t, 3u>& input,
        std::array<float, 3u>& output) override {
        Check(frame == 0x4000u, "aim rotation helper frame");
        calls.emplace_back("rotation_update");
        offset = input;
        output = {11.0f, 22.0f, 33.0f};
    }
    std::uint8_t SetRotation(pal4::EffectObjectState&,
                             const std::array<float, 3u>& input,
                             const std::uint32_t mode) override {
        Check(mode == 0u, "aim rotation facet mode");
        calls.emplace_back("rotation");
        angles = input;
        return 0xa7u;
    }
};

void TestFrameUpdate() {
    pal4::EffectObjectState state{};
    state.words[32u] = 0x12345678u;
    FrameRuntime runtime;
    Check(pal4::EffectObjectAimFacet::UpdateFrame(
              state, 1u, runtime) == 0u &&
              state.words[32u] == 0x12345678u && runtime.calls.empty(),
          "inactive aim update preserves mode and skips native calls");
    state.words[1u] = 1u;
    Check(pal4::EffectObjectAimFacet::UpdateFrame(
              state, 0u, runtime) == 0u &&
              state.words[32u] == 0x12345600u && runtime.calls.empty(),
          "disabled aim update only writes mode byte");

    state.words[13u] = std::bit_cast<std::uint32_t>(2.0f);
    state.words[14u] = std::bit_cast<std::uint32_t>(3.0f);
    state.words[15u] = std::bit_cast<std::uint32_t>(4.0f);
    state.words[26u] = 0u;
    state.words[27u] = 0u;
    state.words[28u] = std::bit_cast<std::uint32_t>(10.0f);
    state.words[29u] = std::bit_cast<std::uint32_t>(5.0f);
    state.words[30u] = std::bit_cast<std::uint32_t>(6.0f);
    state.words[31u] = std::bit_cast<std::uint32_t>(7.0f);
    Check(pal4::EffectObjectAimFacet::UpdateFrame(
              state, 1u, runtime) == 0xa7u &&
              (state.words[32u] & 0xffu) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "position", "normalize", "normalize", "normalize",
                  "matrix_update", "frame", "identity", "scale", "apply",
                  "translate", "rotation_update", "rotation"},
          "enabled aim frame callback order and result");
    Check(runtime.matrix.right ==
              std::array<float, 3u>{1.0f, 0.0f, 0.0f} &&
              runtime.matrix.up ==
                  std::array<float, 3u>{0.0f, 1.0f, 0.0f} &&
              runtime.matrix.at ==
                  std::array<float, 3u>{0.0f, 0.0f, 1.0f} &&
              runtime.matrix.flags == 0x20003u &&
              runtime.scale == std::array<std::uint32_t, 3u>{
                  state.words[13u], state.words[14u], state.words[15u]} &&
              runtime.position ==
                  std::array<std::uint32_t, 3u>{0u, 0u, 0u} &&
              runtime.offset == std::array<std::uint32_t, 3u>{
                  state.words[29u], state.words[30u], state.words[31u]} &&
              runtime.angles ==
                  std::array<float, 3u>{11.0f, 22.0f, 33.0f},
          "aim matrix basis and owner offsets");
}

}  // namespace

int main() {
    TestAimFacet();
    TestFrameUpdate();
}
