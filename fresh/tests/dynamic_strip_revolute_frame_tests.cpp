#include "pal4/dynamic_strip_revolute_frame.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IDynamicStripRevoluteFrameRuntime {
    std::vector<std::string> calls;
    std::uint32_t frame = 0x2000u;
    std::array<float, 3> frame_position{12.0f, -5.0f, 30.0f};
    std::array<float, 3> final_position{};

    std::uint32_t GetFrame(
        const pal4::DynamicStripObjectState&) override {
        calls.push_back("frame");
        return frame;
    }
    void TranslateMatrix(std::uint32_t matrix,
                         const std::array<float, 3>& vector,
                         std::uint32_t mode) override {
        Check(matrix == frame + 16u && mode == 2u,
              "RenderWare translation uses frame matrix and mode two");
        if (calls.size() == 1u)
            Check(vector == std::array<float, 3>{-10.0f, 10.0f, -20.0f},
                  "first translation uses negative target");
        else
            Check(vector == std::array<float, 3>{10.0f, -10.0f, 20.0f},
                  "second translation restores target");
        calls.push_back("translate");
    }
    void RotateMatrix(std::uint32_t matrix,
                      const std::array<float, 3>& axis,
                      float angle, std::uint32_t mode) override {
        Check(matrix == frame + 16u && mode == 2u,
              "RenderWare rotation uses frame matrix and mode two");
        const auto index = calls.size() - 2u;
        const std::array<std::array<float, 3>, 3> axes{{
            {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}};
        Check(index < 3u && axis == axes[index] &&
                  angle == static_cast<float>(index + 1u),
              "X, Y, Z axes and rotation values preserve native order");
        calls.push_back("rotate");
    }
    void UpdateFrameObjects(std::uint32_t input) override {
        Check(input == frame, "frame update receives frame start");
        calls.push_back("update");
    }
    float ReadFloat(std::uint32_t address) override {
        Check(address >= frame + 64u && address <= frame + 72u &&
                  (address - frame - 64u) % 4u == 0u,
              "frame position is read from offsets 64, 68, 72");
        calls.push_back("read");
        return frame_position[(address - frame - 64u) / 4u];
    }
    std::uint32_t SetPosition(
        pal4::DynamicStripObjectState& state,
        const std::array<float, 3>& values,
        std::uint32_t mode) override {
        Check((state.words[1u] & 0xffu) != 0u && mode == 0u,
              "position facet receives active owner and absolute mode");
        final_position = values;
        calls.push_back("scale");
        return 0xabcdef01u;
    }
};

void TestApply() {
    pal4::DynamicStripObjectState state{};
    Runtime runtime;
    const std::array<float, 3> target{10.0f, -10.0f, 20.0f};
    const std::array<float, 3> rotation{1.0f, 2.0f, 3.0f};
    const std::array<float, 3> step{1.0f, 10.0f, 2.0f};
    Check(pal4::DynamicStripRevoluteFrame::Apply(
              state, target, rotation, step, runtime) == 0u &&
              runtime.calls.empty(),
          "inactive frame path has no native calls");
    state.words[1u] = 1u;
    Check(pal4::DynamicStripRevoluteFrame::Apply(
              state, target, rotation, step, runtime) == 0xabcdef01u &&
              runtime.calls == std::vector<std::string>{
                  "frame", "translate", "rotate", "rotate", "rotate",
                  "translate", "update", "read", "read", "read", "scale"} &&
              runtime.final_position == std::array<float, 3>{11.0f, -10.0f, 28.0f},
          "active frame path rotates and approaches each axis before scale");
}

void TestEqualAndNan() {
    pal4::DynamicStripObjectState state{};
    state.words[1u] = 1u;
    Runtime runtime;
    runtime.frame_position = {10.0f, -10.0f,
                              std::numeric_limits<float>::quiet_NaN()};
    (void)pal4::DynamicStripRevoluteFrame::Apply(
        state, {10.0f, -10.0f, 20.0f}, {1.0f, 2.0f, 3.0f},
        {1.0f, 1.0f, 1.0f}, runtime);
    Check(runtime.final_position[0u] == 10.0f &&
              runtime.final_position[1u] == -10.0f &&
              std::isnan(runtime.final_position[2u]),
          "equal and unordered axes remain unchanged");
}

}  // namespace

int main() {
    TestApply();
    TestEqualAndNan();
}
