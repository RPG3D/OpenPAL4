#include "pal4/combat_world_clump_gate.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatWorldClumpRuntime {
    std::int32_t result = 0;
    std::vector<std::string> trace;

    std::uint32_t CombatSystem() override {
        trace.push_back("combat-system");
        return 0x3000u;
    }
    std::int32_t AddClump(std::uint32_t world,
                          std::uint32_t clump) override {
        Check(world == 0x3000u + 116u && clump == 0x9000u,
              "combat world offset and clump argument");
        trace.push_back("add-clump");
        return result;
    }
};

}  // namespace

int main() {
    try {
        for (std::int32_t result : {-100, -1, 0, 1, 100}) {
            Runtime runtime;
            runtime.result = result;
            Check(pal4::CombatWorldClumpGate::Add(0x9000u, runtime) ==
                      (result > 0) &&
                      runtime.trace == std::vector<std::string>{
                          "combat-system", "add-clump"},
                  "signed positive-result gate");
        }
        std::cout << "combat_world_clump_gate_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
