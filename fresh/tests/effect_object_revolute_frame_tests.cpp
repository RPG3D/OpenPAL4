#include "pal4/effect_object_revolute_frame.h"

#include <array>
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

struct Runtime final : pal4::IEffectObjectRevoluteFrameRuntime {
    std::vector<std::string> calls;
    std::array<float, 3u> current{5.0f, 25.0f, 30.0f};
    std::array<float, 3u> output{};
    std::uint32_t axis_index = 0u;

    std::uint32_t GetFrame(pal4::EffectObjectState&) override {
        calls.emplace_back("frame");
        return 0x4000u;
    }
    void TranslateMatrix(const std::uint32_t matrix,
                         const std::array<float, 3u>& vector,
                         const std::uint32_t mode) override {
        Check(matrix == 0x4010u && mode == 2u, "matrix translation ABI");
        Check(vector == (calls.size() == 1u
                            ? std::array<float, 3u>{-10.0f, -20.0f, -30.0f}
                            : std::array<float, 3u>{10.0f, 20.0f, 30.0f}),
              "translate about supplied pivot");
        calls.emplace_back("translate");
    }
    void RotateMatrix(const std::uint32_t matrix,
                      const std::array<float, 3u>& axis,
                      const float angle, const std::uint32_t mode) override {
        constexpr std::array<std::array<float, 3u>, 3u> axes{{
            {1.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 1.0f}}};
        constexpr std::array<float, 3u> angles{1.0f, 2.0f, 3.0f};
        Check(matrix == 0x4010u && mode == 2u && axis_index < 3u &&
                  axis == axes[axis_index] && angle == angles[axis_index],
              "native X/Y/Z axis rotation order");
        ++axis_index;
        calls.emplace_back("rotate");
    }
    void UpdateFrameObjects(const std::uint32_t frame) override {
        Check(frame == 0x4000u, "frame object update handle");
        calls.emplace_back("update");
    }
    float ReadFloat(const std::uint32_t address) override {
        Check(address >= 0x4040u && address <= 0x4048u &&
                  (address - 0x4040u) % 4u == 0u,
              "frame position offsets");
        calls.emplace_back("read");
        return current[(address - 0x4040u) / 4u];
    }
    std::uint32_t SetPosition(
        pal4::EffectObjectState&,
        const std::array<float, 3u>& values,
        const std::uint32_t mode) override {
        Check(mode == 0u, "position facet absolute mode");
        calls.emplace_back("set_position");
        output = values;
        return 0x12345678u;
    }
};

void TestApply() {
    pal4::EffectObjectState state{};
    Runtime runtime;
    constexpr std::array<float, 3u> target{10.0f, 20.0f, 30.0f};
    constexpr std::array<float, 3u> rotation{1.0f, 2.0f, 3.0f};
    constexpr std::array<float, 3u> step{2.0f, 10.0f, 4.0f};
    Check(pal4::EffectObjectRevoluteFrame::Apply(
              state, target, rotation, step, 0x1234567fu, runtime) ==
              0x12345600u && runtime.calls.empty(),
          "inactive facet preserves entry EAX high bytes");
    state.words[1u] = 1u;
    Check(pal4::EffectObjectRevoluteFrame::Apply(
              state, target, rotation, step, 0u, runtime) == 0x12345678u &&
              runtime.calls == std::vector<std::string>{
                  "frame", "translate", "rotate", "rotate", "rotate",
                  "translate", "update", "read", "read", "read",
                  "set_position"} &&
              runtime.output == std::array<float, 3u>{7.0f, 20.0f, 30.0f},
          "pivot rotation and axis-wise bounded approach");
    runtime.calls.clear();
    runtime.axis_index = 0u;
    runtime.current = {9.0f, 21.0f, 29.0f};
    (void)pal4::EffectObjectRevoluteFrame::Apply(
        state, target, rotation, step, 0u, runtime);
    Check(runtime.output == target,
          "overshooting steps clamp to target on both sides");
}

}  // namespace

int main() {
    TestApply();
}
