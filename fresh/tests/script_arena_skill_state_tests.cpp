#include "pal4/script_arena_skill_state.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptArenaSkillStateRuntime {
    std::vector<std::string> calls;

    std::uint32_t ArenaSkillManager() override {
        calls.emplace_back("manager");
        return 0x1000u;
    }
    std::uint32_t CleanupPlayerData(std::uint32_t manager) override {
        Check(manager == 0x1000u, "cleanup arena skill manager");
        calls.emplace_back("cleanup");
        return 0x81u;
    }
    std::uint32_t UpdatePlayerData(std::uint32_t manager) override {
        Check(manager == 0x1000u, "update arena skill manager");
        calls.emplace_back("update");
        return 0x82u;
    }
};

void StateCallbacks() {
    Runtime hidden;
    Check(pal4::ScriptArenaSkillState::HideSkillObject(hidden) == 0x81u &&
              hidden.calls == std::vector<std::string>{"manager", "cleanup"},
          "hide skill object forwards cleanup result");
    Runtime shown;
    Check(pal4::ScriptArenaSkillState::ShowSignpost(shown) == 0x82u &&
              shown.calls == std::vector<std::string>{"manager", "update"},
          "show signpost forwards update result");
}

}  // namespace

int main() {
    try {
        StateCallbacks();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
