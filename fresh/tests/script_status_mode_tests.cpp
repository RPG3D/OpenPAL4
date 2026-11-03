#include "pal4/script_status_mode.h"

#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct Runtime final : pal4::IScriptStatusModeRuntime {
    std::vector<std::string> calls;
    std::int32_t expected_mode = 0;

    std::uint32_t ActorControl() override {
        calls.emplace_back("actor");
        return 0x4000u;
    }
    std::uint32_t ActivateStatusMode(
        std::uint32_t actor, std::int32_t mode) override {
        if (actor != 0x4000u || mode != expected_mode)
            throw std::runtime_error("status mode operands");
        calls.emplace_back("activate");
        return 0x80000000u + static_cast<std::uint32_t>(mode);
    }
};

}  // namespace

int main() {
    try {
        for (const auto mode : {
                 std::numeric_limits<std::int32_t>::min(), -1, 0, 4,
                 std::numeric_limits<std::int32_t>::max()}) {
            Runtime runtime;
            if (pal4::ScriptStatusMode::Enable(mode, runtime) !=
                    static_cast<std::uint32_t>(mode) ||
                !runtime.calls.empty())
                throw std::runtime_error("invalid mode must return unchanged");
        }
        for (std::int32_t mode = 1; mode <= 3; ++mode) {
            Runtime runtime;
            runtime.expected_mode = mode;
            if (pal4::ScriptStatusMode::Enable(mode, runtime) !=
                    0x80000000u + static_cast<std::uint32_t>(mode) ||
                runtime.calls != std::vector<std::string>{
                    "actor", "activate"})
                throw std::runtime_error("valid mode activation or return");
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
