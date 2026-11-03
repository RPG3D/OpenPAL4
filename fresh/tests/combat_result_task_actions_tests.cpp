#include "pal4/combat_result_task_actions.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatResultTaskRuntime {
    std::vector<std::string> calls;
    std::uint32_t state = 0u;
    std::uint32_t finished = 0u;
    bool visible = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        if (address == 0x403cu) return 0x5000u;
        if (address == 0x4040u) return 2u;
        if (address == 0x5058u) return 0x6000u;
        throw std::runtime_error("unexpected word address");
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == 0x52ccu, "dialog state field +716");
        calls.emplace_back("write");
        state = value;
    }
    void PlaySound(std::string_view name,
                   std::uint32_t mode) override {
        Check(name == "WA46.wav" && mode == 1u,
              "panel reveal sound and mode");
        calls.emplace_back("sound");
    }
    void SetVisible(std::uint32_t window,
                    bool show) override {
        Check(window == 0x6000u, "panel window at dialog +88");
        calls.emplace_back("visible");
        visible = show;
    }
    void EndCombatSequence(std::uint32_t combat_result) override {
        calls.emplace_back("finish");
        finished = combat_result;
    }
};

void StatePanelAndEnd() {
    Runtime runtime;
    Check(pal4::CombatResultTaskActions::SetState(
              0x4000u, runtime) == 1u &&
              runtime.state == 2u &&
              runtime.calls ==
                  std::vector<std::string>{"read", "read", "write"},
          "state value is written to dialog in read order");
    runtime.calls.clear();
    Check(pal4::CombatResultTaskActions::ShowPanel(
              0x4000u, runtime) == 1u && runtime.visible &&
              runtime.calls == std::vector<std::string>{
                                   "sound", "read", "read", "visible"},
          "sound precedes panel reveal");
    runtime.calls.clear();
    Check(pal4::CombatResultTaskActions::FinishCombat(
              0x4000u, runtime) == 1u &&
              runtime.finished == 0x5000u &&
              runtime.calls ==
                  std::vector<std::string>{"read", "finish"},
          "combat system receives result pointer");
}

}  // namespace

int main() {
    try {
        StatePanelAndEnd();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
