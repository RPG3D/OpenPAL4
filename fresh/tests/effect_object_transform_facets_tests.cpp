#include "pal4/effect_object_transform_facets.h"

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

struct Runtime final : pal4::IEffectObjectTransformFacetRuntime {
    std::uint32_t calls = 0u;
    std::uint32_t UpdateFrame(pal4::EffectObjectState&) override {
        ++calls;
        return 0x12345678u;
    }
};

void TestGetScale() {
    pal4::EffectObjectState state{};
    state.words[13u] = 0x11111111u;
    state.words[14u] = 0x22222222u;
    state.words[15u] = 0x333333abu;
    std::array<std::uint32_t, 3u> output{7u, 8u, 9u};
    Check(pal4::EffectObjectTransformFacets::GetScale(state, output) == 0u &&
              output == std::array<std::uint32_t, 3u>{7u, 8u, 9u},
          "inactive scale getter preserves output");
    state.words[1u] = 1u;
    Check(pal4::EffectObjectTransformFacets::GetScale(state, output) == 0xabu &&
              output == std::array<std::uint32_t, 3u>{
                  0x11111111u, 0x22222222u, 0x333333abu},
          "scale getter copies exact three words and last low byte");
}

void TestSetters() {
    pal4::EffectObjectState state{};
    state.words.fill(0xaaaaaaaau);
    state.words[1u] &= 0xffffff00u;
    Runtime runtime;
    const std::array<float, 3u> values{2.0f, -3.0f, 0.5f};
    Check(pal4::EffectObjectTransformFacets::SetScale(
              state, values, 0u, 0x1234567fu, runtime) == 0x12345600u &&
              pal4::EffectObjectTransformFacets::SetRotation(
                  state, values, 1u, 0x1234567fu, runtime) == 0x12345600u &&
              pal4::EffectObjectTransformFacets::SetPosition(
                  state, values, 0u, 0x1234567fu, runtime) == 0x12345600u &&
              runtime.calls == 0u && state.words[13u] == 0xaaaaaaaau,
          "inactive setters preserve words and entry EAX high bytes");

    state.words[1u] |= 1u;
    for (std::uint32_t i = 13u; i != 22u; ++i)
        state.words[i] = std::bit_cast<std::uint32_t>(1.0f);
    Check(pal4::EffectObjectTransformFacets::SetScale(
              state, values, 0u, 0u, runtime) == 0x12345678u &&
              pal4::EffectObjectTransformFacets::SetRotation(
                  state, values, 0u, 0u, runtime) == 0x12345678u &&
              pal4::EffectObjectTransformFacets::SetPosition(
                  state, values, 0u, 0u, runtime) == 0x12345678u &&
              runtime.calls == 3u,
          "absolute setters forward frame update return");
    for (std::uint32_t group = 0u; group != 3u; ++group) {
        for (std::uint32_t i = 0u; i != 3u; ++i)
            Check(state.words[13u + group * 3u + i] ==
                      std::bit_cast<std::uint32_t>(values[i]),
                  "each setter writes its own IDA triple");
    }

    const std::array<float, 3u> increment{1.0f, 2.0f, -0.25f};
    (void)pal4::EffectObjectTransformFacets::SetScale(
        state, increment, 1u, 0u, runtime);
    (void)pal4::EffectObjectTransformFacets::SetRotation(
        state, increment, 2u, 0u, runtime);
    (void)pal4::EffectObjectTransformFacets::SetPosition(
        state, increment, 1u, 0u, runtime);
    for (std::uint32_t group = 0u; group != 3u; ++group) {
        for (std::uint32_t i = 0u; i != 3u; ++i)
            Check(state.words[13u + group * 3u + i] ==
                      std::bit_cast<std::uint32_t>(values[i] + increment[i]),
                  "relative setters add float components");
    }
    Check(runtime.calls == 6u,
          "each active setter updates native frame exactly once");
}

void TestFrameRebuild() {
    struct FrameRuntime final : pal4::IEffectObjectFrameRuntime {
        std::vector<std::string> calls;
        std::uint32_t axis_index = 0u;
        void SetIdentity(std::uint32_t frame) override {
            Check(frame == 0x4000u, "identity frame");
            calls.emplace_back("identity");
        }
        void Scale(std::uint32_t frame,
                   const std::array<std::uint32_t, 3u>& values,
                   std::uint32_t mode) override {
            Check(frame == 0x4000u && mode == 0u &&
                      values == std::array<std::uint32_t, 3u>{1u, 2u, 3u},
                  "scale triple and mode");
            calls.emplace_back("scale");
        }
        void Rotate(std::uint32_t frame,
                    const std::array<float, 3u>& axis,
                    float angle, std::uint32_t mode) override {
            constexpr std::array<std::array<float, 3u>, 3u> axes{{
                {1.0f, 0.0f, 0.0f},
                {0.0f, 1.0f, 0.0f},
                {0.0f, 0.0f, 1.0f}}};
            Check(frame == 0x4000u && mode == 2u && axis_index < 3u &&
                      axis == axes[axis_index] &&
                      angle == static_cast<float>(axis_index + 1u),
                  "rotation axes, angles and mode");
            ++axis_index;
            calls.emplace_back("rotate");
        }
        void Translate(std::uint32_t frame,
                       const std::array<std::uint32_t, 3u>& values,
                       std::uint32_t mode) override {
            Check(frame == 0x4000u && mode == 2u &&
                      values == std::array<std::uint32_t, 3u>{4u, 5u, 6u},
                  "position triple and mode");
            calls.emplace_back("translate");
        }
        std::uint32_t UpdateObjects(std::uint32_t frame) override {
            Check(frame == 0x4000u, "frame object update");
            calls.emplace_back("update");
            return 0x12345678u;
        }
    } runtime;
    pal4::EffectObjectState state{};
    Check(pal4::EffectObjectFrame::Rebuild(state, runtime) == 0u &&
              runtime.calls.empty(), "missing frame returns zero");
    state.words[24u] = 0x4000u;
    state.words[13u] = 1u;
    state.words[14u] = 2u;
    state.words[15u] = 3u;
    state.words[16u] = std::bit_cast<std::uint32_t>(1.0f);
    state.words[17u] = std::bit_cast<std::uint32_t>(2.0f);
    state.words[18u] = std::bit_cast<std::uint32_t>(3.0f);
    state.words[19u] = 4u;
    state.words[20u] = 5u;
    state.words[21u] = 6u;
    Check(pal4::EffectObjectFrame::Rebuild(state, runtime) == 0x12345678u &&
              runtime.calls == std::vector<std::string>{
                  "identity", "scale", "rotate", "rotate", "rotate",
                  "translate", "update"},
          "frame rebuild order and native return");
}

}  // namespace

int main() {
    TestGetScale();
    TestSetters();
    TestFrameRebuild();
}
