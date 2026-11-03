#include "pal4/combat_actor_status.h"

#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatActorStatusRuntime {
    std::vector<std::string> calls;
    std::int32_t query = 0;
    std::int32_t current = 0;
    std::int32_t maximum = 100;
    std::int32_t set_result = 73;

    std::int32_t Query(std::uint32_t actor,
                       std::uint32_t offset) override {
        calls.push_back("query " + std::to_string(actor) + " " +
                        std::to_string(offset));
        return query;
    }
    std::int32_t QueryIndexed(std::uint32_t actor,
                              std::uint32_t offset,
                              std::uint32_t index) override {
        calls.push_back("indexed " + std::to_string(actor) + " " +
                        std::to_string(offset) + " " +
                        std::to_string(index));
        return offset == 0x30u ? maximum : current;
    }
    std::int32_t Set(std::uint32_t actor, std::uint32_t offset,
                      std::int32_t value) override {
        calls.push_back("set " + std::to_string(actor) + " " +
                        std::to_string(offset) + " " +
                        std::to_string(value));
        return set_result;
    }
};

void Forwarders() {
    Runtime runtime;
    runtime.query = 3;
    Check(pal4::CombatActorStatus::IsStateThree(1000u, runtime) &&
              !pal4::CombatActorStatus::IsNotStateThree(1000u, runtime),
          "state-three predicates use virtual offset 0xa8");
    runtime.query = 1;
    Check(!pal4::CombatActorStatus::IsStateThree(1000u, runtime) &&
              pal4::CombatActorStatus::IsNotStateThree(1000u, runtime) &&
              pal4::CombatActorStatus::IsStatusOne(1000u, runtime),
          "state and status predicates compare exact integer values");
    runtime.query = 0;
    Check(!pal4::CombatActorStatus::IsStatusOne(1000u, runtime),
          "status predicate rejects values other than one");
    Check(pal4::CombatActorStatus::SetSecondaryStateOne(1000u, runtime) == 73 &&
              pal4::CombatActorStatus::SetSecondaryStateZero(1000u, runtime) == 73 &&
              pal4::CombatActorStatus::ResetStatus(1000u, runtime) == 73,
          "setter wrappers forward virtual results");
    Check(runtime.calls == std::vector<std::string>{
              "query 1000 168", "query 1000 168", "query 1000 168",
              "query 1000 168", "query 1000 176", "query 1000 176",
              "set 1000 196 1",
              "set 1000 196 0", "set 1000 180 0"},
          "all six wrappers preserve virtual slot and argument order");
}

void ConditionalStatus() {
    Runtime runtime;
    runtime.current = 11;
    Check(pal4::CombatActorStatus::QueryConditionalStatus(
              2000u, 2, runtime) == 20 &&
              pal4::CombatActorStatus::QueryConditionalStatus(
                  2000u, 1, runtime) == 15 &&
              runtime.calls == std::vector<std::string>{
                  "indexed 2000 52 4", "indexed 2000 52 4"},
          "mode two adds nine; every other mode adds four");
    runtime.current = std::numeric_limits<std::int32_t>::max();
    Check(pal4::CombatActorStatus::QueryConditionalStatus(
              2000u, 2, runtime) ==
              std::numeric_limits<std::int32_t>::min() + 8,
          "conditional addition wraps like the native 32-bit instruction");
}

void HealthBands() {
    Runtime runtime;
    for (const auto& item : std::vector<std::pair<std::int32_t, std::int32_t>>{
             {-1, 3}, {0, 3}, {1, 2}, {20, 2},
             {21, 1}, {40, 1}, {41, 0}, {100, 0}}) {
        runtime.calls.clear();
        runtime.current = item.first;
        Check(pal4::CombatActorStatus::UpdateHealthStatus(
                  3000u, runtime) == 73,
              "health update forwards state setter result");
        const auto expected = std::vector<std::string>{
            "indexed 3000 52 7",
            "indexed 3000 48 0",
            "set 3000 172 " + std::to_string(item.second)};
        if (item.first <= 0)
            Check(runtime.calls == std::vector<std::string>{
                      "indexed 3000 52 7",
                      "set 3000 172 3"},
                  "nonpositive health skips base-stat query");
        else
            Check(runtime.calls == expected,
                  "positive health chooses the exact 20/40 percent band");
    }
    runtime.calls.clear();
    runtime.current = 1;
    runtime.maximum = 0;
    Check(pal4::CombatActorStatus::UpdateHealthStatus(
              3000u, runtime) == 73 &&
              runtime.calls.back() == "set 3000 172 0",
          "positive health over zero maximum selects the high band");
    runtime.maximum = -10;
    runtime.calls.clear();
    Check(pal4::CombatActorStatus::UpdateHealthStatus(
              3000u, runtime) == 73 &&
              runtime.calls.back() == "set 3000 172 2",
          "negative maximum produces a ratio in the lowest band");
}

}  // namespace

int main() {
    try {
        Forwarders();
        ConditionalStatus();
        HealthBands();
        std::cout << "combat actor status tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
