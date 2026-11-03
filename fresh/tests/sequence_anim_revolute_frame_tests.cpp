#include "pal4/sequence_anim_revolute_frame.h"

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

struct Runtime final : pal4::ISequenceAnimRevoluteRuntime {
    std::vector<std::string> events;
    std::array<float, 3> current{14.0f, -25.0f, 4.0f};
    std::array<float, 3> next{};
    std::uint32_t GetFrame(pal4::SequenceAnimObjectState&) override {
        events.emplace_back("frame");
        return 0x4000u;
    }
    void TranslateMatrix(std::uint32_t matrix,
                         const std::array<float, 3>& vector,
                         std::uint32_t mode) override {
        Check(matrix == 0x4010u && mode == 2u,
              "matrix translation target and mode");
        if (events.size() == 1u)
            Check(vector == std::array<float, 3>{-10.0f, 20.0f, -5.0f},
                  "translation to negative pivot");
        else
            Check(vector == std::array<float, 3>{10.0f, -20.0f, 5.0f},
                  "translation back to pivot");
        events.emplace_back("translate");
    }
    void RotateMatrix(std::uint32_t matrix,
                      const std::array<float, 3>& axis,
                      float angle, std::uint32_t mode) override {
        Check(matrix == 0x4010u && mode == 2u,
              "matrix rotation target and mode");
        const auto index = events.size() - 2u;
        constexpr std::array<std::array<float, 3>, 3> axes{{
            {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 1.0f}}};
        constexpr std::array<float, 3> angles{90.0f, 45.0f, 30.0f};
        Check(index < 3u && axis == axes[index] && angle == angles[index],
              "IDA X/Y/Z axes and rotation order");
        events.emplace_back("rotate");
    }
    void UpdateFrameObjects(std::uint32_t frame) override {
        Check(frame == 0x4000u, "frame object update handle");
        events.emplace_back("update-frame");
    }
    float ReadFloat(std::uint32_t address) override {
        Check(address >= 0x4040u && address <= 0x4048u &&
                  (address - 0x4040u) % 4u == 0u,
              "frame position read addresses");
        events.emplace_back("read");
        return current[(address - 0x4040u) / 4u];
    }
    std::uint32_t SetPosition(
        pal4::SequenceAnimObjectState&,
        const std::array<float, 3>& position,
        std::uint32_t mode) override {
        Check(mode == 0u, "position facet absolute mode");
        next = position;
        events.emplace_back("set-position");
        return 0x12345678u;
    }
};

}  // namespace

int main() {
    pal4::SequenceAnimObjectState state{};
    Runtime runtime;
    const std::array<float, 3> pivot{10.0f, -20.0f, 5.0f};
    const std::array<float, 3> rotation{90.0f, 45.0f, 30.0f};
    const std::array<float, 3> step{2.0f, 10.0f, 2.0f};
    Check(pal4::SequenceAnimRevoluteFrame::Apply(
              state, pivot, rotation, step,
              0xabcdef55u, runtime) == 0xabcdef00u &&
              runtime.events.empty(),
          "inactive owner clears entry EAX low byte");
    state.words[1u] = 1u;
    Check(pal4::SequenceAnimRevoluteFrame::Apply(
              state, pivot, rotation, step,
              0xabcdef55u, runtime) == 0x12345678u &&
              runtime.next == std::array<float, 3>{12.0f, -20.0f, 5.0f},
          "bounded movement and final facet return");
    Check(runtime.events == std::vector<std::string>{
              "frame", "translate", "rotate", "rotate", "rotate",
              "translate", "update-frame", "read", "read", "read",
              "set-position"},
          "matrix, frame and position callback order");
}
