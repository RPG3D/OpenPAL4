#include "pal4/global_render_state.h"

#include <array>
#include <bit>
#include <cmath>
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

class Runtime final : public pal4::IGlobalRenderStateRuntime {
public:
    std::uint32_t calls = 0u;
    std::uint32_t commits = 0u;
    bool strict_initial_state = true;
    std::array<std::uint32_t, 3> result{0x11u, 0x22u, 0x33u};
    std::array<float, 3> committed{};
    std::array<std::uint32_t, 13> transform_snapshot{};

    std::array<std::uint32_t, 3> TransformDirection(
        const pal4::GlobalRenderState& state,
        const std::array<std::uint32_t, 3>& source) override {
        ++calls;
        Check(source == std::array<std::uint32_t, 3>{
                            0u, 0u, 0x3f800000u},
              "virtual transform receives default source vector");
        for (std::uint32_t i = 0u; i < transform_snapshot.size(); ++i)
            transform_snapshot[i] = state.ReadDword(i * 4u);
        if (strict_initial_state) {
            Check(state.ReadDword(0u) == 0x841dc8u,
                  "virtual transform sees installed vtable");
            for (std::uint32_t offset = 4u; offset <= 0x30u; offset += 4u) {
                Check(state.ReadDword(offset) ==
                          (offset == 0x24u ? 0x3f800000u : 0u),
                      "fields are cleared before virtual transform");
            }
        }
        return result;
    }
    std::uint32_t CommitDirection(
        pal4::GlobalRenderState& state,
        const std::array<float, 3>& direction) override {
        ++commits;
        committed = direction;
        Check(state.ReadDword(0x10u) == 0x11u &&
                  state.ReadDword(0x14u) == 0x22u &&
                  state.ReadDword(0x18u) == 0x33u,
              "basis is stored before direction callback");
        return 0x77u;
    }
};

void TestInitialization() {
    pal4::GlobalRenderState state;
    Runtime runtime;
    for (std::uint32_t offset = 0u; offset <= 0x30u; offset += 4u)
        state.WriteDword(offset, 0xdeadbeefu);
    Check(state.InitializeThunk(runtime) == 0x33u && runtime.calls == 1u,
          "thunk forwards to initializer and returns transformed z word");
    const std::array<std::uint32_t, 13> expected{
        0x841dc8u, 0x11u, 0x22u, 0x33u, 0u, 0x3f800000u, 0u,
        0u, 0u, 0x3f800000u, 0u, 0u, 0u};
    for (std::uint32_t i = 0u; i < expected.size(); ++i)
        Check(state.ReadDword(i * 4u) == expected[i],
              "final global render record matches every observed field");
    runtime.result = {0u, 0x80000000u, 0x3f800000u};
    Check(state.Initialize(runtime) == 0x3f800000u &&
              state.ReadDword(4u) == 0u &&
              state.ReadDword(8u) == 0x80000000u &&
              state.ReadDword(12u) == 0x3f800000u &&
              runtime.calls == 2u,
          "repeat initialization replaces transformed direction");
}

