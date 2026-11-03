#include "pal4/strip_frame.h"

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

std::uint32_t Bits(float value) {
    return std::bit_cast<std::uint32_t>(value);
}

struct Runtime final : pal4::IStripFrameRuntime {
    std::vector<std::string> events;
    std::vector<std::uint32_t> modes;
    void SetIdentity(std::uint32_t frame) override {
        Check(frame == 0x4000u, "identity frame handle");
        events.emplace_back("identity");
    }
    void Scale(std::uint32_t frame,
               const std::array<float, 3>& values,
               std::uint32_t mode) override {
        Check(frame == 0x4000u &&
                  values == std::array{2.0f, 3.0f, 4.0f},
              "scale frame and values");
        events.emplace_back("scale");
        modes.push_back(mode);
    }
    void Rotate(std::uint32_t frame,
                const std::array<float, 3>& axis,
                float angle, std::uint32_t mode) override {
        constexpr std::array<std::array<float, 3>, 3> axes{{
            {1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
        constexpr std::array<float, 3> angles{10.0f, 20.0f, 30.0f};
        const auto index = static_cast<std::size_t>(angle / 10.0f - 1.0f);
        Check(frame == 0x4000u && index < 3u && axis == axes[index] &&
                  angle == angles[index],
              "X/Y/Z rotation axes and values");
        events.emplace_back("rotate");
        modes.push_back(mode);
    }
    void Translate(std::uint32_t frame,
                   const std::array<float, 3>& values,
                   std::uint32_t mode) override {
        Check(frame == 0x4000u && mode == 2u &&
                  values == std::array{5.0f, 6.0f, 7.0f},
              "final position and translation mode");
        events.emplace_back("translate");
    }
    std::uint8_t UpdateObjects(std::uint32_t frame) override {
        Check(frame == 0x4000u, "frame object update handle");
        events.emplace_back("update");
        return 0x5au;
    }
};

pal4::StripObjectState State() {
    pal4::StripObjectState state{};
    state.words[1u] = 1u;
    state.words[33u] = 0x4000u;
    for (std::uint32_t i = 0u; i < 3u; ++i) {
        state.words[16u + i] = Bits(2.0f + static_cast<float>(i));
        state.words[19u + i] = Bits(10.0f * (i + 1u));
        state.words[22u + i] = Bits(5.0f + static_cast<float>(i));
    }
    return state;
}

void TestFrame() {
    auto state = State();
    Runtime runtime;
    state.words[1u] = 0u;
    Check(pal4::StripFrame::Rebuild(state, runtime) == 0u &&
              runtime.events.empty(), "inactive frame skips native calls");
    state.words[1u] = 1u;
    Check(pal4::StripFrame::Rebuild(state, runtime) == 0x5au &&
              runtime.events == std::vector<std::string>{
                  "identity", "scale", "rotate", "rotate", "rotate",
                  "translate", "update"} &&
              runtime.modes == std::vector<std::uint32_t>{0u, 2u, 2u, 2u},
          "default frame scales before rotation");

    state.words[15u] = 1u;
    runtime.events.clear();
    runtime.modes.clear();
    Check(pal4::StripFrame::Rebuild(state, runtime) == 0x5au &&
              runtime.events == std::vector<std::string>{
                  "identity", "rotate", "rotate", "rotate", "scale",
                  "translate", "update"} &&
              runtime.modes == std::vector<std::uint32_t>{0u, 2u, 2u, 2u},
          "RS mode rotates before scaling with native combine modes");
}

}  // namespace

int main() {
    TestFrame();
}
