#include "pal4/sound3d_properties.h"

#include <array>
#include <bit>
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

struct Runtime final : pal4::ISound3DPropertyRuntime {
    std::vector<std::string> events;
    std::array<float, 3> expected_position{};

    std::uint32_t GetFrame(pal4::Sound3DObjectState& state) override {
        for (std::uint32_t i = 0u; i < 3u; ++i)
            Check(state.words[10u + i] ==
                      std::bit_cast<std::uint32_t>(expected_position[i]),
                  "position written before virtual frame lookup");
        events.emplace_back("get-frame");
        return 0x5000u;
    }
    void SetFrameIdentity(std::uint32_t frame) override {
        Check(frame == 0x5000u, "frame identity target");
        events.emplace_back("identity");
    }
    void ScaleFrame(std::uint32_t frame,
                    const std::array<float, 3>& scale,
                    std::uint32_t mode) override {
        Check(frame == 0x5000u && scale == std::array<float, 3>{1, 1, 1} &&
                  mode == 0u, "unit frame scale");
        events.emplace_back("scale");
    }
    void RotateFrame(std::uint32_t frame,
                     const std::array<float, 3>& axis,
                     float degrees, std::uint32_t mode) override {
        constexpr std::array<std::array<float, 3>, 3> axes{{
            {1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
        const auto index = events.size() - 3u;
        Check(frame == 0x5000u && index < axes.size() &&
                  axis == axes[index] && degrees == 0.0f && mode == 2u,
              "ordered axis rotations");
        events.emplace_back("rotate");
    }
    std::uint32_t TranslateFrame(std::uint32_t frame,
                                 const std::array<float, 3>& position,
                                 std::uint32_t mode) override {
        Check(frame == 0x5000u && position == expected_position && mode == 2u,
              "frame translation uses updated position");
        events.emplace_back("translate");
        return 0x12345678u;
    }
};

void TestGetPosition() {
    pal4::Sound3DObjectState state{};
    state.words[10u] = 0x11111111u;
    state.words[11u] = 0x22222222u;
    state.words[12u] = 0x3f80007du;
    std::uint32_t x = 4u, y = 5u, z = 6u;
    Check(pal4::Sound3DProperties::GetPosition(state, x, y, z) == 0u &&
              x == 4u && y == 5u && z == 6u,
          "inactive getter preserves outputs");
    state.words[1u] = 1u;
    Check(pal4::Sound3DProperties::GetPosition(state, x, y, z) == 0x7du &&
              x == state.words[10u] && y == state.words[11u] &&
              z == state.words[12u],
          "active getter copies raw words and returns Z low byte");
}

void TestSetPosition() {
    pal4::Sound3DObjectState state{};
    Runtime runtime;
    Check(pal4::Sound3DProperties::SetPosition(
              state, 1.0f, 2.0f, 3.0f, 0u, runtime) == 0u &&
              runtime.events.empty(),
          "inactive setter skips frame and position writes");
    state.words[1u] = 1u;
    runtime.expected_position = {1.0f, 2.0f, 3.0f};
    Check(pal4::Sound3DProperties::SetPosition(
              state, 1.0f, 2.0f, 3.0f, 0u, runtime) == 0x12345678u,
          "absolute setter returns native frame translation result");
    Check(runtime.events == std::vector<std::string>{
              "get-frame", "identity", "scale", "rotate", "rotate",
              "rotate", "translate"},
          "absolute frame reconstruction order");
    runtime.events.clear();
    runtime.expected_position = {1.5f, 1.0f, 5.0f};
    Check(pal4::Sound3DProperties::SetPosition(
              state, 0.5f, -1.0f, 2.0f, 2u, runtime) == 0x12345678u &&
              runtime.events.size() == 7u,
          "nonzero mode adds per-axis positions and rebuilds frame");
}

}  // namespace

int main() {
    TestGetPosition();
    TestSetPosition();
}
