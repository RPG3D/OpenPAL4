#include "pal4/script_scenario_portrait.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptScenarioPortraitRuntime {
    std::uint8_t visible = 0u;
    std::uint32_t name = 0x5000u;
    std::vector<std::string> calls;

    std::uint32_t UiFrameManager() override {
        calls.emplace_back("frames"); return 0x1000u;
    }
    std::uint32_t ScenarioDialog(std::uint32_t frames) override {
        Check(frames == 0x1000u, "scenario owner");
        calls.emplace_back("dialog"); return 0x2000u;
    }
    std::uint8_t IsVisible(std::uint32_t dialog) override {
        Check(dialog == 0x2000u, "scenario visibility target");
        calls.emplace_back("visible"); return visible;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x3004u, "portrait string payload offset");
        calls.emplace_back("name"); return name;
    }
    std::uint32_t EmptyStringAddress() override {
        calls.emplace_back("empty"); return 0x4000u;
    }
    std::uint8_t SetPortrait(std::uint32_t dialog,
                             std::uint32_t resolved_name,
                             std::int32_t argument) override {
        Check(dialog == 0x2000u && resolved_name ==
                  (name == 0u ? 0x4000u : name) && argument == -7,
              "portrait receives full integer flag and resolved name");
        calls.emplace_back("set"); return 0x9au;
    }
};

void PortraitBranches() {
    Runtime shown;
    shown.visible = 2u;
    Check(pal4::ScriptScenarioPortrait::Set(0x3000u, -7, shown) == 2u &&
              shown.calls == std::vector<std::string>{
                  "frames", "dialog", "visible"},
          "visible dialog skips string and portrait setter");
    for (const bool empty : {false, true}) {
        Runtime hidden;
        if (empty) hidden.name = 0u;
        const auto result = pal4::ScriptScenarioPortrait::Set(
            0x3000u, -7, hidden);
        auto expected = std::vector<std::string>{
            "frames", "dialog", "visible", "name"};
        if (empty) expected.emplace_back("empty");
        expected.emplace_back("set");
        Check(result == 0x9au && hidden.calls == expected,
              "hidden dialog sets portrait with name fallback");
    }
}

}  // namespace

int main() {
    try {
        PortraitBranches();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
