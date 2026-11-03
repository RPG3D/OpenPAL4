#include "pal4/dynamic_strip_object_facets.h"

#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct VisibilityRuntime final : pal4::IDynamicStripVisibilityRuntime {
    int calls = 0;
    void Deactivate(pal4::DynamicStripObjectState& state) override {
        ++calls;
        (void)pal4::DynamicStripVisibilityFacet::Deactivate(state);
    }
};

void TestVisibility() {
    pal4::DynamicStripObjectState state{};
    VisibilityRuntime runtime;
    state.words[1u] = 0xaabbcc00u;
    Check(pal4::DynamicStripVisibilityFacet::Activate(state, runtime) == 0u &&
              runtime.calls == 1 && state.words[1u] == 0xaabbcc00u,
          "activation invokes virtual reset even when inactive");
    Check(pal4::DynamicStripVisibilityFacet::Deactivate(state) == 0u &&
              state.words[1u] == 0xaabbcc00u,
          "inactive deactivation leaves flags alone");
    state.words[1u] = 0xaabbcc01u;
    Check(pal4::DynamicStripVisibilityFacet::Activate(state, runtime) == 1u &&
              runtime.calls == 2 && state.words[1u] == 0x0101cc01u,
          "active activation clears then enables flags six and seven");
    Check(pal4::DynamicStripVisibilityFacet::Deactivate(state) == 0u &&
              state.words[1u] == 0x0000cc01u,
          "active deactivation clears only flags six and seven");
}

void TestColor() {
    pal4::DynamicStripObjectState state{};
    std::array<std::uint8_t, 4> rgba{5u, 6u, 7u, 8u};
    Check(pal4::DynamicStripColorFacet::Get(state, rgba) == 0u &&
              rgba == std::array<std::uint8_t, 4>{5u, 6u, 7u, 8u},
          "inactive color getter preserves output");
    Check(pal4::DynamicStripColorFacet::Set(state, rgba) == 0u &&
              state.words[26u] == 0u,
          "inactive color setter preserves field");
    state.words[1u] = 1u;
    state.words[26u] = 0xefcdab89u;
    Check(pal4::DynamicStripColorFacet::Get(state, rgba) == 0xefu &&
              rgba == std::array<std::uint8_t, 4>{0x89u, 0xabu, 0xcdu, 0xefu},
          "color getter copies RGBA and returns alpha");
    rgba = {0x12u, 0x34u, 0x56u, 0xffu};
    Check(pal4::DynamicStripColorFacet::Set(state, rgba) == 0x12u &&
              state.words[26u] == 0xff563412u,
          "color setter packs bytes and returns red");
}

void TestTransforms() {
    pal4::DynamicStripObjectState state{};
    std::array<std::uint32_t, 3> values{9u, 10u, 11u};
    Check(pal4::DynamicStripTransformFacet::GetScale(state, values) == 0u &&
              pal4::DynamicStripTransformFacet::GetRotation(state, values) == 0u &&
              pal4::DynamicStripTransformFacet::GetPosition(state, values) == 0u &&
              values == std::array<std::uint32_t, 3>{9u, 10u, 11u},
          "inactive transform getters preserve output");
    state.words[1u] = 1u;
    state.words[17u] = 0x11223344u;
    state.words[18u] = 0x55667788u;
    state.words[19u] = 0xaabbcc01u;
    state.words[20u] = 0x12345678u;
    state.words[21u] = 0x87654321u;
    state.words[22u] = 0xaabbcc02u;
    state.words[23u] = 0x11111111u;
    state.words[24u] = 0x22222222u;
    state.words[25u] = 0xaabbcc03u;
    Check(pal4::DynamicStripTransformFacet::GetScale(state, values) == 1u &&
              values == std::array<std::uint32_t, 3>{
                  0x11223344u, 0x55667788u, 0xaabbcc01u},
          "scale uses owner offsets 68 through 76");
    Check(pal4::DynamicStripTransformFacet::GetRotation(state, values) == 2u &&
              values == std::array<std::uint32_t, 3>{
                  0x12345678u, 0x87654321u, 0xaabbcc02u},
          "rotation uses owner offsets 80 through 88");
    Check(pal4::DynamicStripTransformFacet::GetPosition(state, values) == 3u &&
              values == std::array<std::uint32_t, 3>{
                  0x11111111u, 0x22222222u, 0xaabbcc03u},
          "position uses owner offsets 92 through 100");
}

