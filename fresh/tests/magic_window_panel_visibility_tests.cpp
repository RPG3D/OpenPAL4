#include "pal4/magic_window_panel_visibility.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::MagicWindowPanelVisibilityRuntimeStub {
    std::vector<std::pair<std::uint32_t, bool>> visible;
    std::vector<std::string> calls;
    std::uint32_t detail_owner{};
    std::int32_t detail_index{};
    void SetVisible(std::uint32_t window, bool value) override {
        calls.push_back("visible:" + std::to_string(window));
        visible.emplace_back(window, value);
    }
    void SetImage(std::uint32_t window, std::string_view imageset,
                  std::string_view image) override {
        Check(window == 0x1154u && imageset == "zi0" &&
                  image == "xianshuteji", "stunt image target and resource");
        calls.push_back("image");
    }
    void UpdateMagicDetails(std::uint32_t object, std::int32_t index) override {
        Check(visible.size() == 14u, "detail call follows all visibility writes");
        detail_owner = object;
        detail_index = index;
    }
};

void TestVisibility(bool enabled) {
    pal4::MagicWindowPanelState panel;
    for (std::uint32_t offset = 0; offset < 0x170u; offset += 4u)
        panel.At(offset) = 0x1000u + offset;
    Runtime runtime;
    pal4::MagicWindowPanelVisibility::Set(0x3000u, enabled, panel, runtime);
    Check(runtime.visible.size() == 14u &&
              runtime.visible[0] == std::pair<std::uint32_t, bool>{0x115cu, enabled} &&
              runtime.visible[1] == std::pair<std::uint32_t, bool>{0x1164u, !enabled},
          "main controls have opposite visibility");
    for (std::uint32_t row = 0; row < 6u; ++row) {
        Check(runtime.visible[2u + 2u * row] ==
                  std::pair<std::uint32_t, bool>{0x10c0u + 4u * row, enabled} &&
                  runtime.visible[3u + 2u * row] ==
                  std::pair<std::uint32_t, bool>{0x10f0u + 4u * row, enabled},
              "paired row visibility and order");
    }
    Check(runtime.detail_owner == 0x3000u && runtime.detail_index == -1,
          "detail update sentinel");
}

void TestStuntVisibility(bool enabled) {
    pal4::MagicWindowPanelState panel;
    for (std::uint32_t offset = 0; offset < 0x170u; offset += 4u)
        panel.At(offset) = 0x1000u + offset;
    Runtime runtime;
    pal4::MagicWindowPanelVisibility::SetStunt(enabled, panel, runtime);
    Check(runtime.calls == std::vector<std::string>{
                               "image", "visible:4448", "visible:4440",
                               "visible:4456", "visible:4460"},
          "stunt image precedes four visibility writes in IDA order");
    Check(runtime.visible == std::vector<std::pair<std::uint32_t, bool>>{
                                 {0x1160u, enabled}, {0x1158u, enabled},
                                 {0x1168u, enabled}, {0x116cu, enabled}},
          "stunt controls receive requested visibility");
}

}  // namespace

int main() {
    TestVisibility(false);
    TestVisibility(true);
    TestStuntVisibility(false);
    TestStuntVisibility(true);
}
