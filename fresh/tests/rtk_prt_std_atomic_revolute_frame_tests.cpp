#include "pal4/rtk_prt_std_atomic_revolute_frame.h"

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

struct Runtime final : pal4::IRtkPrtStdAtomicRevoluteRuntime {
    std::vector<std::string> calls;
    std::array<float, 3> current{5.0f, 25.0f, 30.0f};
    std::array<float, 3> position{};
    std::uint32_t rotations = 0u;

    std::uint32_t GetFrame(pal4::RtkPrtStdAtomicObjectState&) override {
        calls.emplace_back("frame");
        return 0x4000u;
    }
    void TranslateMatrix(std::uint32_t matrix,
                         const std::array<float, 3>& value,
                         std::uint32_t mode) override {
        Check(matrix == 0x4010u && mode == 2u &&
                  value == (calls.size() == 1u
                      ? std::array<float, 3>{-10.0f, -20.0f, -30.0f}
                      : std::array<float, 3>{10.0f, 20.0f, 30.0f}),
              "matrix translates around target pivot in mode two");
        calls.emplace_back("translate");
    }
    void RotateMatrix(std::uint32_t matrix,
                      const std::array<float, 3>& axis,
                      float angle, std::uint32_t mode) override {
        constexpr std::array<std::array<float, 3>, 3> axes{{
            {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 1.0f}}};
        constexpr std::array<float, 3> angles{1.0f, 2.0f, 3.0f};
        Check(matrix == 0x4010u && mode == 2u && rotations < 3u &&
                  axis == axes[rotations] && angle == angles[rotations],
              "native X/Y/Z rotation axes and order");
        ++rotations;
        calls.emplace_back("rotate");
    }
    void UpdateFrameObjects(std::uint32_t frame) override {
        Check(frame == 0x4000u, "frame object update handle");
        calls.emplace_back("update");
    }
    float ReadFloat(std::uint32_t address) override {
        Check(address >= 0x4040u && address <= 0x4048u &&
                  (address - 0x4040u) % 4u == 0u,
              "frame position offsets plus 64");
        calls.emplace_back("read");
        return current[(address - 0x4040u) / 4u];
    }
    std::uint32_t SetPosition(
        pal4::RtkPrtStdAtomicObjectState&,
        const std::array<float, 3>& value,
        std::uint32_t mode) override {
        Check(mode == 0u, "position facet absolute mode");
        position = value;
        calls.emplace_back("set-position");
        return 0x12345678u;
    }
};

void TestRevolute() {
    pal4::RtkPrtStdAtomicObjectState state{};
    Runtime runtime;
    constexpr std::array<float, 3> target{10.0f, 20.0f, 30.0f};
    constexpr std::array<float, 3> rotation{1.0f, 2.0f, 3.0f};
    constexpr std::array<float, 3> step{2.0f, 10.0f, 4.0f};
    Check(pal4::RtkPrtStdAtomicRevoluteFrame::Apply(
              state, target, rotation, step, 0xabcdef7fu, runtime) ==
                  0xabcdef00u && runtime.calls.empty(),
          "inactive facet clears only entry EAX low byte");
    state.words[1u] = 1u;
    Check(pal4::RtkPrtStdAtomicRevoluteFrame::Apply(
              state, target, rotation, step, 0u, runtime) == 0x12345678u &&
              runtime.calls == std::vector<std::string>{
                  "frame", "translate", "rotate", "rotate", "rotate",
                  "translate", "update", "read", "read", "read",
                  "set-position"} &&
              runtime.position == std::array<float, 3>{7.0f, 20.0f, 30.0f},
          "pivot rotation, axis approach and facet result");
    runtime.calls.clear();
    runtime.rotations = 0u;
    runtime.current = {9.0f, 21.0f, 29.0f};
    (void)pal4::RtkPrtStdAtomicRevoluteFrame::Apply(
        state, target, rotation, step, 0u, runtime);
    Check(runtime.position == target,
          "positive and negative overshoot clamps each axis to target");
}

}  // namespace

int main() {
    TestRevolute();
}
