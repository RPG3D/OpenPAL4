#include "pal4/ui_frame_manager_singleton.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IUiFrameManagerEdgeWindowsRuntime {
    std::uint32_t width = 1280u;
    std::uint32_t height = 800u;
    std::map<std::uint32_t, std::uint32_t> words{
        {0x5198u, 0x6000u}, {0x51d4u, 0x7000u},
        {0x51d8u, 0x8000u}, {0x9030u, 0xa000u},
        {0x7044u, 0xb000u}, {0x8044u, 0xc000u}};
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> additions;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> removals;
    std::vector<std::uint32_t> positioned;

    std::uint32_t ReadConfigWord(std::uint32_t offset) override {
        calls.emplace_back("config " + std::to_string(offset));
        return offset == 0u ? width : height;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read " + std::to_string(address));
        return words[address];
    }
    std::array<float, 2> TransformMouseCoordinates(
        std::uint32_t system, std::array<float, 2> input) override {
        Check(system == 0x6000u && input[1] == 0.0f,
              "coordinate transform receiver/input");
        if (std::bit_cast<std::uint32_t>(input[0]) == 0u) {
            calls.emplace_back("transform 0");
            return {10.0f, 20.0f};
        }
        Check(std::bit_cast<std::uint32_t>(input[0]) == 0x4492aaaau,
              "second edge uses exact float bits");
        calls.emplace_back("transform 1");
        return {30.0f, 40.0f};
    }
    std::uint32_t SystemSingleton() override {
        calls.emplace_back("singleton");
        return 0x9000u;
    }
    void AddChildWindow(std::uint32_t parent,
                        std::uint32_t child) override {
        Check(parent == 0xa000u, "CEGUI root");
        calls.emplace_back("add " + std::to_string(child));
        additions.emplace_back(parent, child);
    }
    void RemoveChildWindow(std::uint32_t parent,
                           std::uint32_t child) override {
        calls.emplace_back("remove " + std::to_string(child));
        removals.emplace_back(parent, child);
    }
    void SetAlwaysOnTop(std::uint32_t window, bool value) override {
        Check(value, "overlay always on top");
        calls.emplace_back("top " + std::to_string(window));
    }
    void SetClippedByParent(std::uint32_t window, bool value) override {
        Check(!value, "overlay unclipped");
        calls.emplace_back("clip " + std::to_string(window));
    }
    void SetPosition(std::uint32_t window, std::uint32_t mode,
                     std::array<float, 2> position) override {
        Check(mode == 1u, "position metric mode");
        Check(position == (window == 0x7000u
                               ? std::array<float, 2>{10.0f, 20.0f}
                               : std::array<float, 2>{30.0f, 40.0f}),
              "transformed position forwarded");
        calls.emplace_back("position " + std::to_string(window));
        positioned.emplace_back(window);
    }
    void SetWidth(std::uint32_t window, std::uint32_t mode,
                  std::uint32_t bits) override {
        Check(mode == 1u && bits == 0x42a00002u,
              "exact width metric and bits");
        calls.emplace_back("width " + std::to_string(window));
    }
    void SetHeight(std::uint32_t window, std::uint32_t mode,
                   std::uint32_t bits) override {
        Check(mode == 1u && bits == 0x44160000u,
              "exact height metric and bits");
        calls.emplace_back("height " + std::to_string(window));
    }
};

void CheckResolutionGate() {
    for (const bool wrong_width : {true, false}) {
        Runtime runtime;
        if (wrong_width) runtime.width = 1024u;
        else runtime.height = 768u;
        pal4::UiFrameManagerSingleton::ToggleEdgeWindows(
            0x5000u, true, runtime);
        Check(runtime.calls ==
                  (wrong_width ? std::vector<std::string>{"config 0"}
                               : std::vector<std::string>{"config 0", "config 4"}),
              "unsupported resolution exits before manager reads");
    }
}

void CheckEnable() {
    Runtime runtime;
    pal4::UiFrameManagerSingleton::ToggleEdgeWindows(0x5000u, true, runtime);
    Check(runtime.additions ==
              (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {0xa000u, 0x7000u}, {0xa000u, 0x8000u}}),
          "both overlays attached in order");
    Check(runtime.positioned ==
              (std::vector<std::uint32_t>{0x7000u, 0x8000u}),
          "both overlays positioned");
    Check(runtime.calls[2] == "read 20948" &&
              runtime.calls[3] == "read 20888" &&
              runtime.calls[4] == "transform 0",
          "first overlay and transform order");

    Runtime missing_first;
    missing_first.words[0x51d4u] = 0u;
    pal4::UiFrameManagerSingleton::ToggleEdgeWindows(
        0x5000u, true, missing_first);
    Check(missing_first.additions.size() == 1u &&
              missing_first.additions.front().second == 0x8000u,
          "missing first overlay does not skip second");
    Runtime missing_both;
    missing_both.words[0x51d4u] = 0u;
    missing_both.words[0x51d8u] = 0u;
    pal4::UiFrameManagerSingleton::ToggleEdgeWindows(
        0x5000u, true, missing_both);
    Check(missing_both.additions.empty(), "missing overlays do nothing");
}

void CheckDisable() {
    Runtime runtime;
    pal4::UiFrameManagerSingleton::ToggleEdgeWindows(0x5000u, false, runtime);
    Check(runtime.removals ==
              (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {0xb000u, 0x7000u}, {0xc000u, 0x8000u}}),
          "detach each overlay from its current parent");
    Check(runtime.calls == (std::vector<std::string>{
              "config 0", "config 4", "read 20948", "read 28740",
              "remove 28672", "read 20952", "read 32836",
              "remove 32768"}),
          "disable follows exact parent read and removal order");

    Runtime unparented;
    unparented.words[0x7044u] = 0u;
    unparented.words[0x8044u] = 0u;
    pal4::UiFrameManagerSingleton::ToggleEdgeWindows(
        0x5000u, false, unparented);
    Check(unparented.removals.empty(), "unparented overlays are retained");
}

}  // namespace

int main() {
    try {
        CheckResolutionGate();
        CheckEnable();
        CheckDisable();
        std::cout << "ui_frame_manager_edge_windows_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
