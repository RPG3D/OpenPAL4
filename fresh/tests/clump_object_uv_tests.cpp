#include "pal4/clump_object_uv.h"

#include <array>
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

struct Runtime final : pal4::IClumpObjectUvRuntime {
    std::vector<std::string> calls;
    std::uint32_t next_matrix = 0x2000u;
    std::array<float, 3> last_vector{};
    float last_angle = 0.0f;

    std::uint32_t CreateMatrix() override {
        calls.push_back("create");
        return next_matrix++;
    }
    std::uint8_t DestroyMatrix(std::uint32_t matrix) override {
        calls.push_back("destroy:" + std::to_string(matrix));
        return 0xa5u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x1014u, "clump pointer at manager +20");
        calls.push_back("read_clump");
        return 0x3000u;
    }
    void ApplyToAtomics(std::uint32_t clump, bool second_uv,
                        std::uint32_t matrix) override {
        Check(clump == 0x3000u, "clump argument");
        calls.push_back(std::string(second_uv ? "second:" : "first:") +
                        std::to_string(matrix));
    }
    std::uint32_t RotateMatrix(std::uint32_t matrix,
                               const std::array<float, 3>& axis,
                               float angle, std::uint32_t mode) override {
        Check(mode == 1u, "rotation combine mode");
        last_vector = axis;
        last_angle = angle;
        calls.push_back("rotate:" + std::to_string(matrix));
        return 0x12345678u;
    }
    std::uint8_t TranslateMatrix(
        std::uint32_t matrix, const std::array<float, 3>& vector,
        std::uint32_t mode) override {
        Check(mode == 1u, "translation combine mode");
        last_vector = vector;
        calls.push_back("translate:" + std::to_string(matrix));
        return 0x7bu;
    }
};

void TestInactive() {
    pal4::ClumpObjectState state{};
    Runtime runtime;
    state.words[1u] = 0x100u;
    Check(pal4::ClumpObjectUvFacet::Acquire(
              state, pal4::ClumpUvFacetKind::Rotation, 0u, runtime) == 0u &&
              pal4::ClumpObjectUvFacet::Release(
                  state, pal4::ClumpUvFacetKind::Translation, 1u,
                  runtime) == 0u &&
              pal4::ClumpObjectUvFacet::Rotate(
                  state, 0u, {1.0f, 2.0f, 3.0f}, 4.0f,
                  runtime) == 0u &&
              pal4::ClumpObjectUvFacet::Translate(
                  state, 1u, {5.0f, 6.0f, 7.0f}, runtime) == 0u &&
              runtime.calls.empty(),
          "inactive low-byte gate has no side effects");
}

void TestRotationFacet() {
    pal4::ClumpObjectState state{};
    state.words[1u] = 1u;
    state.words[18u] = 0x1000u;
    Runtime runtime;
    using pal4::ClumpObjectUvFacet;
    using pal4::ClumpUvFacetKind;

    Check(ClumpObjectUvFacet::Acquire(
              state, ClumpUvFacetKind::Rotation, 0u, runtime) == 1u &&
              state.words[44u] == 0x2000u && state.words[48u] == 1u &&
              runtime.calls == std::vector<std::string>{
                  "create", "read_clump", "first:8192"},
          "first rotation acquisition creates and applies first UV matrix");
    runtime.calls.clear();
    Check(ClumpObjectUvFacet::Acquire(
              state, ClumpUvFacetKind::Rotation, 0u, runtime) == 2u &&
              state.words[48u] == 2u && runtime.calls.empty(),
          "repeated acquisition only increments count");
    Check(ClumpObjectUvFacet::Release(
              state, ClumpUvFacetKind::Rotation, 0u, runtime) == 1u &&
              runtime.calls.empty(),
          "nonfinal release keeps matrix");
    Check(ClumpObjectUvFacet::Rotate(
              state, 0u, {1.0f, 2.0f, 3.0f}, 42.0f,
              runtime) == 0x12345678u &&
              runtime.last_vector == std::array<float, 3>{1.0f, 2.0f, 3.0f} &&
              runtime.last_angle == 42.0f &&
              runtime.calls == std::vector<std::string>{"rotate:8192"},
          "rotation forwards matrix, axis, angle and result");
    runtime.calls.clear();
    Check(ClumpObjectUvFacet::Release(
              state, ClumpUvFacetKind::Rotation, 0u, runtime) == 0xa5u &&
              state.words[44u] == 0u && state.words[48u] == 0u &&
              runtime.calls == std::vector<std::string>{"destroy:8192"},
          "final release destroys and clears matrix");
    runtime.calls.clear();
    Check(ClumpObjectUvFacet::Release(
              state, ClumpUvFacetKind::Rotation, 0u, runtime) == 0xffu &&
              state.words[48u] == 0xffffffffu && runtime.calls.empty(),
          "native release underflows an empty count");
}

void TestTranslationFacetAndSecondUv() {
    pal4::ClumpObjectState state{};
    state.words[1u] = 1u;
    state.words[18u] = 0x1000u;
    Runtime runtime;
    using pal4::ClumpObjectUvFacet;
    using pal4::ClumpUvFacetKind;

    Check(ClumpObjectUvFacet::Acquire(
              state, ClumpUvFacetKind::Translation, 1u, runtime) == 1u &&
              state.words[47u] == 0x2000u && state.words[51u] == 1u &&
              runtime.calls == std::vector<std::string>{
                  "create", "read_clump", "second:8192"},
          "translation index one uses second UV and distinct fields");
    runtime.calls.clear();
    Check(ClumpObjectUvFacet::Translate(
              state, 1u, {4.0f, 5.0f, 6.0f}, runtime) == 0x7bu &&
              runtime.last_vector == std::array<float, 3>{4.0f, 5.0f, 6.0f} &&
              runtime.calls == std::vector<std::string>{"translate:8192"},
          "translation forwards matrix and vector");
    runtime.calls.clear();
    Check(ClumpObjectUvFacet::Release(
              state, ClumpUvFacetKind::Translation, 1u, runtime) == 0xa5u &&
              state.words[47u] == 0u && state.words[51u] == 0u &&
              runtime.calls == std::vector<std::string>{"destroy:8192"},
          "translation release clears its own matrix");
    runtime.calls.clear();
    Check(ClumpObjectUvFacet::Acquire(
              state, ClumpUvFacetKind::Rotation, 1u, runtime) == 1u &&
              state.words[45u] == 0x2001u && state.words[49u] == 1u &&
              runtime.calls == std::vector<std::string>{
                  "create", "read_clump", "second:8193"},
          "rotation index one also selects second UV callback");
}

}  // namespace

int main() {
    TestInactive();
    TestRotationFacet();
    TestTranslationFacetAndSecondUv();
}
