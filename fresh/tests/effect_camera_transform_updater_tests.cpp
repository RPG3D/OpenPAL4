#include "pal4/effect_camera_transform_updater.h"

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

std::uint32_t Bits(const float value) {
    return std::bit_cast<std::uint32_t>(value);
}

struct Runtime final : pal4::IEffectCameraTransformRuntime {
    std::vector<std::string> events;
    pal4::CameraVector3 first_direction{};
    std::uint32_t child_frame = 0u;
    std::uint32_t update_count = 0u;
    std::uint32_t normalize_count = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x8004u, "target child pointer is at frame +4");
        events.push_back("read-child");
        return child_frame;
    }
    pal4::CameraVector3 GetTargetPosition(
        const pal4::EffectCameraObjectState&, std::uint32_t offset) override {
        Check(offset == 64u, "target position uses name at owner +64");
        events.push_back("target-position");
        return {10.0f, 20.0f, 30.0f};
    }
    pal4::CameraVector3 GetTargetLookAt(
        const pal4::EffectCameraObjectState&, std::uint32_t offset) override {
        Check(offset == 64u, "target LookAt uses name at owner +64");
        events.push_back("target-lookat");
        return {4.0f, 5.0f, 6.0f};
    }
    std::uint32_t ObjectTransform(std::uint32_t frame) override {
        if (frame == 0x9000u) {
            events.push_back("child-transform");
            return 0xa000u;
        }
        Check(frame == 0x8000u, "frame axis uses source frame transform");
        events.push_back("source-transform");
        return 0xb000u;
    }
    float ReadFloat(std::uint32_t address) override {
        if (address >= 0xa030u && address <= 0xa038u) {
            const auto index = (address - 0xa030u) / 4u;
            events.push_back("target-translation");
            return static_cast<float>(index + 1u);
        }
        Check(address >= 0xb020u && address <= 0xb028u &&
                  (address % 4u) == 0u,
              "roll axis reads source transform +32..+40");
        const auto index = (address - 0xb020u) / 4u;
        events.push_back("read-axis");
        return index == 1u ? 1.0f : 0.0f;
    }
    void SetupMatrix(pal4::CameraMatrixWords& matrix,
                     std::uint32_t transform) override {
        Check(transform == 0xa000u, "child transform matrix input");
        matrix[0u] = 0x12345678u;
        events.push_back("setup-target-matrix");
    }
    void MultiplyMatrix(pal4::CameraMatrixWords& matrix,
                        std::uint32_t frame_matrix) override {
        Check(matrix[0u] == 0x12345678u && frame_matrix == 0x9010u,
              "child frame matrix multiplication operands");
        events.push_back("multiply-target-matrix");
    }
    void TransformPoint(pal4::CameraVector3& point,
                        const pal4::CameraMatrixWords& matrix) override {
        Check(matrix[0u] == 0x12345678u &&
                  point == pal4::CameraVector3{9.0f, 18.0f, 27.0f},
              "target point has transform translation removed");
        point = {109.0f, 218.0f, 327.0f};
        events.push_back("transform-point");
    }
    void TransformVector(pal4::CameraVector3& vector,
                         const pal4::CameraMatrixWords& matrix) override {
        Check(matrix[0u] == 0x12345678u &&
                  vector == pal4::CameraVector3{3.0f, 3.0f, -7.0f},
              "target LookAt is relative to stored LookAt");
        vector = {6.0f, 6.0f, -14.0f};
        events.push_back("transform-vector");
    }
    void Normalize(pal4::CameraVector3& vector) override {
        if (normalize_count++ == 0u)
            first_direction = vector;
        events.push_back("normalize");
        const float length = std::sqrt(vector[0u] * vector[0u] +
                                       vector[1u] * vector[1u] +
                                       vector[2u] * vector[2u]);
        Check(length > 0.0f, "basis normalization has nonzero vector");
        for (float& component : vector)
            component /= length;
    }
    void UpdateMatrix(pal4::CameraMatrixWords& matrix) override {
        Check((matrix[3u] & 0x20003u) == 0x20003u &&
                  matrix[12u] == 0u && matrix[13u] == 0u &&
                  matrix[14u] == 0u,
              "basis matrix has native flags and zero translation");
        events.push_back("update-basis-matrix");
    }
    void SetFrameIdentity(std::uint32_t frame) override {
        Check(frame == 0x8000u, "identity targets source frame");
        events.push_back("identity");
    }
    void ApplyMatrix(std::uint32_t frame,
                     const pal4::CameraMatrixWords& matrix,
                     std::uint32_t mode) override {
        Check(frame == 0x8000u && mode == 0u &&
                  (matrix[3u] & 0x20003u) == 0x20003u,
              "basis applied to source frame with mode zero");
        events.push_back("apply-basis");
    }
    std::uint32_t UpdateFrameObjects(std::uint32_t frame) override {
        Check(frame == 0x8000u, "update targets source frame");
        events.push_back("update-frame");
        return ++update_count == 1u ? 0x11111111u : 0x12345678u;
    }
    void RotateFrame(std::uint32_t frame,
                     const pal4::CameraVector3& axis, float angle,
                     std::uint32_t mode) override {
        Check(frame == 0x8000u, "rotation targets source frame");
        if (axis == pal4::CameraVector3{1.0f, 0.0f, 0.0f}) {
            Check(angle == 10.0f && mode == 0u,
                  "ordinary pitch rotates about X with mode zero");
            events.push_back("rotate-x");
        } else if (axis == pal4::CameraVector3{0.0f, 0.0f, 1.0f}) {
            Check(angle == 30.0f && mode == 2u,
                  "ordinary roll rotates about Z with mode two");
            events.push_back("rotate-z");
        } else {
            Check(axis == pal4::CameraVector3{0.0f, 1.0f, 0.0f} &&
                      mode == 2u, "Y axis rotation uses mode two");
            if (update_count == 0u) {
                Check(angle == 20.0f, "ordinary yaw uses owner +92");
                events.push_back("rotate-y");
            } else {
                Check(angle == 30.0f,
                      "locked mode applies owner +96 roll about frame axis");
                events.push_back("rotate-frame-axis");
            }
        }
    }
    void TranslateFrame(
        std::uint32_t frame,
        const std::array<std::uint32_t, 3>& position,
        std::uint32_t mode) override {
        Check(frame == 0x8000u && mode == 2u &&
                  position == std::array<std::uint32_t, 3>{
                      Bits(1.0f), Bits(2.0f), Bits(3.0f)},
              "frame translation uses raw owner +100..+108 words");
        events.push_back("translate");
    }
    void Reset() {
        events.clear();
        update_count = 0u;
        normalize_count = 0u;
        first_direction = {};
    }
};

}  // namespace

