#include "pal4/camera_projection_matrix.h"

#include <bit>
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

struct Runtime final : pal4::ICameraProjectionRuntime {
    pal4::CameraProjectionGlobals& globals;
    std::vector<std::string> calls;
    float parameter{2.0f};
    float depth{3.0f};

    explicit Runtime(pal4::CameraProjectionGlobals& value) : globals(value) {}

    [[nodiscard]] std::uint32_t ReadWord(
        const std::uint32_t address) override {
        Check(globals.texture == 0x2000u &&
                  globals.strength_bits == std::bit_cast<std::uint32_t>(-0.75f),
              "projection globals stored before memory access");
        calls.push_back("word");
        Check(address == 0x3004u || address == 0x5004u,
              "scene or camera frame offset");
        return address == 0x3004u ? 0x3100u : 0x5100u;
    }
    [[nodiscard]] float ReadFloat(const std::uint32_t address) override {
        Check(address == 0x5068u, "camera parameter offset 104");
        calls.push_back("parameter");
        return parameter;
    }
    [[nodiscard]] std::uint32_t GetObjectTransform(
        const std::uint32_t frame) override {
        Check(frame == 0x3100u, "scene frame transform");
        calls.push_back("get-transform");
        return 0x7000u;
    }
    void CopyMatrix(pal4::CameraProjectionMatrixImage& destination,
                    const std::uint32_t source) override {
        Check(source == 0x7000u, "scene transform source");
        calls.push_back("copy");
        destination.fill(0x12345678u);
        destination[0] = std::bit_cast<std::uint32_t>(1.0f);
        destination[1] = std::bit_cast<std::uint32_t>(2.0f);
        destination[2] = std::bit_cast<std::uint32_t>(3.0f);
    }
    void InvertMatrix(
        pal4::CameraProjectionMatrixImage& destination,
        const std::uint32_t source) override {
        Check(source == 0x5110u, "camera frame plus sixteen source");
        calls.push_back("setup");
        destination.fill(0x87654321u);
    }
    void Scale(pal4::CameraProjectionMatrixImage& matrix,
               const pal4::CameraProjectionVector3& scale,
               const std::uint32_t mode) override {
        Check(matrix[0] == 0x87654321u && mode == 2u &&
                  scale[0] == -0.5f / parameter &&
                  scale[1] == -0.5f / parameter &&
                  scale[2] == 1.0f / (parameter + depth),
              "projection scale vector and mode");
        calls.push_back("scale");
    }
    void Translate(pal4::CameraProjectionMatrixImage& matrix,
                   const pal4::CameraProjectionVector3& offset,
                   const std::uint32_t mode) override {
        Check(matrix[0] == 0x87654321u && mode == 2u &&
                  offset == pal4::CameraProjectionVector3{0.5f, 0.5f, 0.0f},
              "projection half-texel translation");
        calls.push_back("translate");
    }
    void Multiply(pal4::CameraProjectionMatrixImage& destination,
                  const pal4::CameraProjectionMatrixImage& left,
                  const pal4::CameraProjectionMatrixImage& right) override {
        Check(left[0] == std::bit_cast<std::uint32_t>(-1.0f) &&
                  left[1] == std::bit_cast<std::uint32_t>(-2.0f) &&
                  left[2] == std::bit_cast<std::uint32_t>(-3.0f) &&
                  left[3] == 0x12345678u && right[0] == 0x87654321u,
              "only scene matrix first direction is negated");
        calls.push_back("multiply");
        destination.fill(0xffffffffu);
    }
    [[nodiscard]] std::uint32_t SetTransform(
        const std::uint32_t transform_id,
        const pal4::CameraProjectionMatrixImage& matrix) override {
        Check(transform_id == 17u && matrix[3] == 0u &&
                  matrix[7] == 0u && matrix[11] == 0u &&
                  matrix[15] == 0x3f800000u &&
                  matrix[0] == 0xffffffffu && matrix[14] == 0xffffffffu,
              "D3D9 transform slot and matrix padding");
        calls.push_back("set-transform");
        return 0xbeefu;
    }
};

void TestProjection() {
    pal4::CameraProjectionGlobals globals{0u, 0u};
    Runtime runtime(globals);
    Check(pal4::CameraProjectionMatrix::Set(
              0x2000u, 0x3000u, 0x5000u, -0.75f, 3.0f,
              globals, runtime) == 0xbeefu,
          "D3D9 transform result forwarded");
    Check(runtime.calls == std::vector<std::string>{
              "word", "get-transform", "copy", "word", "parameter",
              "setup", "scale", "translate", "multiply", "set-transform"},
          "projection call order");

    runtime.calls.clear();
    runtime.parameter = 4.0f;
    runtime.depth = -2.0f;
    Check(pal4::CameraProjectionMatrix::Set(
              0x2000u, 0x3000u, 0x5000u, -0.75f, -2.0f,
              globals, runtime) == 0xbeefu &&
              runtime.calls.back() == "set-transform",
          "signed depth and changed camera parameter flow into scale");
}

}  // namespace

int main() {
    TestProjection();
}