struct TransformRuntime final : pal4::IDynamicStripTransformRuntime {
    int calls = 0;
    std::array<std::uint32_t, 3> observed{};
    std::uint32_t first_offset = 0;
    std::uint8_t result = 0x7bu;
    std::uint8_t UpdateFrame(pal4::DynamicStripObjectState& state) override {
        ++calls;
        for (std::uint32_t i = 0; i != 3u; ++i)
            observed[i] = state.words[first_offset / 4u + i];
        return result;
    }
};

void TestTransformSetters() {
    pal4::DynamicStripObjectState state{};
    TransformRuntime runtime;
    const std::array<float, 3> input{1.5f, -2.0f, 3.25f};
    Check(pal4::DynamicStripTransformFacet::SetScale(
              state, input, 0u, runtime) == 0u && runtime.calls == 0,
          "inactive transform setter skips frame update");
    state.words[1u] = 1u;
    runtime.first_offset = 68u;
    Check(pal4::DynamicStripTransformFacet::SetScale(
              state, input, 0u, runtime) == 0x7bu && runtime.calls == 1 &&
              runtime.observed == std::array<std::uint32_t, 3>{
                  std::bit_cast<std::uint32_t>(1.5f),
                  std::bit_cast<std::uint32_t>(-2.0f),
                  std::bit_cast<std::uint32_t>(3.25f)},
          "absolute scale writes three raw floats before frame update");
    Check(pal4::DynamicStripTransformFacet::SetScale(
              state, {0.5f, 1.0f, -0.25f}, 2u, runtime) == 0x7bu &&
              runtime.calls == 2 && runtime.observed ==
                  std::array<std::uint32_t, 3>{
                      std::bit_cast<std::uint32_t>(2.0f),
                      std::bit_cast<std::uint32_t>(-1.0f),
                      std::bit_cast<std::uint32_t>(3.0f)},
          "nonzero mode adds to current scale");
    runtime.first_offset = 80u;
    state.words[20u] = std::bit_cast<std::uint32_t>(10.0f);
    state.words[21u] = std::bit_cast<std::uint32_t>(20.0f);
    state.words[22u] = std::bit_cast<std::uint32_t>(30.0f);
    Check(pal4::DynamicStripTransformFacet::SetRotation(
              state, input, 1u, runtime) == 0x7bu && runtime.calls == 3 &&
              runtime.observed == std::array<std::uint32_t, 3>{
                  std::bit_cast<std::uint32_t>(11.5f),
                  std::bit_cast<std::uint32_t>(18.0f),
                  std::bit_cast<std::uint32_t>(33.25f)},
          "rotation relative mode uses offsets 80 through 88");
    runtime.first_offset = 92u;
    Check(pal4::DynamicStripTransformFacet::SetPosition(
              state, input, 0u, runtime) == 0x7bu && runtime.calls == 4 &&
              runtime.observed == std::array<std::uint32_t, 3>{
                  std::bit_cast<std::uint32_t>(1.5f),
                  std::bit_cast<std::uint32_t>(-2.0f),
                  std::bit_cast<std::uint32_t>(3.25f)},
          "absolute position uses offsets 92 through 100");
    Check(state.words[17u] == std::bit_cast<std::uint32_t>(2.0f) &&
              state.words[20u] == std::bit_cast<std::uint32_t>(11.5f),
          "unrelated transform fields stay isolated");
    runtime.first_offset = 92u;
    Check(pal4::DynamicStripTransformFacet::SetTrackPosition(
              state, {0xdeadbeefu, 0x12345678u, 0x87654321u},
              runtime) == 0x7bu && runtime.calls == 5 &&
              runtime.observed == std::array<std::uint32_t, 3>{
                  0xdeadbeefu, 0x12345678u, 0x87654321u} &&
              state.words[22u] == std::bit_cast<std::uint32_t>(33.25f) &&
              state.words[26u] == 0u,
          "track facet writes exactly owner offsets 92 through 100");
    state.words[1u] = 0u;
    Check(pal4::DynamicStripTransformFacet::SetTrackPosition(
              state, {1u, 2u, 3u}, runtime) == 0u &&
              runtime.calls == 5 && state.words[23u] == 0xdeadbeefu,
          "inactive track setter leaves fields and frame alone");
}

}  // namespace

int main() {
    TestVisibility();
    TestColor();
    TestTransforms();
    TestTransformSetters();
}
