#include "pal4/camera_matrix.h"
#include "pal4/camera_bounds.h"

#include <bit>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::ICameraMatrixRuntime,
                      public pal4::ICameraBoundsRuntime {
public:
    unsigned calls = 0u;
    unsigned primary_rotations = 0u;
    std::vector<std::string> trace;

    void Rotate(pal4::CameraDataState& state,
                std::uint32_t matrix_offset,
                const std::array<float, 3>& axis,
                float degrees, std::uint32_t mode) override {
        if (matrix_offset == 0x128u) {
            Check(axis == std::array<float, 3>{0.0f, 1.0f, 0.0f} &&
                      degrees == 45.0f && mode == 0u,
                  "secondary matrix Y-axis rotation operands");
            Check(state.At(0x128u) == std::bit_cast<std::uint32_t>(1.0f) &&
                      state.At(0x13cu) == std::bit_cast<std::uint32_t>(1.0f) &&
                      state.At(0x150u) == std::bit_cast<std::uint32_t>(1.0f) &&
                      state.At(0x134u) == 0x20083u,
                  "secondary matrix identity and flags before rotation");
        } else {
            Check(matrix_offset == 0xa8u && state.At(0xb4u) == 0x20083u,
                  "primary matrix flags before rotations");
            const std::array<std::array<float, 3>, 3> axes{{
                {1.0f, 0.0f, 0.0f},
                {0.0f, 1.0f, 0.0f},
                {0.0f, 0.0f, 1.0f}}};
            const std::array<float, 3> angles{20.0f, 10.0f, 30.0f};
            const std::array<std::uint32_t, 3> modes{0u, 2u, 2u};
            Check(primary_rotations < 3u &&
                      axis == axes[primary_rotations] &&
                      degrees == angles[primary_rotations] &&
                      mode == modes[primary_rotations],
                  "primary matrix X/Y/Z rotation order");
            trace.emplace_back("rotate:" +
                               std::to_string(primary_rotations));
            if (primary_rotations == 2u) {
                state.At(0xc8u) = std::bit_cast<std::uint32_t>(0.0f);
                state.At(0xccu) = std::bit_cast<std::uint32_t>(0.6f);
                state.At(0xd0u) = std::bit_cast<std::uint32_t>(0.8f);
            }
            ++primary_rotations;
        }
        ++calls;
    }
    std::uint32_t CameraFrame(std::uint32_t camera) override {
        Check(camera == 0x1000u, "matrix camera frame receiver");
        trace.emplace_back("frame");
        return 0x2000u;
    }
    void SetFrameIdentity(std::uint32_t frame) override {
        Check(frame == 0x2000u, "matrix frame identity receiver");
        trace.emplace_back("identity");
    }
    void ApplyTransform(std::uint32_t frame,
                        pal4::CameraDataState& state,
                        std::uint32_t matrix_offset,
                        std::uint32_t mode) override {
        Check(frame == 0x2000u && matrix_offset == 0xa8u &&
                  mode == 0u &&
                  state.At(0xd8u) == state.At(0x68u) &&
                  state.At(0xdcu) == state.At(0x6cu) &&
                  state.At(0xe0u) == state.At(0x70u),
              "matrix translation copies eye before native apply");
        trace.emplace_back("apply");
    }
    void UpdateFrameObjects(std::uint32_t frame) override {
        Check(frame == 0x2000u, "frame object update receiver");
        trace.emplace_back("update frame");
    }
    std::uint32_t CalculateBoundingBox(
        pal4::CameraDataState& state, std::uint32_t bounds_offset,
        const pal4::CameraBoundsVertices& vertices,
        const pal4::CameraBoundsMatrix& transform,
        std::uint32_t camera) override {
        Check(state.At(0x50u) == std::bit_cast<std::uint32_t>(0.0f) &&
                  state.At(0x54u) == std::bit_cast<std::uint32_t>(0.6f) &&
                  state.At(0x58u) == std::bit_cast<std::uint32_t>(0.8f) &&
                  state.At(0x1c0u) == state.At(0x74u) &&
                  std::bit_cast<float>(state.At(0x1d4u)) ==
                      std::bit_cast<float>(state.At(0x7cu)) - 4.0f &&
                  bounds_offset == 0x230u && camera == 0x1000u &&
                  vertices[0][0] == std::bit_cast<float>(state.At(0x1c0u)) &&
                  transform[3] == 0x20003u,
              "camera basis and frustum vertices reach bounding-box call");
        trace.emplace_back("prepare");
        return 0x88u;
    }
};

