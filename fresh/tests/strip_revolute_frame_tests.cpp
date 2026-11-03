#include "pal4/strip_revolute_frame.h"

#include <array>
#include <bit>
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

struct Runtime final : pal4::IStripRevoluteRuntime {
    pal4::StripObjectState* object = nullptr;
    std::vector<std::string> events;
    std::array<float, 3> current{14.0f, -25.0f, 5.0f};
    std::array<float, 3> expected{12.0f, -20.0f, 5.0f};

    std::uint32_t GetFrame(pal4::StripObjectState& state) override {
        Check(&state == object, "frame getter owner");
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
                  "negative pivot translation");
        else
            Check(vector == std::array<float, 3>{10.0f, -20.0f, 5.0f},
                  "positive pivot translation");
        events.emplace_back("translate");
    }
    void RotateMatrix(std::uint32_t matrix,
                      const std::array<float, 3>& axis,
                      float angle, std::uint32_t mode) override {
        constexpr std::array<std::array<float, 3>, 3> axes{{
            {1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
        constexpr std::array<float, 3> angles{90.0f, 45.0f, 30.0f};
        const auto index = events.size() - 2u;
        Check(matrix == 0x4010u && mode == 2u && index < 3u &&
                  axis == axes[index] && angle == angles[index],
              "native X/Y/Z rotation order");
        events.emplace_back("rotate");
    }
    void UpdateFrameObjects(std::uint32_t frame) override {
        Check(frame == 0x4000u, "frame object update target");
        events.emplace_back("update-frame");
    }
    float ReadFloat(std::uint32_t address) override {
        Check(address >= 0x4040u && address <= 0x4048u &&
                  (address - 0x4040u) % 4u == 0u,
              "frame position address");
        Check(object->words[22u] == 0xaaaaaaaau &&
                  object->words[23u] == 0xaaaaaaaau &&
                  object->words[24u] == 0xaaaaaaaau,
              "all frame positions read before object writes");
        events.emplace_back("read");
        return current[(address - 0x4040u) / 4u];
    }
    std::uint32_t RefreshPosition(
        pal4::StripObjectState& state,
        const std::array<float, 3>& zero_delta,
        std::uint32_t mode) override {
        Check(&state == object &&
                  zero_delta == std::array<float, 3>{0, 0, 0} && mode == 1u,
              "position facet called with zero relative delta");
        for (std::uint32_t i = 0u; i < 3u; ++i)
            Check(state.words[22u + i] ==
                      std::bit_cast<std::uint32_t>(expected[i]),
                  "bounded position written before facet dispatch");
        events.emplace_back("refresh-position");
        return 0x12345678u;
    }
};

void TestApply(const std::array<float, 3>& current,
               const std::array<float, 3>& step,
               const std::array<float, 3>& expected) {
    pal4::StripObjectState state{};
    state.words.fill(0xaaaaaaaau);
    state.words[1u] &= 0xffffff00u;
    Runtime runtime;
    runtime.object = &state;
    runtime.current = current;
    runtime.expected = expected;
    const std::array<float, 3> pivot{10.0f, -20.0f, 5.0f};
    const std::array<float, 3> rotation{90.0f, 45.0f, 30.0f};
    Check(pal4::StripRevoluteFrame::Apply(
              state, pivot, rotation, step, 0xabcdef55u, runtime) ==
              0xabcdef00u && runtime.events.empty(),
          "inactive owner clears entry EAX low byte");
    state.words[1u] = 0xaaaaaa01u;
    Check(pal4::StripRevoluteFrame::Apply(
              state, pivot, rotation, step, 0xabcdef55u, runtime) ==
              0x12345678u,
          "active return comes from position facet");
    Check(runtime.events == std::vector<std::string>{
              "frame", "translate", "rotate", "rotate", "rotate",
              "translate", "update-frame", "read", "read", "read",
              "refresh-position"},
          "matrix, frame, position and dispatch order");
}

}  // namespace

int main() {
    TestApply({14.0f, -25.0f, 5.0f}, {2.0f, 10.0f, 2.0f},
              {12.0f, -20.0f, 5.0f});
    TestApply({14.0f, -25.0f, 4.0f}, {10.0f, 2.0f, 2.0f},
              {10.0f, -23.0f, 5.0f});
}
