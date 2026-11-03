#include "pal4/script_combat_ground_camera.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct Runtime final : pal4::IScriptCombatGroundCameraRuntime {
    std::vector<std::string> calls;
    std::uint32_t expected_text = 0u;
    std::uint8_t result = 0u;

    std::uint32_t CombatSystem() override {
        calls.emplace_back("system");
        return 0x4000u;
    }
    std::uint8_t UpdateText(std::uint32_t system,
                            std::uint32_t text) override {
        if (system != 0x4000u || text != expected_text)
            throw std::runtime_error("ground camera arguments");
        calls.emplace_back("update");
        return result;
    }
};

}  // namespace

int main() {
    try {
        for (const auto text : {0u, 0x12345678u}) {
            Runtime runtime;
            runtime.expected_text = text;
            runtime.result = 0xf6u;
            if (pal4::ScriptCombatGroundCamera::Configure(text, runtime) !=
                    0xf6u ||
                runtime.calls != std::vector<std::string>{"system", "update"})
                throw std::runtime_error("camera update return or order");
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
