#include "pal4/script_actor_control_reset.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptActorControlResetRuntime {
    std::vector<std::string> calls;
    std::uint32_t expected_context = 0u;

    std::uint32_t ActorControl() override {
        calls.emplace_back("control");
        return 0x1000u;
    }
    void ResetPlayerToJumpStart(std::uint32_t control,
                                std::uint32_t context) override {
        Check(control == 0x1000u && context == expected_context,
              "actor-control reset operands");
        calls.emplace_back("reset");
    }
};

}  // namespace

int main() {
    try {
        for (auto context : {0u, 0x12345678u, 0xffffffffu}) {
            Runtime runtime;
            runtime.expected_context = context;
            pal4::ScriptActorControlReset::ToJumpStart(context, runtime);
            Check(runtime.calls == std::vector<std::string>{
                      "control", "reset"},
                  "actor control acquired before reset tail call");
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
