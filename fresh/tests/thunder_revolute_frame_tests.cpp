#include "pal4/thunder_revolute_frame.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(const bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IThunderRevoluteRuntime {
    std::vector<std::string> calls;
    std::array<float, 3u> current{8.0f, -8.0f, 3.0f};
    std::array<float, 3u> output{};

    std::uint32_t GetFrame(pal4::ThunderObjectState&) override {
        calls.emplace_back("frame");
        return 0x1000u;
    }
    void TranslateMatrix(const std::uint32_t matrix,
                         const std::array<float, 3u>& translation,
                         const std::uint32_t mode) override {
        Check(matrix == 0x1010u && mode == 2u, "matrix translation ABI");
        Check(translation == (calls.size() == 1u
                                  ? std::array<float, 3u>{-5.0f, 5.0f, -3.0f}
                                  : std::array<float, 3u>{5.0f, -5.0f, 3.0f}),
              "pivot translation direction");
        calls.emplace_back("translate");
    }
    void RotateMatrix(const std::uint32_t matrix,
                      const std::array<float, 3u>& axis,
                      const float degrees, const std::uint32_t mode) override {
        const auto i = calls.size() - 2u;
        Check(matrix == 0x1010u && mode == 2u &&
                  axis[i] == 1.0f && degrees == static_cast<float>(i + 1u),
              "axis, angle and rotation mode");
        calls.emplace_back("rotate");
    }
    void UpdateFrameObjects(const std::uint32_t frame) override {
        Check(frame == 0x1000u, "frame update address");
        calls.emplace_back("update");
    }
    float ReadFloat(const std::uint32_t address) override {
        const auto i = (address - 0x1040u) / 4u;
        Check(address == 0x1040u + 4u * i && i < 3u,
              "frame translation read address");
        calls.emplace_back("read");
        return current[i];
    }
    std::uint32_t SetPosition(pal4::ThunderObjectState&,
                              const std::array<float, 3u>& position,
                              const std::uint32_t relative) override {
        Check(relative == 0u, "revolute setter uses absolute position");
        calls.emplace_back("position");
        output = position;
        return 0x87654321u;
    }
};

}  // namespace

int main() {
    pal4::ThunderObjectState state{};
    Runtime runtime;
    const std::array<float, 3u> pivot{5.0f, -5.0f, 3.0f};
    const std::array<float, 3u> rotation{1.0f, 2.0f, 3.0f};
    const std::array<float, 3u> step{2.0f, 4.0f, 1.0f};
    Check(pal4::ThunderRevoluteFrame::Apply(
              state, pivot, rotation, step, 0x13572468u, runtime) ==
              0x13572400u && runtime.calls.empty(),
          "inactive facet clears only AL and makes no native calls");

    state.words[1u] = 1u;
    Check(pal4::ThunderRevoluteFrame::Apply(
              state, pivot, rotation, step, 0u, runtime) == 0x87654321u &&
              runtime.output == std::array<float, 3u>{6.0f, -5.0f, 3.0f} &&
              runtime.calls == std::vector<std::string>{
                                   "frame", "translate", "rotate", "rotate",
                                   "rotate", "translate", "update", "read",
                                   "read", "read", "position"},
          "transform order, axis clamp and setter return");

    runtime.calls.clear();
    runtime.current = {4.0f, -7.0f, 3.0f};
    Check(pal4::ThunderRevoluteFrame::Apply(
              state, pivot, rotation, step, 0u, runtime) == 0x87654321u &&
              runtime.output == std::array<float, 3u>{5.0f, -5.0f, 3.0f},
          "below-target and equal axes clamp independently");
}
