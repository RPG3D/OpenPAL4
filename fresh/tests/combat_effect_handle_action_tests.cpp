#include "pal4/combat_effect_handle_action.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatEffectHandleActionRuntime {
    std::vector<std::string> calls;
    pal4::CombatEffectHandle before_reset{};
    pal4::CombatEffectHandle after_reset{};

    void BuildHandle(std::uint32_t target, pal4::CombatEffectHandle& handle,
                     std::uint32_t first_source,
                     std::uint32_t second_source) override {
        calls.push_back("build " + std::to_string(target) + " " +
                        std::to_string(first_source) + " " +
                        std::to_string(second_source));
        handle = {17u, 29u};
    }
    void ResetHandle(pal4::CombatEffectHandle& handle) override {
        calls.push_back("reset");
        before_reset = handle;
        handle = {0xffffffffu, 0u};
        after_reset = handle;
    }
};

void BuildThenReset() {
    Runtime runtime;
    pal4::CombatEffectHandleAction::Process(1000u, 300u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "build 300 1100 1116", "reset"} &&
              runtime.before_reset == pal4::CombatEffectHandle{17u, 29u} &&
              runtime.after_reset ==
                  pal4::CombatEffectHandle{0xffffffffu, 0u},
          "builder receives action field addresses before handle reset");
}

}  // namespace

int main() {
    try {
        BuildThenReset();
        std::cout << "combat effect handle action tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
