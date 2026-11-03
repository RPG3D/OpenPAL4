#include "pal4/magic_window_selection_handler.h"

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

struct RequirementsRuntime final : pal4::MagicWindowRequirementsRuntimeStub {
    explicit RequirementsRuntime(std::vector<std::string>& calls) : calls(calls) {}

    std::vector<std::string>& calls;
    std::uint32_t player{0x9000u};

    std::uint32_t GetPlayer(std::int32_t index) override {
        Check(index == 3, "selected character index");
        calls.push_back("player");
        return player;
    }
    bool ConditionContains(std::uint32_t selected, std::uint32_t key) override {
        Check(selected == player && key == 71u, "condition arguments");
        calls.push_back("condition");
        return false;
    }
    bool SortedContains(std::uint32_t selected, std::uint32_t key) override {
        Check(selected == player && key == 71u, "sorted arguments");
        calls.push_back("sorted");
        return true;
    }
    std::int32_t PlayerValue(std::uint32_t selected, std::uint32_t index) override {
        Check(selected == player && index == 8u, "stat arguments");
        calls.push_back("stat");
        return 5;
    }
};

struct DialogRuntime final : pal4::MagicWindowSelectionDialogRuntimeStub {
    explicit DialogRuntime(std::vector<std::string>& calls) : calls(calls) {}

    std::vector<std::string>& calls;
    pal4::MagicSelectionButton button;
    std::uint32_t payload{};

    void HideAndCleanup() override { calls.push_back("hide"); }
    std::string TriggerTextUtf8(std::uint32_t trigger_id) override {
        Check(trigger_id == 0x16u, "trigger text ID");
        calls.push_back("text");
        return "Use skill";
    }
    void CreateButton(const pal4::MagicSelectionButton& value) override {
        calls.push_back("button");
        button = value;
    }
    void ShowDialog(std::uint32_t value) override {
        calls.push_back("show");
        payload = value;
    }
};

pal4::MagicWindowState State() {
    pal4::MagicWindowState state;
    state.selected_character_index = 3;
    return state;
}

pal4::MagicDbStuntRecord Record() {
    pal4::MagicDbStuntRecord record;
    record.key = 71u;
    record.kind = 2u;
    record.required_stat = 5;
    return record;
}

void TestEmptyRecord() {
    std::vector<std::string> calls;
    RequirementsRuntime requirements(calls);
    DialogRuntime dialog(calls);
    Check(pal4::MagicWindowSelectionHandler::Handle(
              0x1234u, nullptr, State(), requirements, dialog),
          "empty record returns success");
    Check(calls.empty(), "empty record has no side effects");
}

void TestEnabledButton() {
    std::vector<std::string> calls;
    RequirementsRuntime requirements(calls);
    DialogRuntime dialog(calls);
    const auto record = Record();
    Check(pal4::MagicWindowSelectionHandler::Handle(
              0x1234u, &record, State(), requirements, dialog),
          "selection handler returns success");
    Check(calls == std::vector<std::string>{
                       "hide", "player", "condition", "sorted", "stat",
                       "text", "button", "show"},
          "dialog and requirements call order");
    Check(dialog.button.record_key == record.key &&
              dialog.button.unknown_04 == 0u &&
              dialog.button.command_address ==
                  pal4::MagicWindowSelectionHandler::kButtonCommandAddress &&
              dialog.button.text == "Use skill" && dialog.button.enabled,
          "enabled button fields");
    Check(dialog.payload == 0x1234u, "dialog receives event payload");
}

void TestDisabledButton() {
    std::vector<std::string> calls;
    RequirementsRuntime requirements(calls);
    requirements.player = 0;
    DialogRuntime dialog(calls);
    const auto record = Record();
    Check(pal4::MagicWindowSelectionHandler::Handle(
              0x4321u, &record, State(), requirements, dialog),
          "disabled selection returns success");
    Check(calls == std::vector<std::string>{"hide", "player", "text", "button", "show"},
          "failed requirement still creates and shows button");
    Check(!dialog.button.enabled && dialog.payload == 0x4321u,
          "disabled button preserves event payload");
}

}  // namespace

int main() {
    TestEmptyRecord();
    TestEnabledButton();
    TestDisabledButton();
}
