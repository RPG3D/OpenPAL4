#include "pal4/thunder_visual_transform.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool good, const char* message) {
    if (!good) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Equal(pal4::ThunderVector3 left, pal4::ThunderVector3 right) {
    return left == right;
}

struct Runtime final : pal4::IThunderVisualTransformRuntime {
    std::vector<std::string> calls;
    pal4::ThunderFrameMatrix applied{};
    std::uint32_t FrameTransform(std::uint32_t frame) override {
        calls.push_back("transform:" + std::to_string(frame));
        Check(frame == 0x500u || frame == 0x700u,
              "source or camera frame queried");
        return frame == 0x500u ? 0x600u : 0x710u;
    }
    pal4::ThunderVector3 MatrixPosition(
        std::uint32_t transform) override {
        calls.push_back("position");
        Check(transform == 0x600u || transform == 0x710u,
              "source or camera matrix position");
        return transform == 0x600u
                   ? pal4::ThunderVector3{1.0f, 2.0f, 3.0f}
                   : pal4::ThunderVector3{1.0f, 2.0f, 7.0f};
    }
    pal4::ThunderVector3 TransformPoint(
        pal4::ThunderVector3 point, std::uint32_t transform) override {
        Check(transform == 0x600u &&
                  Equal(point, {4.0f, 5.0f, 6.0f}),
              "direct mode transforms authored target");
        calls.push_back("point-direct");
        return {1.0f, 6.0f, 3.0f};
    }
    pal4::ThunderFrameMatrix BuildTransformMatrix(
        std::uint32_t transform) override {
        Check(transform == 0x600u, "local mode reads source transform");
        calls.push_back("build-matrix");
        return {};
    }
    void MultiplyMatrix(pal4::ThunderFrameMatrix&,
                        std::uint32_t frame_matrix) override {
        Check(frame_matrix == 0x510u,
              "local mode multiplies frame matrix at +16");
        calls.push_back("multiply");
    }
    pal4::ThunderVector3 TransformPoint(
        pal4::ThunderVector3 point,
        const pal4::ThunderFrameMatrix&) override {
        Check(Equal(point, {4.0f, 5.0f, 6.0f}),
              "local mode transforms authored target");
        calls.push_back("point-local");
        return {1.0f, 6.0f, 3.0f};
    }
    float VectorLength(pal4::ThunderVector3 value) override {
        Check(Equal(value, {0.0f, 4.0f, 0.0f}),
              "length of target minus source");
        calls.push_back("length");
        return 4.0f;
    }
    pal4::ThunderVector3 Normalize(pal4::ThunderVector3 value) override {
        calls.push_back("normalize");
        const float length = std::sqrt(value[0] * value[0] +
                                       value[1] * value[1] +
                                       value[2] * value[2]);
        if (length == 0.0f) return {};
        return {value[0] / length, value[1] / length,
                value[2] / length};
    }
    std::uint32_t CameraFrame() override {
        calls.push_back("camera-frame");
        return 0x700u;
    }
    void MatrixUpdate(pal4::ThunderFrameMatrix& matrix) override {
        Check(Equal(matrix.right, {1.0f, 0.0f, 0.0f}) &&
                  Equal(matrix.up, {0.0f, 1.0f, 0.0f}) &&
                  Equal(matrix.at, {0.0f, 0.0f, 1.0f}) &&
                  Equal(matrix.position, {0.0f, 0.0f, 0.0f}) &&
                  matrix.flags == 0x20003u,
              "matrix axes, zero position and native flags");
        calls.push_back("matrix-update");
    }
    void FrameSetIdentity(std::uint32_t frame) override {
        Check(frame == 0x800u, "output frame reset");
        calls.push_back("identity");
    }
    void ApplyMatrix(std::uint32_t frame,
                     const pal4::ThunderFrameMatrix& matrix,
                     std::uint32_t combine) override {
        Check(frame == 0x800u && combine == 0u,
              "output frame matrix replaces transform");
        calls.push_back("apply");
        applied = matrix;
    }
    std::uint32_t FrameUpdateObjects(std::uint32_t frame) override {
        Check(frame == 0x800u, "output frame objects updated");
        calls.push_back("update-objects");
        return 0x1234u;
    }
};

pal4::ThunderObjectState State(bool direct) {
    pal4::ThunderObjectState state;
    state.words[0u] = 0xdeadbeefu;
    state.words[28u] = 0x500u;
    state.words[34u] = 0x800u;
    state.words[15u] = direct ? 0x00010000u : 0u;
    state.words[37u] = std::bit_cast<std::uint32_t>(4.0f);
    state.words[38u] = std::bit_cast<std::uint32_t>(5.0f);
    state.words[39u] = std::bit_cast<std::uint32_t>(6.0f);
    return state;
}

void Run(bool direct) {
    Runtime runtime;
    auto state = State(direct);
    Check(pal4::ThunderVisualTransform::Update(state, runtime) == 0x1234u,
          "frame update result forwarded");
    Check(std::bit_cast<float>(state.words[30u]) == 4.0f &&
              std::bit_cast<float>(state.words[31u]) == 1.0f &&
              std::bit_cast<float>(state.words[32u]) == 4.0f &&
              std::bit_cast<float>(state.words[33u]) == 3.0f,
          "length and midpoint stored at +60/+64");
    Check(Equal(runtime.applied.right, {1.0f, 0.0f, 0.0f}) &&
              Equal(runtime.applied.up, {0.0f, 1.0f, 0.0f}) &&
              Equal(runtime.applied.at, {0.0f, 0.0f, 1.0f}),
          "final frame receives calculated axes");
    Check(state.words[0u] == 0xdeadbeefu,
          "outer Thunder object header is untouched");
    std::vector<std::string> expected{
        "transform:1280", "position", "transform:1280"};
    if (direct) {
        expected.push_back("point-direct");
    } else {
        expected.insert(expected.end(),
                        {"build-matrix", "multiply", "point-local"});
    }
    expected.insert(expected.end(),
                    {"length", "normalize", "normalize", "camera-frame",
                     "transform:1792", "position", "normalize", "normalize",
                     "matrix-update", "identity", "apply", "update-objects"});
    Check(runtime.calls == expected, "native transform call order");
}

}  // namespace

int main() {
    Run(true);
    Run(false);
    auto state = State(true);
    pal4::ThunderVisualTransformRuntimeStub stub;
    Check(pal4::ThunderVisualTransform::Update(state, stub) == 0u,
          "RenderWare stub is linkable");
}
