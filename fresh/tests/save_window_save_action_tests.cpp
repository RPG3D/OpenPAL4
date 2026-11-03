#include "pal4/save_window_save_action.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::SaveWindowSaveActionRuntimeStub {
    std::vector<std::string> calls;
    std::uint32_t hover{0x7000u};
    std::int32_t slot{27};
    std::int32_t global_slot{-1};
    float global_scroll{-1.0f};
    bool fail_save{};

    std::uint32_t HoverItem(std::uint32_t list) override {
        Check(list == 0x5000u, "hover item queried from List");
        calls.emplace_back("hover");
        return hover;
    }
    std::int32_t ReadSlot(std::uint32_t item) override {
        Check(item == 0x7000u, "slot read from hovered item");
        calls.emplace_back("slot");
        return slot;
    }
    float ScrollPosition(std::uint32_t list) override {
        Check(list == 0x5000u, "scroll read from List scrollbar");
        calls.emplace_back("scroll");
        return 0.625f;
    }
    void SetGlobalSaveIndex(std::int32_t value) override {
        global_slot = value;
        calls.emplace_back("global-slot");
    }
    void SetGlobalScrollPosition(float value) override {
        global_scroll = value;
        calls.emplace_back("global-scroll");
    }
    void SaveSettings() override {
        calls.emplace_back("settings");
    }
    void SaveGameSlot(std::int32_t value) override {
        Check(value == slot, "save targets hovered slot");
        calls.emplace_back("save");
        if (fail_save) throw std::runtime_error("save");
    }
    void PopulateSaveList(pal4::SaveWindowState&,
                          std::int32_t page,
                          std::uint8_t reset) override {
        Check(page == slot / 10 && reset == 1u,
              "refreshes saved slot page with reset");
        calls.emplace_back("populate");
    }
};

}  // namespace

int main() {
    pal4::SaveWindowState state{};
    state.Write(0x54u, 0x5000u);
    Runtime runtime;
    Check(pal4::SaveWindowSaveAction::Confirm(state, 0, runtime) == 1u &&
              runtime.calls.empty(), "cancel does not save");
    runtime.hover = 0u;
    Check(pal4::SaveWindowSaveAction::Confirm(state, 1, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{"hover"},
          "confirm without hover item does not save");
    runtime = Runtime{};
    Check(pal4::SaveWindowSaveAction::Confirm(state, 1, runtime) == 1u &&
              runtime.global_slot == 27 &&
              runtime.global_scroll == 0.625f &&
              runtime.calls == std::vector<std::string>{
                  "hover", "slot", "global-slot", "scroll", "global-scroll",
                  "settings", "save", "populate"},
          "confirm stores globals before saving and refreshing");
    runtime = Runtime{};
    runtime.slot = -12;
    Check(pal4::SaveWindowSaveAction::Confirm(state, 1, runtime) == 1u &&
              runtime.global_slot == -12 &&
              runtime.calls.back() == "populate",
          "negative slot keeps signed page division");
    runtime = Runtime{};
    runtime.fail_save = true;
    try {
        (void)pal4::SaveWindowSaveAction::Confirm(state, 1, runtime);
        Check(false, "save failure propagates");
    } catch (const std::runtime_error&) {
        Check(runtime.calls.back() == "save",
              "failure skips page refresh");
    }
    std::cout << "Save window save action tests passed\n";
}
