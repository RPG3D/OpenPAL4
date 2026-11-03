#include "pal4/combat_random_condition_response.h"

#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatRandomConditionResponseRuntime {
    std::vector<std::string> calls;
    std::uint32_t random = 0u;
    float threshold = 0.0f;
    bool condition = true;
    std::int32_t stat = 12;

    std::uint32_t RandomNumber() override {
        calls.push_back("random");
        return random;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return std::bit_cast<std::uint32_t>(threshold);
    }
    bool CheckCondition(std::uint32_t address,
                        std::uint32_t mode) override {
        calls.push_back("condition " + std::to_string(address) + " " +
                        std::to_string(mode));
        return condition;
    }
    std::int32_t ApplyEvent(std::uint32_t target, std::uint32_t context,
                            std::uint32_t action,
                            std::uint32_t mode) override {
        calls.push_back("apply " + std::to_string(target) + " " +
                        std::to_string(context) + " " +
                        std::to_string(action) + " " + std::to_string(mode));
        return 81;
    }
    std::int32_t ActorStat(std::uint32_t target,
                           std::uint32_t slot) override {
        calls.push_back("stat " + std::to_string(target) + " " +
                        std::to_string(slot));
        return stat;
    }
    std::int32_t AdjustStat(std::uint32_t target, std::uint32_t slot,
                            std::int32_t delta) override {
        calls.push_back("adjust " + std::to_string(target) + " " +
                        std::to_string(slot) + " " + std::to_string(delta));
        return 42;
    }
    std::int32_t TriggerEvent(std::uint32_t target) override {
        calls.push_back("trigger " + std::to_string(target));
        return 73;
    }
};

void GateAndEvent() {
    Runtime rejected;
    rejected.random = 1001u;
    rejected.threshold = 0.0f;
    Check(pal4::CombatRandomConditionResponse::Process(
              1000u, 20u, 30u, rejected) == 0 &&
              rejected.calls == std::vector<std::string>{
                  "random", "read 1020"},
          "random uses modulo 1000 and rejects above threshold");

    Runtime equal;
    equal.random = 1000u;
    equal.threshold = 0.0f;
    Check(pal4::CombatRandomConditionResponse::Process(
              1000u, 20u, 30u, equal) == 81 &&
              equal.calls == std::vector<std::string>{
                  "random", "read 1020", "condition 370 1",
                  "apply 30 20 1000 1"},
          "equal roll passes and event branch forwards arguments and result");

    Runtime unordered;
    unordered.threshold = std::numeric_limits<float>::quiet_NaN();
    Check(pal4::CombatRandomConditionResponse::Process(
              1000u, 20u, 30u, unordered) == 81,
          "unordered comparison passes through x87 status mask");
}

void StatFallback() {
    Runtime fallback;
    fallback.condition = false;
    fallback.stat = -19;
    Check(pal4::CombatRandomConditionResponse::Process(
              1000u, 20u, 30u, fallback) == 42 &&
              fallback.calls == std::vector<std::string>{
                  "random", "read 1020", "condition 370 1",
                  "stat 30 7", "adjust 30 7 19"},
          "failed condition negates stat seven and forwards adjustment result");

    Runtime minimum;
    minimum.condition = false;
    minimum.stat = std::numeric_limits<std::int32_t>::min();
    Check(pal4::CombatRandomConditionResponse::Process(
              1000u, 20u, 30u, minimum) == 42 &&
              minimum.calls.back() == "adjust 30 7 -2147483648",
          "minimum signed stat preserves native 32-bit negation wrap");
}

void Trigger() {
    Runtime rejected;
    rejected.random = 1u;
    Check(pal4::CombatRandomConditionResponse::Trigger(
              1000u, 30u, rejected) == 0 &&
              rejected.calls == std::vector<std::string>{
                  "random", "read 1020"},
          "random trigger rejects above threshold");

    Runtime equal;
    equal.random = 1000u;
    Check(pal4::CombatRandomConditionResponse::Trigger(
              1000u, 30u, equal) == 73 &&
              equal.calls == std::vector<std::string>{
                  "random", "read 1020", "trigger 30"},
          "equal roll invokes target virtual and forwards result");

    Runtime unordered;
    unordered.threshold = std::numeric_limits<float>::quiet_NaN();
    Check(pal4::CombatRandomConditionResponse::Trigger(
              1000u, 30u, unordered) == 73,
          "unordered threshold passes random trigger gate");
}

}  // namespace

int main() {
    try {
        GateAndEvent();
        StatFallback();
        Trigger();
        std::cout << "combat random condition response tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
