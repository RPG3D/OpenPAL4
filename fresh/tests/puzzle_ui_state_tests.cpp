#include "pal4/puzzle_ui_state.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IPuzzleUiRuntime {
    std::map<std::string, std::optional<std::uint32_t>> attributes;
    std::vector<std::string> calls;
    std::uint32_t descriptor = 0u;
    std::uint32_t event_type = 0u;

    std::uint32_t UIFrameManager() override {
        calls.emplace_back("ui"); return 0x4000u;
    }
    void SetUIFlag(std::uint32_t manager, std::uint32_t value) override {
        Check(manager == 0x4000u && value == 1u, "UI flag arguments");
        calls.emplace_back("flag");
    }
    std::uint32_t Console() override {
        calls.emplace_back("console"); return 0x5000u;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        Check(address == 0x5000u && value == 0u, "console reset");
        calls.emplace_back("clear-console");
    }
    std::optional<std::uint32_t> ReadIntegerAttribute(
        std::uint32_t config, std::string_view key) override {
        Check(config == 0x6000u, "config pointer forwarded");
        calls.push_back("read:" + std::string(key));
        return attributes[std::string(key)];
    }
    void DispatchEvent(std::uint32_t manager,
                       std::uint32_t selected,
                       std::uint32_t first,
                       std::uint32_t second) override {
        Check(manager == 0x4000u && first == 1u, "event payload first word");
        descriptor = selected;
        event_type = second;
        calls.emplace_back("dispatch");
    }
    std::uint32_t UpdateUI(std::uint32_t manager, double delta) override {
        Check(manager == 0x4000u && delta == 0.25,
              "UI update manager and widened float");
        calls.emplace_back("update"); return 0x77u;
    }
};

void TestEntry() {
    Runtime runtime;
    const std::vector<std::string> order = {
        "ui", "flag", "console", "clear-console", "read:Mode",
        "read:Type", "ui", "dispatch"};
    Check(pal4::PuzzleUiState::Enter(99u, 0x6000u, runtime) == 1u &&
          runtime.descriptor == pal4::PuzzleUiState::kShowPuzzleEvent &&
          runtime.event_type == 0u && runtime.calls == order,
          "default mode and type select puzzle event in order");
    runtime.calls.clear();
    runtime.attributes["Mode"] = 1u;
    runtime.attributes["Type"] = 42u;
    Check(pal4::PuzzleUiState::Enter(0u, 0x6000u, runtime) == 1u &&
          runtime.descriptor == pal4::PuzzleUiState::kShowJigsawEvent &&
          runtime.event_type == 42u && runtime.calls == order,
          "nonzero mode selects jigsaw with configured type");
    runtime.calls.clear();
    runtime.attributes["Mode"] = 0xffffffffu;
    Check(pal4::PuzzleUiState::Enter(0u, 0x6000u, runtime) == 1u &&
          runtime.descriptor == pal4::PuzzleUiState::kShowJigsawEvent,
          "negative mode remains nonzero");
}

void TestUpdate() {
    Runtime runtime;
    Check(pal4::PuzzleUiState::Update(0.25f, 123u, 456u, runtime) == 0x77u &&
          runtime.calls == std::vector<std::string>{"ui", "update"},
          "update wrapper forwards delta and return");
}

}  // namespace

int main() {
    try {
        TestEntry();
        TestUpdate();
        std::cout << "puzzle UI state tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
