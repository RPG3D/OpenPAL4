#include "pal4/thunder_visual_transform.h"

#include <bit>
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

void Float(pal4::ThunderObjectState& state,
           std::uint32_t offset, float value) {
    state.words[(offset + 60u) / 4u] =
        std::bit_cast<std::uint32_t>(value);
}

struct Runtime final : pal4::IThunderSourceFrameRuntime {
    std::vector<std::string> calls;
    pal4::ThunderVector3 scale{}, position{};
    std::vector<pal4::ThunderVector3> axes;
    std::vector<float> angles;
    void FrameSetIdentity(std::uint32_t frame) override {
        Check(frame == 0x800u, "source frame identity");
        calls.push_back("identity");
    }
    void ScaleFrame(std::uint32_t frame, pal4::ThunderVector3 value,
                    std::uint32_t combine) override {
        Check(frame == 0x800u && combine == 0u,
              "scale replaces source transform");
        calls.push_back("scale");
        scale = value;
    }
    void RotateFrame(std::uint32_t frame, pal4::ThunderVector3 axis,
                     float angle, std::uint32_t combine) override {
        Check(frame == 0x800u && combine == 2u,
              "rotation combines with source transform");
        calls.push_back("rotate");
        axes.push_back(axis);
        angles.push_back(angle);
    }
    void TranslateFrame(std::uint32_t frame,
                        pal4::ThunderVector3 value,
                        std::uint32_t combine) override {
        Check(frame == 0x800u && combine == 2u,
              "translation combines with source transform");
        calls.push_back("translate");
        position = value;
    }
    std::uint32_t FrameUpdateObjects(std::uint32_t frame) override {
        Check(frame == 0x800u, "source frame objects updated");
        calls.push_back("update");
        return 0x55u;
    }
};

}  // namespace

int main() {
    pal4::ThunderObjectState state;
    state.words[0u] = 0xdeadbeefu;
    Float(state, 12u, -1.0f);
    Float(state, 16u, 2.0f);
    Float(state, 20u, -3.0f);
    Runtime runtime;
    Check(pal4::ThunderVisualTransform::SetupSourceFrame(
              state, runtime) == 0u && runtime.calls.empty() &&
              std::bit_cast<float>(state.words[18u]) == -1.0f,
          "missing source frame leaves scale and runtime untouched");

    state.words[28u] = 0x800u;
    Float(state, 24u, 10.0f);
    Float(state, 28u, 20.0f);
    Float(state, 32u, 30.0f);
    Float(state, 36u, 4.0f);
    Float(state, 40u, 5.0f);
    Float(state, 44u, 6.0f);
    Check(pal4::ThunderVisualTransform::SetupSourceFrame(
              state, runtime) == 0x55u,
          "frame update result forwarded");
    Check(state.words[18u] == 0u && state.words[20u] == 0u &&
              std::bit_cast<float>(state.words[19u]) == 2.0f &&
              state.words[0u] == 0xdeadbeefu,
          "negative scale fields clamped without touching outer header");
    Check(runtime.calls == std::vector<std::string>{
              "identity", "scale", "rotate", "rotate", "rotate",
              "translate", "update"}, "native transform call order");
    Check(runtime.scale == pal4::ThunderVector3{0.0f, 2.0f, 0.0f} &&
              runtime.axes == std::vector<pal4::ThunderVector3>{
                  {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
                  {0.0f, 0.0f, 1.0f}} &&
              runtime.angles == std::vector<float>{10.0f, 20.0f, 30.0f} &&
              runtime.position == pal4::ThunderVector3{4.0f, 5.0f, 6.0f},
          "clamped scale, XYZ axes/angles and position forwarded");

    pal4::ThunderSourceFrameRuntimeStub stub;
    Check(pal4::ThunderVisualTransform::SetupSourceFrame(
              state, stub) == 0u,
          "RenderWare source-frame stub is linkable");
}
