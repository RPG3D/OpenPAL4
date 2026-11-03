#include "pal4/script_forbidden_skill.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptForbiddenSkillRuntime {
    bool found = true;
    std::uint32_t expected_enabled = 0u;
    std::vector<std::string> calls;

    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager");
        return 0x1000u;
    }
    std::uint32_t FindPlayer(std::uint32_t manager,
                             std::int32_t index) override {
        Check(manager == 0x1000u && index == -2,
              "signed player index and manager");
        calls.emplace_back("player");
        return found ? 0x2000u : 0u;
    }
    std::uint8_t SetPresent(std::uint32_t collection,
                            std::int32_t skill,
                            std::uint32_t enabled) override {
        Check(collection == 0x2938u && skill == -17 &&
                  enabled == expected_enabled,
              "forbidden skill collection and arguments");
        calls.emplace_back("set");
        return 0xa5u;
    }
};

void Branches() {
    for (auto enabled : {0u, 1u, 0xffffffffu}) {
        Runtime runtime;
        runtime.expected_enabled = enabled;
        Check(pal4::ScriptForbiddenSkill::Set(-2, -17, enabled, runtime) ==
                  0xa5u &&
                  runtime.calls == std::vector<std::string>{
                      "manager", "player", "set"},
              "found player forwards original third argument and result");
    }
    Runtime missing;
    missing.found = false;
    Check(pal4::ScriptForbiddenSkill::Set(-2, -17, 1u, missing) == 0u &&
              missing.calls == std::vector<std::string>{
                  "manager", "player"},
          "missing player skips skill collection");
}

}  // namespace

int main() {
    try {
        Branches();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
