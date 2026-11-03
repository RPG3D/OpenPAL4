#include "pal4/combat_probability_calculator.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void Near(double actual, double expected, const char* message) {
    Check(std::fabs(actual - expected) < 0.00001, message);
}

struct Runtime final : pal4::ICombatProbabilityRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, float> factors{
        {4u, 0.1f}, {9u, 1.0f}, {10u, 1.5f},
        {11u, 0.1f}, {19u, 0.3f},
        {20u, 0.02f}, {21u, 0.05f}, {22u, 0.4f}};
    std::vector<std::string> calls;
    std::vector<std::uint32_t> indices;
    std::uint32_t random = 500u;
    int database_reads = 0;
    int random_reads = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words[address];
    }
    std::uint32_t GameDatabase() override {
        calls.emplace_back("database");
        ++database_reads;
        return 0x4000u;
    }
    float FloatArrayElement(std::uint32_t table,
                            std::uint32_t index) override {
        Check(table == 0x4000u + 184u, "GameDB float table offset");
        calls.push_back("factor:" + std::to_string(index));
        indices.push_back(index);
        return factors.at(index);
    }
    std::uint32_t RandomNumber() override {
        calls.emplace_back("random");
        ++random_reads;
        return random;
    }
    std::int32_t ActorStat(std::uint32_t actor,
                           std::uint32_t index) override {
        Check(actor == 0x2000u && index == 5u,
              "critical multiplier attacker stat slot");
        calls.emplace_back("stat:5");
        return 4;
    }
};

void Prepare(Runtime& runtime) {
    runtime.words[0x3000u + 28u] = 10u;
    runtime.words[0x2000u + 28u] = 12u;
    runtime.words[0x2000u + 36u] = std::bit_cast<std::uint32_t>(0.1f);
    runtime.words[0x3000u + 220u] = 10u;
    runtime.words[0x2000u + 220u] = 15u;
    runtime.words[0x3000u + 24u] = 10u;
    runtime.words[0x2000u + 24u] = 12u;
    runtime.words[0x2000u + 44u] = std::bit_cast<std::uint32_t>(0.1f);
}

void HitRateAndRoll() {
    Runtime runtime;
    Prepare(runtime);
    Near(pal4::CombatProbabilityCalculator::HitRate(
             0x2000u, 0x3000u, runtime), 0.7,
         "hit rate combines stat delta, GameDB and attacker bonus");
    Check(runtime.calls == std::vector<std::string>{
              "read:12316", "read:8220", "database", "factor:4",
              "database", "factor:22", "read:8228"} &&
              runtime.database_reads == 2 &&
              runtime.indices == std::vector<std::uint32_t>{4u, 22u},
          "hit rate operand and GameDB order");

    runtime.calls.clear();
    runtime.factors[22u] = 2.0f;
    Near(pal4::CombatProbabilityCalculator::HitRate(
             0x2000u, 0x3000u, runtime), 1.0,
         "hit rate upper clamp");
    runtime.factors[22u] = -2.0f;
    Near(pal4::CombatProbabilityCalculator::HitRate(
             0x2000u, 0x3000u, runtime), 0.0,
         "hit rate lower clamp");

    Runtime equal;
    equal.factors[4u] = 0.0f;
    equal.factors[22u] = 0.5f;
    equal.random = 500u;
    Check(!pal4::CombatProbabilityCalculator::CheckHit(
              0x2000u, 0x3000u, equal) &&
              equal.calls.front() == "random" &&
              equal.random_reads == 1,
          "hit roll is first and equality misses");
    equal.random = 1499u;
    Check(pal4::CombatProbabilityCalculator::CheckHit(
              0x2000u, 0x3000u, equal),
          "hit roll uses modulo 1000 and strict greater-than");
}

void DodgeRateAndRoll() {
    Runtime runtime;
    Prepare(runtime);
    Near(pal4::CombatProbabilityCalculator::DodgeRate(
             0x2000u, 0x3000u, runtime), 0.6,
         "dodge rate combines two stat deltas and bonus");
    Check(runtime.calls == std::vector<std::string>{
              "read:12508", "read:8412", "database", "factor:20",
              "read:12312", "read:8216", "database", "factor:21",
              "database", "factor:19", "read:8236"} &&
              runtime.database_reads == 3 &&
              runtime.indices ==
                  std::vector<std::uint32_t>{20u, 21u, 19u},
          "dodge rate read and GameDB order");

    runtime.factors[19u] = 2.0f;
    Near(pal4::CombatProbabilityCalculator::DodgeRate(
             0x2000u, 0x3000u, runtime), 1.0,
         "dodge rate upper clamp");
    runtime.factors[19u] = -2.0f;
    Near(pal4::CombatProbabilityCalculator::DodgeRate(
             0x2000u, 0x3000u, runtime), 0.0,
         "dodge rate lower clamp");

    Runtime equal;
    equal.factors[20u] = 0.0f;
    equal.factors[21u] = 0.0f;
    equal.factors[19u] = 0.5f;
    Check(!pal4::CombatProbabilityCalculator::CheckDodge(
              0x2000u, 0x3000u, 0.0f, equal) &&
              equal.calls.front() == "random",
          "dodge equality fails after random-first rate calculation");
    equal.factors[19u] = 2.0f;
    Check(!pal4::CombatProbabilityCalculator::CheckDodge(
              0x2000u, 0x3000u, -0.5f, equal),
          "extra dodge bonus is added after rate clamp");
    Check(pal4::CombatProbabilityCalculator::CheckDodge(
              0x2000u, 0x3000u, -0.25f, equal),
          "extra dodge bonus can leave rate above roll");
}