int main() {
    pal4::EffectCameraObjectState state{};
    Runtime runtime;
    Check(pal4::EffectCameraTransformUpdater::Update(state, runtime) == 0u &&
              runtime.events.empty(), "missing source frame is inert");
    state.words[31u] = 0x8000u;
    state.words[11u] = 1u;
    Check(pal4::EffectCameraTransformUpdater::Update(state, runtime) ==
              0x8000u && runtime.events.empty(),
          "inherit mode returns frame without transform calls");

    state.words[11u] = 0u;
    state.words[12u] = Bits(1.0f);
    state.words[13u] = Bits(2.0f);
    state.words[14u] = Bits(13.0f);
    state.words[22u] = Bits(10.0f);
    state.words[23u] = Bits(20.0f);
    state.words[24u] = Bits(30.0f);
    state.words[25u] = Bits(1.0f);
    state.words[26u] = Bits(2.0f);
    state.words[27u] = Bits(3.0f);
    Check(pal4::EffectCameraTransformUpdater::Update(state, runtime) ==
              0x11111111u &&
              runtime.events == std::vector<std::string>{
                  "identity", "rotate-x", "rotate-y", "rotate-z",
                  "translate", "update-frame"},
          "ordinary camera path rotates X/Y/Z then translates");

    runtime.Reset();
    state.words[11u] = 0x01000000u;
    Check(pal4::EffectCameraTransformUpdater::Update(state, runtime) ==
              0x12345678u &&
              runtime.first_direction ==
                  pal4::CameraVector3{0.0f, 0.0f, 10.0f} &&
              runtime.events == std::vector<std::string>{
                  "normalize", "normalize", "normalize",
                  "update-basis-matrix", "identity", "apply-basis",
                  "update-frame", "source-transform", "read-axis",
                  "read-axis", "read-axis", "rotate-frame-axis",
                  "translate", "update-frame"},
          "locked camera uses LookAt direction and current frame roll axis");

    runtime.Reset();
    state.words[15u] = 1u;
    Check(pal4::EffectCameraTransformUpdater::Update(state, runtime) ==
              0x12345678u &&
              runtime.first_direction ==
                  pal4::CameraVector3{0.0f, 0.0f, 10.0f} &&
              runtime.events.front() == "read-child",
          "missing child frame falls back to stored LookAt");

    runtime.Reset();
    runtime.child_frame = 0x9000u;
    Check(pal4::EffectCameraTransformUpdater::Update(state, runtime) ==
              0x12345678u &&
              runtime.first_direction ==
                  pal4::CameraVector3{102.0f, 210.0f, 338.0f} &&
              runtime.events == std::vector<std::string>{
                  "read-child", "target-position", "child-transform",
                  "target-translation", "target-translation",
                  "target-translation", "setup-target-matrix",
                  "multiply-target-matrix", "transform-point",
                  "target-lookat", "transform-vector", "normalize",
                  "normalize", "normalize", "update-basis-matrix",
                  "identity", "apply-basis", "update-frame",
                  "source-transform", "read-axis", "read-axis",
                  "read-axis", "rotate-frame-axis", "translate",
                  "update-frame"},
          "target mode composes child transform before camera basis");
}
