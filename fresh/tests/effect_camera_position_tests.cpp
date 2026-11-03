#include "pal4/effect_camera_position.h"

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

void PutByte(pal4::EffectCameraObjectState& state,
             std::uint32_t offset, std::uint8_t value) {
    const auto shift = 8u * (offset % 4u);
    const auto mask = 0xffu << shift;
    state.words[offset / 4u] =
        (state.words[offset / 4u] & ~mask) |
        (static_cast<std::uint32_t>(value) << shift);
}

struct Runtime final : pal4::IEffectCameraPositionRuntime {
    std::vector<std::string> events;
    std::uint32_t child_frame = 0x9000u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x8004u, "child frame pointer at source frame +4");
        events.push_back("child-frame");
        return child_frame;
    }
    pal4::CameraVector3 GetTargetPosition(
        const pal4::EffectCameraObjectState&,
        std::uint32_t offset) override {
        Check(offset == 64u, "target name field offset for position");
        events.push_back("target-position");
        return {10.0f, 20.0f, 30.0f};
    }
    pal4::CameraVector3 GetTargetLookAt(
        const pal4::EffectCameraObjectState&,
        std::uint32_t offset) override {
        Check(offset == 64u, "target name field offset for LookAt");
        events.push_back("target-lookat");
        return {4.0f, 5.0f, 6.0f};
    }
    std::uint32_t ObjectTransform(std::uint32_t frame) override {
        Check(frame == 0x9000u, "object transform frame");
        events.push_back("object-transform");
        return 0xa000u;
    }
    float ReadFloat(std::uint32_t address) override {
        Check(address >= 0xa030u && address <= 0xa038u &&
                  (address % 4u) == 0u,
              "transform translation word offset");
        events.push_back("read-translation:" + std::to_string(address));
        return static_cast<float>((address - 0xa030u) / 4u + 1u);
    }
    void SetupMatrix(pal4::CameraMatrixWords& matrix,
                     std::uint32_t transform) override {
        Check(transform == 0xa000u && sizeof(matrix) == 64u,
              "64-byte transform matrix temporary");
        events.push_back("setup-matrix");
        matrix[0u] = 0x12345678u;
    }
    void MultiplyMatrix(pal4::CameraMatrixWords& matrix,
                        std::uint32_t frame_matrix) override {
        Check(matrix[0u] == 0x12345678u &&
                  frame_matrix == 0x9010u,
              "matrix multiplication operands");
        events.push_back("multiply-matrix");
    }
    void TransformPoint(pal4::CameraVector3& point,
                        const pal4::CameraMatrixWords& matrix) override {
        Check(matrix[0u] == 0x12345678u &&
                  point == pal4::CameraVector3{9.0f, 18.0f, 27.0f},
              "point is offset by object transform translation");
        events.push_back("transform-point");
        point = {109.0f, 218.0f, 327.0f};
    }
    void TransformVector(pal4::CameraVector3& vector,
                         const pal4::CameraMatrixWords& matrix) override {
        Check(matrix[0u] == 0x12345678u &&
                  vector == pal4::CameraVector3{3.0f, 3.0f, 3.0f},
              "target LookAt is offset by original camera position");
        events.push_back("transform-vector");
        vector = {6.0f, 6.0f, 6.0f};
    }
};

}  // namespace

int main() {
    Runtime runtime;
    pal4::EffectCameraObjectState state{};
    state.words[12u] = Bits(1.0f);
    state.words[13u] = Bits(2.0f);
    state.words[14u] = Bits(3.0f);
    state.words[31u] = 0x8000u;
    std::array<std::uint32_t, 3> output{
        0x111u, 0x222u, 0x333u};
    Check(pal4::EffectCameraPosition::GetLookAtWorld(
              state, output, 0x5au, runtime) == 0u &&
              output == std::array<std::uint32_t, 3>{
                  0x111u, 0x222u, 0x333u} && runtime.events.empty(),
          "inactive camera does not write outputs");
    PutByte(state, 4u, 1u);
    Check(pal4::EffectCameraPosition::GetLookAtWorld(
              state, output, 0x5au, runtime) == 0u &&
              runtime.events.empty(), "unready camera leaves outputs intact");
    PutByte(state, 80u, 1u);
    Check(pal4::EffectCameraPosition::GetLookAtWorld(
              state, output, 0x5au, runtime) == 0x5au &&
              output == std::array<std::uint32_t, 3>{
                  Bits(1.0f), Bits(2.0f), Bits(3.0f)} &&
              runtime.events.empty(),
          "ordinary LookAt path copies three raw coordinate words");
    PutByte(state, 60u, 1u);
    runtime.child_frame = 0u;
    Check(pal4::EffectCameraPosition::GetLookAtWorld(
              state, output, 0x5au, runtime) == 0x5au &&
              runtime.events == std::vector<std::string>{"child-frame"} &&
              output[0u] == Bits(1.0f),
          "missing child frame falls back to stored LookAt");
    runtime.events.clear();
    runtime.child_frame = 0x9000u;
    Check(pal4::EffectCameraPosition::GetLookAtWorld(
              state, output, 0x5au, runtime) == 0x5au &&
              runtime.events == std::vector<std::string>{
                  "child-frame", "target-position", "object-transform",
                  "read-translation:41008", "read-translation:41012",
                  "read-translation:41016", "setup-matrix",
                  "multiply-matrix", "transform-point", "target-lookat",
                  "transform-vector"} &&
              output == std::array<std::uint32_t, 3>{
                  Bits(103.0f), Bits(212.0f), Bits(321.0f)},
          "target mode computes transformed world LookAt in native order");
}
