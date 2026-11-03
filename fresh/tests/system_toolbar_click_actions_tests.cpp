#include "pal4/system_toolbar_click_actions.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ISystemToolbarClickRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x8b07d8u, 0x7000u}, {0x9000u, 1u}};
    std::vector<std::string> calls;
    std::uint32_t frame = 0x5000u;
    std::uint32_t mouse_window = 0u;
    std::uint32_t manager_calls = 0u;

    Runtime() {
        for (std::uint32_t offset = 64u; offset <= 88u; offset += 4u) {
            words[frame + offset] = 0x8000u + offset;
        }
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words.at(address);
    }
    std::uint32_t FrameManager() override {
        ++manager_calls;
        calls.emplace_back("manager");
        return 0x4000u;
    }
    std::uint32_t FindFrame(const std::uint32_t manager,
                            const std::uint32_t name) override {
        Check(manager == 0x4000u && name == 0x7000u,
              "toolbar lookup arguments");
        calls.emplace_back("find");
        return frame;
    }
    void ClearAllSelection(const std::uint32_t target) override {
        Check(target == frame, "clear-selection frame");
        calls.emplace_back("clear");
    }
    void Close(const std::uint32_t target) override {
        Check(target == frame, "close frame");
        calls.emplace_back("close");
    }
    void DispatchEvent(const std::uint32_t tree,
                       const std::uint32_t key,
                       const std::uint32_t first,
                       const std::uint32_t second) override {
        Check(tree == 0x4104u && key == 0x8a80acu &&
                  first == 1u && second == 0u,
              "toolbar event key and payload");
        calls.emplace_back("event");
    }
    void HandleMouseEvent(const std::uint32_t window) override {
        mouse_window = window;
        calls.emplace_back("mouse");
    }
};

void TestClick1() {
    Runtime runtime;
    Check(pal4::SystemToolbarClickActions::HandleClick1(runtime) == 1u &&
              runtime.manager_calls == 2u &&
              runtime.calls == std::vector<std::string>{
                  "manager", "read:9111512", "find", "clear",
                  "manager", "event"},
          "button 1 clears selection then dispatches main event");
    runtime = Runtime{};
    runtime.frame = 0u;
    Check(pal4::SystemToolbarClickActions::HandleClick1(runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "manager", "read:9111512", "find", "manager", "event"},
          "button 1 still dispatches without a toolbar frame");
}

void TestClick2() {
    Runtime runtime;
    Check(pal4::SystemToolbarClickActions::HandleClick2(runtime) &&
              runtime.manager_calls == 1u &&
              runtime.calls == std::vector<std::string>{
                  "manager", "read:9111512", "find", "close"},
          "button 2 closes toolbar and returns true through shared tail");
    runtime = Runtime{};
    runtime.frame = 0u;
    Check(pal4::SystemToolbarClickActions::HandleClick2(runtime) &&
              runtime.calls.back() == "find",
          "button 2 returns true when frame is absent");
}

void TestClick3() {
    constexpr std::uint32_t expected_offsets[]{
        0u, 72u, 64u, 76u, 68u, 88u, 80u, 84u};
    for (std::uint32_t choice = 1u; choice <= 7u; ++choice) {
        Runtime runtime;
        runtime.words[0x9000u] = choice;
        Check(pal4::SystemToolbarClickActions::HandleClick3(
                  0x9000u, runtime) == 1u &&
                  runtime.mouse_window == 0x8000u + expected_offsets[choice] &&
                  runtime.calls.back() == "mouse" &&
                  runtime.manager_calls == 1u,
              "button 3 dispatches each choice to exact window");
    }
    for (const auto choice : {0u, 8u, 0xffffffffu}) {
        Runtime runtime;
        runtime.words[0x9000u] = choice;
        Check(pal4::SystemToolbarClickActions::HandleClick3(
                  0x9000u, runtime) == 1u &&
                  runtime.mouse_window == 0u &&
                  runtime.calls.back() == "read:36864",
              "unsupported choice has no mouse action");
    }
    Runtime missing;
    missing.frame = 0u;
    Check(pal4::SystemToolbarClickActions::HandleClick3(
              0x9000u, missing) == 1u &&
              missing.calls.back() == "find",
          "missing frame does not read choice");
}

}  // namespace

int main() {
    TestClick1();
    TestClick2();
    TestClick3();
    return EXIT_SUCCESS;
}
