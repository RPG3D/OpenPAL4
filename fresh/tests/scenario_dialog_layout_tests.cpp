#include "pal4/scenario_dialog.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

bool Equals(const pal4::ScenarioRect& actual,
            const pal4::ScenarioRect& expected) {
    return actual.left == expected.left &&
           actual.top == expected.top &&
           actual.right == expected.right &&
           actual.bottom == expected.bottom;
}

struct Runtime final : pal4::IScenarioLayoutRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, pal4::ScenarioRect>> rects;
    std::vector<std::pair<std::uint32_t, std::uint8_t>> flags;
    bool portrait_visible = false;
    float font_extent = 18.0f;

    explicit Runtime(bool has_image) {
        words[0x1038u] = 0x2000u;
        words[0x102cu] = 0x3000u;
        words[0x1034u] = 0x4000u;
        words[0x1030u] = 0x5000u;
        words[0x36b0u] = has_image ? 0x6000u : 0u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    float ReadFloat(std::uint32_t address) override {
        Check(address == 0x6020u, "image height address");
        calls.emplace_back("height");
        return 75.0f;
    }
    float FontExtentAA(std::uint32_t editbox) override {
        Check(editbox == 0x2000u, "font source editbox");
        calls.emplace_back("font-AA");
        return font_extent;
    }
    void SetVisible(std::uint32_t window,
                    bool visible) override {
        Check(window == 0x3000u, "portrait visibility target");
        calls.emplace_back("visible");
        portrait_visible = visible;
    }
    std::uint32_t SetRect(
        std::uint32_t window,
        const pal4::ScenarioRect& rectangle) override {
        calls.emplace_back("rect");
        rects.emplace_back(window, rectangle);
        return 0x9000u + static_cast<std::uint32_t>(rects.size());
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.emplace_back("flag");
        flags.emplace_back(address, value);
    }
    void RequestRedraw(std::uint32_t window) override {
        Check(window == 0x4000u, "scenario redraw target");
        calls.emplace_back("redraw");
    }
};

void CheckPortrait(std::uint8_t side, float extent) {
    Runtime runtime(true);
    runtime.font_extent = extent;
    const auto result = pal4::ScenarioDialog::Layout(
        0x1000u, side, runtime);
    Check(result == 0x9003u && runtime.portrait_visible &&
              runtime.rects.size() == 3u &&
              runtime.rects[0].first == 0x2000u &&
              runtime.rects[1].first == 0x3000u &&
              runtime.rects[2].first == 0x5000u,
          "portrait branch targets and final return");
    const bool left = side != 0u;
    Check(Equals(runtime.rects[0].second,
                 left ? pal4::ScenarioRect{206, 12, 717 + extent, 115}
                      : pal4::ScenarioRect{78, 12, 587 + extent, 115}) &&
              Equals(runtime.rects[1].second,
                 left ? pal4::ScenarioRect{0, 525, 199, 600}
                      : pal4::ScenarioRect{601, 525, 800, 600}) &&
              Equals(runtime.rects[2].second,
                 left ? pal4::ScenarioRect{746, 95, 772, 118}
                      : pal4::ScenarioRect{575, 95, 601, 118}),
          "left and right portrait geometry");
    Check(runtime.flags ==
              (std::vector<std::pair<std::uint32_t, std::uint8_t>>{
                  {0x46b4u, static_cast<std::uint8_t>(!left)},
                  {0x36b4u, static_cast<std::uint8_t>(!left)}}) &&
              runtime.calls ==
              (std::vector<std::string>{
                  "read", "font-AA", "read", "read", "read",
                  "read", "visible", "rect", "flag", "redraw",
                  "height", "rect", "flag", "rect"}),
          "portrait flags and operation order");
}

void CheckNoPortrait(float extent) {
    Runtime runtime(false);
    runtime.font_extent = extent;
    Check(pal4::ScenarioDialog::Layout(
              0x1000u, 1u, runtime) ==
              0x9002u && !runtime.portrait_visible &&
              runtime.rects.size() == 2u &&
              runtime.rects[0].first == 0x2000u &&
              runtime.rects[1].first == 0x5000u &&
              Equals(runtime.rects[0].second,
                     {30, 12, 717.0f + extent, 115}) &&
              Equals(runtime.rects[1].second,
                     {746, 95, 772, 118}) &&
              runtime.flags ==
                  (std::vector<std::pair<std::uint32_t,
                                         std::uint8_t>>{{0x46b4u, 0u}}),
          "no-image layout uses font extent, independent of side");
}

}  // namespace

int main() {
    try {
        for (const float extent : {0.0f, 18.0f, 30.0f}) {
            CheckPortrait(0u, extent);
            CheckPortrait(1u, extent);
            CheckPortrait(255u, extent);
        }
        CheckNoPortrait(18.0f);
        CheckNoPortrait(0.0f);
        std::cout << "scenario_dialog_layout_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