void Cases() {
    Runtime runtime;
    pal4::CameraBounds bounds(runtime);
    pal4::CameraMatrix matrix(runtime, bounds);
    pal4::CameraDataState state;
    state.words.fill(0xdeadbeefu);
    state.At(0) = 0u;
    Check(matrix.Initialize(state) == nullptr && runtime.calls == 0u &&
              state.At(0x128u) == 0xdeadbeefu,
          "absent camera does not modify matrix");

    state.At(0) = 0x1000u;
    state.At(0x3cu) = std::bit_cast<std::uint32_t>(45.0f);
    state.At(0x134u) = 0x80u;
    Check(matrix.Initialize(state) == &state.At(0x128u) &&
              runtime.calls == 1u &&
              state.At(0x138u) == 0u && state.At(0x130u) == 0u &&
              state.At(0x12cu) == 0u && state.At(0x14cu) == 0u &&
              state.At(0x148u) == 0u && state.At(0x140u) == 0u &&
              state.At(0x160u) == 0u && state.At(0x15cu) == 0u &&
              state.At(0x158u) == 0u &&
              state.At(0x164u) == 0xdeadbeefu,
          "matrix initializer zeros observed off-diagonal fields only");
}

void UpdateCases() {
    Runtime runtime;
    pal4::CameraBounds bounds(runtime);
    pal4::CameraMatrix matrix(runtime, bounds);
    pal4::CameraDataState state;
    state.At(0) = 0x1000u;
    state.At(0xb4u) = 0x80u;
    state.At(0x3cu) = std::bit_cast<std::uint32_t>(10.0f);
    state.At(0x40u) = std::bit_cast<std::uint32_t>(20.0f);
    state.At(0x44u) = std::bit_cast<std::uint32_t>(30.0f);
    state.At(0x48u) = std::bit_cast<std::uint32_t>(5.0f);
    state.At(0x74u) = std::bit_cast<std::uint32_t>(10.0f);
    state.At(0x78u) = std::bit_cast<std::uint32_t>(20.0f);
    state.At(0x7cu) = std::bit_cast<std::uint32_t>(30.0f);

    Check(matrix.Update(state, 1u) == 0x88u &&
              std::bit_cast<float>(state.At(0x68u)) == 10.0f &&
              std::bit_cast<float>(state.At(0x6cu)) == 17.0f &&
              std::bit_cast<float>(state.At(0x70u)) == 26.0f &&
              runtime.trace == std::vector<std::string>{
                  "rotate:0", "rotate:1", "rotate:2", "frame",
                  "identity", "apply", "update frame",
                  "prepare"},
          "solve-eye mode subtracts rotated at-vector times distance");

    runtime.trace.clear();
    runtime.primary_rotations = 0u;
    state.At(0x68u) = std::bit_cast<std::uint32_t>(1.0f);
    state.At(0x6cu) = std::bit_cast<std::uint32_t>(2.0f);
    state.At(0x70u) = std::bit_cast<std::uint32_t>(3.0f);
    Check(matrix.Update(state, 0u) == 0x88u &&
              std::bit_cast<float>(state.At(0x74u)) == 1.0f &&
              std::bit_cast<float>(state.At(0x78u)) == 5.0f &&
              std::bit_cast<float>(state.At(0x7cu)) == 7.0f &&
              state.At(0x5cu) == state.At(0xa8u) &&
              state.At(0x80u) == state.At(0xb8u),
          "solve-target mode adds rotated at-vector and copies basis slots");

    runtime.trace.clear();
    state.At(0) = 0u;
    Check(!matrix.Update(state, 1u).has_value() && runtime.trace.empty(),
          "absent camera skips primary matrix update");
}

}  // namespace

int main() {
    try {
        Cases();
        UpdateCases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