void CriticalRateAndRoll() {
    Runtime runtime;
    Near(pal4::CombatProbabilityCalculator::CriticalRate(
             1.0f, 2.0f, runtime), 0.5,
         "critical rate divides numerator by denominator");
    Check(runtime.calls == std::vector<std::string>{
              "database", "factor:9"},
          "critical rate reads one database factor");
    runtime.factors[9u] = 2.0f;
    Near(pal4::CombatProbabilityCalculator::CriticalRate(
             1.0f, 1.0f, runtime), 1.0,
         "critical rate upper clamp");
    runtime.factors[9u] = 1.0f;
    Near(pal4::CombatProbabilityCalculator::CriticalRate(
             -0.2f, 1.0f, runtime), -0.2,
         "critical rate has no lower clamp");

    Runtime equal;
    Check(!pal4::CombatProbabilityCalculator::CheckCritical(
              1.0f, 2.0f, equal) &&
              equal.calls.front() == "random",
          "critical equality fails with random-first call order");
    equal.random = 499u;
    Check(pal4::CombatProbabilityCalculator::CheckCritical(
              1.0f, 2.0f, equal),
          "critical roll below rate succeeds");
}

void FloatPropertyAndCriticalMultiplier() {
    Runtime runtime;
    runtime.words[0x3000u + 40u] = std::bit_cast<std::uint32_t>(0.49f);
    Near(pal4::CombatProbabilityCalculator::FloatPropertyThreshold(
             0x2000u, 0x3000u, runtime),
         static_cast<double>(0.49f) + static_cast<double>(0.01f),
         "float property receives 0.01 offset");
    Check(runtime.calls == std::vector<std::string>{"read:12328"},
          "float property reads target +40 only");
    runtime.calls.clear();
    runtime.words[0x3000u + 40u] =
        std::bit_cast<std::uint32_t>(-0.01f);
    runtime.random = 0u;
    Check(!pal4::CombatProbabilityCalculator::CheckFloatProperty(
              0x2000u, 0x3000u, runtime) &&
              runtime.calls ==
                  std::vector<std::string>{"random", "read:12328"},
          "float-property equality fails after random-first roll");
    runtime.words[0x3000u + 40u] =
        std::bit_cast<std::uint32_t>(0.0f);
    Check(pal4::CombatProbabilityCalculator::CheckFloatProperty(
              0x2000u, 0x3000u, runtime),
          "zero roll passes positive offset threshold");

    Runtime critical;
    Near(pal4::CombatProbabilityCalculator::CriticalDamageMultiplier(
             0x2000u, critical), 1.1,
         "critical multiplier subtracts scaled attacker stat");
    Check(critical.calls == std::vector<std::string>{
              "database", "factor:10", "stat:5", "database", "factor:11"},
          "critical multiplier GameDB and stat order");
    critical.factors[10u] = 0.2f;
    Near(pal4::CombatProbabilityCalculator::CriticalDamageMultiplier(
             0x2000u, critical), static_cast<double>(0.01f),
         "negative critical multiplier returns 0.01");
    critical.factors[10u] = 1.0f;
    critical.factors[11u] = 0.25f;
    Near(pal4::CombatProbabilityCalculator::CriticalDamageMultiplier(
             0x2000u, critical), 0.0,
         "zero critical multiplier remains zero");
}

void IndexedRandomConditions() {
    using CheckCondition = bool (*)(
        std::uint32_t, std::uint32_t, pal4::ICombatProbabilityRuntime&);
    struct Case {
        CheckCondition check;
        std::uint32_t last;
        std::uint32_t offset;
    };
    const std::vector<Case> cases{
        {&pal4::CombatProbabilityCalculator::CheckPrimaryRandomCondition,
         5u, 144u},
        {&pal4::CombatProbabilityCalculator::CheckSecondaryRandomCondition,
         6u, 168u},
        {&pal4::CombatProbabilityCalculator::CheckTertiaryRandomCondition,
         1u, 196u},
    };
    for (const auto& entry : cases) {
        Runtime runtime;
        const auto last_address = 0x1000u + entry.offset + entry.last * 4u;
        runtime.words[last_address] = std::bit_cast<std::uint32_t>(0.5f);
        runtime.random = 500u;
        Check(!entry.check(0x1000u, entry.last, runtime) &&
                  runtime.calls == std::vector<std::string>{
                      "read:" + std::to_string(last_address), "random"},
              "last indexed threshold is read before equality-failing roll");

        runtime.calls.clear();
        runtime.random = 499u;
        Check(entry.check(0x1000u, entry.last, runtime),
              "roll below indexed threshold succeeds");

        runtime.calls.clear();
        runtime.random = 999u;
        Check(entry.check(0x1000u, entry.last + 1u, runtime) &&
                  runtime.calls == std::vector<std::string>{"random"},
              "out-of-range positive index becomes threshold directly");

        runtime.calls.clear();
        Check(!entry.check(0x1000u, 0xffffffffu, runtime) &&
                  runtime.calls == std::vector<std::string>{"random"},
              "large unsigned index is interpreted as signed threshold");
    }
}

}  // namespace

int main() {
    try {
        HitRateAndRoll();
        DodgeRateAndRoll();
        CriticalRateAndRoll();
        FloatPropertyAndCriticalMultiplier();
        IndexedRandomConditions();
        std::cout << "combat probability calculator tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
