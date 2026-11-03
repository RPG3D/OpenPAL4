#include "pal4/magic_window_image_setup.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(EXIT_FAILURE); }
}

struct ImageCall {
    std::uint32_t window{};
    std::string set;
    std::string image;
};

struct Runtime final : pal4::MagicWindowImageSetupRuntimeStub {
    std::vector<std::string> calls;
    std::vector<ImageCall> images;
    std::vector<std::uint32_t> visible_windows;
    std::uint32_t player{0x9000u};
    std::int32_t queried_index{};

    void SetText(std::uint32_t window, std::string_view text) override {
        Check(text.empty(), "info text cleared");
        calls.push_back("text:" + std::to_string(window));
    }
    void SetImage(std::uint32_t window, std::string_view set,
                  std::string_view image) override {
        images.push_back({window, std::string(set), std::string(image)});
        calls.push_back("image:" + std::string(image));
    }
    void SetVisible(std::uint32_t window, bool visible) override {
        Check(visible, "six labels shown");
        visible_windows.push_back(window);
        calls.push_back("visible:" + std::to_string(window));
    }
    void SetStuntPanelVisible(std::uint32_t object, bool visible) override {
        Check(object == 0x3000u && visible, "stunt panel shown");
        calls.push_back("stunt-panel");
    }
    std::uint32_t GetPlayer(std::int32_t index) override {
        queried_index = index;
        calls.push_back("player");
        return player;
    }
    void SetMagicPanelVisible(std::uint32_t object, bool visible) override {
        Check(object == 0x3000u && visible, "magic panel shown");
        calls.push_back("magic-panel");
    }
    void ResetStuntPanel(std::uint32_t object) override {
        Check(object == 0x3000u, "stunt reset target");
        calls.push_back("reset-stunt");
    }
};

pal4::MagicWindowPanelState MakePanel() {
    pal4::MagicWindowPanelState panel;
    constexpr std::array<std::uint32_t, 16> offsets{
        0x88u, 0x8cu, 0x140u, 0x144u, 0x148u, 0x14cu, 0x150u,
        0x94u, 0x90u, 0x98u, 0xa0u, 0x9cu, 0xa4u, 0x154u,
        0x7cu, 0x80u};
    for (const auto offset : offsets) panel.At(offset) = 0x1000u + offset;
    return panel;
}

void TestImageBranches() {
    constexpr std::array<std::string_view, 5> small{
        "5water_s", "5fire_s", "5thurder_s", "5wind_s", "5earth_s"};
    constexpr std::array<std::string_view, 5> large{
        "5water_l", "5fire_l", "5thurder_l", "5wind_l", "5earth_l"};
    const auto panel = MakePanel();
    for (std::uint32_t position = 0; position < 6u; ++position) {
        pal4::MagicWindowState state;
        state.category_position = position;
        state.selected_character_index = 3;
        state.slot_order = {5u, 0u, 1u, 2u, 3u, 4u};
        Runtime runtime;
        Check(pal4::MagicWindowImageSetup::Prepare(0x3000u, state, panel, runtime) == 0x9000u &&
                  runtime.queried_index == 3, "player lookup and return");
        Check(runtime.calls[0] == "text:4232" &&
                  runtime.calls[1] == "text:4236", "two info texts first");
        for (std::uint32_t i = 0; i < 5u; ++i) {
            Check(runtime.images[i].window == panel.At(0x140u + 4u * i) &&
                      runtime.images[i].set == "maimai0" &&
                      runtime.images[i].image == small[i], "five small element images");
        }
        const auto element = state.slot_order[position];
        Check(runtime.images.size() == (element == 5u ? 6u : 7u),
              "large highlight only for five magic elements");
        if (element < 5u)
            Check(runtime.images[5].window == panel.At(0x140u + 4u * element) &&
                      runtime.images[5].image == large[element],
                  "selected element receives large image");
        Check(runtime.images.back().window == panel.At(0x154u) &&
                  runtime.images.back().set == "zi0" &&
                  runtime.images.back().image == "xianshu", "stunt label image");
        constexpr std::array<std::uint32_t, 6> label_offsets{
            0x94u, 0x90u, 0x98u, 0xa0u, 0x9cu, 0xa4u};
        Check(runtime.visible_windows.size() == 6u, "six labels shown");
        for (std::uint32_t i = 0; i < 6u; ++i)
            Check(runtime.visible_windows[i] == panel.At(label_offsets[i]),
                  "label visibility order");
        Check(runtime.calls[runtime.calls.size() - 3] == "player" &&
                  runtime.calls[runtime.calls.size() - 2] == "magic-panel" &&
                  runtime.calls.back() == "reset-stunt", "player branch sequence");
    }
}

void TestMissingPlayer() {
    auto panel = MakePanel();
    pal4::MagicWindowState state;
    state.slot_order = {5u, 0u, 1u, 2u, 3u, 4u};
    Runtime runtime;
    runtime.player = 0;
    Check(pal4::MagicWindowImageSetup::Prepare(0x3000u, state, panel, runtime) == 0u &&
              runtime.calls.back() == "player" &&
              runtime.images.size() == 6u,
          "missing player exits after common image setup");
}

}  // namespace

int main() { TestImageBranches(); TestMissingPlayer(); }
