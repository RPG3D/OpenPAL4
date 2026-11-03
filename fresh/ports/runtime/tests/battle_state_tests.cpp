#include "pal4/portable/battle_state.h"

#include "pal4/combat_actor_status.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct StatusRuntime final : pal4::ICombatActorStatusRuntime {
    std::int32_t current = 0;
    std::int32_t maximum = 100;
    std::int32_t state = -1;
    std::vector<std::string> calls;

    std::int32_t Query(std::uint32_t, std::uint32_t) override { return 0; }
    std::int32_t QueryIndexed(std::uint32_t, std::uint32_t offset,
                              std::uint32_t index) override {
        calls.push_back("query " + std::to_string(offset) + " " +
                        std::to_string(index));
        return offset == 0x30u ? maximum : current;
    }
    std::int32_t Set(std::uint32_t, std::uint32_t offset,
                     std::int32_t value) override {
        calls.push_back("set " + std::to_string(offset) + " " +
                        std::to_string(value));
        state = value;
        return value;
    }
};

void HealthBands() {
    struct Case {
        std::int32_t current;
        std::int32_t maximum;
        std::int32_t expected;
    };
    constexpr std::array cases{
        Case{-1, 100, 3}, Case{0, 100, 3}, Case{1, 100, 2},
        Case{20, 100, 2}, Case{21, 100, 1}, Case{40, 100, 1},
        Case{41, 100, 0}, Case{100, 100, 0}, Case{1, 0, 0},
        Case{1, -10, 2},
    };
    for (const auto& item : cases) {
        const auto portable = pal4::portable::BattleHealthState(
            item.current, item.maximum);
        Check(portable == item.expected,
              "shared health rule must preserve native 20/40 percent bands");

        StatusRuntime runtime;
        runtime.current = item.current;
        runtime.maximum = item.maximum;
        Check(pal4::CombatActorStatus::UpdateHealthStatus(1u, runtime) ==
                  item.expected && runtime.state == portable,
              "native actor status must agree with portable battle state");
        const auto expected_calls = item.current <= 0
            ? std::vector<std::string>{"query 52 7", "set 172 3"}
            : std::vector<std::string>{"query 52 7", "query 48 0",
                                       "set 172 " + std::to_string(item.expected)};
        Check(runtime.calls == expected_calls,
              "native status integration must retain its actor query sequence");
    }
}

}  // namespace

int main() {
    try {
        HealthBands();
        std::cout << "portable battle health state tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
