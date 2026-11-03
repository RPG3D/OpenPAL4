#include "pal4/combat_system_render_callback.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatSystemRenderCallbackRuntime,
                       pal4::ICombatSystemRenderEventRuntime {
    std::vector<std::uint32_t> calls;
    std::uint8_t inactive = 0u;
    std::uint32_t GetCombatSystem() override {
        calls.push_back(1u);
        return 0x7000u;
    }
    std::uint32_t ProcessEvent61(std::uint32_t combat_system,
                                 std::uint32_t argument) override {
        Check(combat_system == 0x7000u && argument == 0x1234u,
              "render callback forwards singleton and original argument");
        calls.push_back(2u);
        return pal4::CombatSystemRenderCallback::ProcessEvent61(
            combat_system, argument, *this);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == 0x7000u + 516u,
              "event61 reads CombatSystem inactive byte");
        calls.push_back(3u);
        return inactive;
    }
    std::uint32_t ForwardEvent61(std::uint32_t combat_system,
                                 std::uint32_t argument) override {
        Check(combat_system == 0x7000u && argument == 0x1234u,
              "active event61 forwards object and argument");
        calls.push_back(4u);
        return 0xdeadbeefu;
    }
};

}  // namespace

int main() {
    try {
        Runtime runtime;
        Check(pal4::CombatSystemRenderCallback::Invoke(
                  0x1234u, runtime) == 0xdeadbeefu &&
                  runtime.calls ==
                      std::vector<std::uint32_t>{1u, 2u, 3u, 4u},
              "render callback fetches singleton then forwards active event");
        runtime.calls.clear();
        runtime.inactive = 1u;
        Check(pal4::CombatSystemRenderCallback::Invoke(
                  0x1234u, runtime) == 2u &&
                  runtime.calls ==
                      std::vector<std::uint32_t>{1u, 2u, 3u},
              "inactive event returns two without actor dispatch");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
