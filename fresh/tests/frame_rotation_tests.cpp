#include "pal4/frame_rotation.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IFrameRotationRuntime {
public:
    explicit Runtime(pal4::FrameRotationState& frame) : frame_(frame) {}

    void ClampAxis(float* axis) override {
        events.push_back("clamp" + std::to_string(Index(axis)));
        axis[0] += 100.0f;
    }
    void Translate(pal4::FrameRotationState& frame,
                   const std::array<float, 3>& offset,
                   unsigned mode) override {
        Check(&frame == &frame_ && mode == 2, "translation frame/mode");
        offsets.push_back(offset);
        events.push_back("translate");
    }
    void Rotate(pal4::FrameRotationState& frame, float* axis,
                float degrees, unsigned mode) override {
        Check(&frame == &frame_ && mode == 2, "rotation frame/mode");
        const auto index = Index(axis);
        Check(degrees == static_cast<float>(index + 10), "angle order");
        Check(axis[0] == static_cast<float>(index + 101), "clamped axis used");
        events.push_back("rotate" + std::to_string(index));
    }
    void UpdateObjects(pal4::FrameRotationState& frame) override {
        Check(&frame == &frame_, "update frame");
        events.push_back("update");
    }
    int CalculateAngles(const float* axis,
                        std::array<float, 3>& output) override {
        Check(axis == frame_.Axis(0), "first axis passed to angle helper");
        output = {7.0f, 8.0f, 9.0f};
        events.push_back("angles");
        return 53;
    }

    std::vector<std::string> events;
    std::vector<std::array<float, 3>> offsets;

private:
    std::size_t Index(const float* axis) const {
        for (std::size_t i = 0; i < 3; ++i)
            if (axis == frame_.words.data() + 4 + i * 4)
                return i;
        Check(false, "axis address");
        return 0;
    }

    pal4::FrameRotationState& frame_;
};

}  // namespace

int main() {
    pal4::FrameRotationState frame;
    frame.words[4] = 1.0f;
    frame.words[8] = 2.0f;
    frame.words[12] = 3.0f;
    frame.words[16] = 4.0f;
    frame.words[17] = -5.0f;
    frame.words[18] = 6.0f;
    Runtime runtime(frame);
    std::array<float, 3> output{};
    Check(pal4::FrameRotation::Update(frame, {10.0f, 11.0f, 12.0f},
                                      output, runtime) == 53,
          "forwards angle-helper result");
    Check(runtime.events == std::vector<std::string>{
              "clamp1", "clamp0", "clamp2", "translate", "rotate0",
              "rotate1", "rotate2", "translate", "update", "angles"},
          "native operation order");
    Check(runtime.offsets ==
              std::vector<std::array<float, 3>>{{-4.0f, 5.0f, -6.0f},
                                                 {4.0f, -5.0f, 6.0f}},
          "position is copied before axis changes and restored afterward");
    Check(output == std::array<float, 3>{7.0f, 8.0f, 9.0f} &&
              frame.words[16] == 4.0f,
          "output and original position retained");
}