void TestBasisAndVectorOperations() {
    pal4::GlobalRenderState state;
    Runtime runtime;
    runtime.strict_initial_state = false;
    for (std::uint32_t offset = 0u; offset <= 0x30u; offset += 4u)
        state.WriteDword(offset, 0xdeadbeefu);
    state.WriteDword(0u, 0x999u);
    Check(state.InitializeDefaultBasis(runtime) == 0x33u && runtime.calls == 1u,
          "object default initializer returns transformed z word");
    for (std::uint32_t i = 0u; i < runtime.transform_snapshot.size(); ++i) {
        const auto expected = i == 0u ? 0x999u
            : i == 7u || i == 8u ? 0u
            : i == 9u ? 0x3f800000u : 0xdeadbeefu;
        Check(runtime.transform_snapshot[i] == expected,
              "direct default initializer changes only source row before callback");
    }
    const std::array<std::uint32_t, 13> default_basis{
        0x999u, 0x11u, 0x22u, 0x33u, 0u, 0x3f800000u, 0u,
        0u, 0u, 0x3f800000u, 0u, 0u, 0u};
    for (std::uint32_t i = 0u; i < default_basis.size(); ++i)
        Check(state.ReadDword(i * 4u) == default_basis[i],
              "object default initializer preserves vtable and writes every field");

    std::array<std::uint32_t, 3> output{};
    const std::array<std::uint32_t, 3> y{4u, 5u, 6u};
    const std::array<std::uint32_t, 3> z{7u, 8u, 9u};
    Check(&state.SetBasisY(output, y) == &output && output == y &&
              state.ReadDword(0x10u) == 4u &&
              state.ReadDword(0x14u) == 5u &&
              state.ReadDword(0x18u) == 6u,
          "first basis setter writes object and output");
    Check(&state.SetBasisZ(output, z) == &output && output == z &&
              state.ReadDword(0x10u) == 4u &&
              state.ReadDword(0x14u) == 5u &&
              state.ReadDword(0x18u) == 6u &&
              state.ReadDword(0x1cu) == 7u &&
              state.ReadDword(0x20u) == 8u &&
              state.ReadDword(0x24u) == 9u,
          "second basis setter writes object and output");

    for (std::uint32_t i = 0u; i < 9u; ++i)
        state.WriteDword(4u + i * 4u,
                         std::bit_cast<std::uint32_t>(static_cast<float>(i + 1u)));
    std::array<float, 3> vector{2.0f, 3.0f, 4.0f};
    Check(&state.TransformVector(vector, vector) == &vector &&
              vector == std::array<float, 3>{20.0f, 47.0f, 74.0f},
          "matrix transform multiplies three basis rows and supports aliasing");
    const std::array<float, 3> input{2.0f, 3.0f, 4.0f};
    std::array<float, 3> distinct_output{-1.0f, -1.0f, -1.0f};
    Check(&state.TransformVector(distinct_output, input) == &distinct_output &&
              distinct_output == std::array<float, 3>{20.0f, 47.0f, 74.0f} &&
              input == std::array<float, 3>{2.0f, 3.0f, 4.0f},
          "matrix transform returns distinct output and leaves input unchanged");
}

void TestNormalizedDirectionAndHelpers() {
    pal4::GlobalRenderState state;
    Runtime runtime;
    const std::array<std::uint32_t, 3> basis{0x11u, 0x22u, 0x33u};
    Check(state.SetNormalizedDirection({3.0f, 4.0f, 0.0f}, basis,
                                       runtime) == 0x77u &&
              runtime.commits == 1u &&
              std::abs(runtime.committed[0] - 0.6f) < 0.00001f &&
              std::abs(runtime.committed[1] - 0.8f) < 0.00001f,
          "positive vector is normalized before callback");
    Check(state.SetNormalizedDirection({0.0f, 0.0f, 0.0f}, basis,
                                       runtime) == 0x77u &&
              runtime.committed == std::array<float, 3>{0.0f, 0.0f, 0.0f},
          "zero vector bypasses division");
    const auto payload_nan = std::bit_cast<float>(0x7fc12345u);
    Check(state.SetNormalizedDirection({payload_nan, -0.0f, 1.0f}, basis,
                                       runtime) == 0x77u &&
              std::bit_cast<std::uint32_t>(runtime.committed[0]) ==
                  0x7fc12345u &&
              std::bit_cast<std::uint32_t>(runtime.committed[1]) ==
                  0x80000000u &&
              runtime.committed[2] == 1.0f,
          "unordered length forwards direction bits without normalization");
    Check(pal4::Vector3Math::Dot({1.0f, 2.0f, 3.0f},
                                {4.0f, 5.0f, 6.0f}) == 32.0 &&
              pal4::GlobalRenderState::VectorByteSize() == 12u,
          "dot product and vector-size virtual entry");
    float angle = 0.0f;
    pal4::GlobalRenderState::InitializeGlobalAngleThunk(angle);
    Check(pal4::GlobalRenderState::kGlobalAngleAddress == 0x8e3f2cu &&
              angle > 4.71238f && angle < 4.71240f,
          "global angle thunk stores the sum of IDA constants");
}

}  // namespace

int main() {
    TestInitialization();
    TestBasisAndVectorOperations();
    TestNormalizedDirectionAndHelpers();
    return EXIT_SUCCESS;
}
