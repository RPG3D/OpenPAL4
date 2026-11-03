#include "pal4/script_select_dialog.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptSelectDialogRuntime {
    std::vector<std::string> calls;
    std::uint32_t selection = 0u;
    std::uint32_t expected_string = 0x3000u;
    std::uint32_t default_selection = 0u;

    std::uint32_t UiFrameManager() override {
        calls.emplace_back("frames");
        return 0x1000u;
    }
    std::uint32_t SelectDialog(std::uint32_t frames) override {
        Check(frames == 0x1000u, "select dialog frames");
        calls.emplace_back("dialog");
        return 0x2000u;
    }
    std::int32_t AddButton(std::uint32_t dialog,
                           std::uint32_t string_object) override {
        Check(dialog == 0x2000u && string_object == expected_string,
              "select dialog receives original string object");
        calls.emplace_back("add");
        return -17;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x209cu, "selection offset");
        calls.emplace_back("read");
        return selection;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == 0x20a0u, "default selection offset");
        default_selection = value;
        calls.emplace_back("write");
    }
};

void DialogBranches() {
    for (auto string_object : {0u, 0x3000u}) {
        Runtime add;
        add.expected_string = string_object;
        Check(pal4::ScriptSelectDialog::AddItem(string_object, add) == -17 &&
                  add.calls == std::vector<std::string>{
                      "frames", "dialog", "add"},
              "select item passes string object without fallback");
    }
    for (auto selection : {0u, 4u, 0xffffffffu}) {
        Runtime last;
        last.selection = selection;
        Check(pal4::ScriptSelectDialog::LastSelection(last) ==
                  static_cast<std::int32_t>(selection) &&
                  last.calls == std::vector<std::string>{
                      "frames", "dialog", "read"},
              "last selection forwards signed stored index");
    }
    for (auto value : {0, 4, -1}) {
        Runtime selected;
        Check(pal4::ScriptSelectDialog::SetDefaultSelection(value, selected) ==
                  0x2000u &&
                  selected.default_selection ==
                      static_cast<std::uint32_t>(value) &&
                  selected.calls == std::vector<std::string>{
                      "frames", "dialog", "write"},
              "default selection writes +0xa0 and returns dialog");
    }
}

}  // namespace

int main() {
    try {
        DialogBranches();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
