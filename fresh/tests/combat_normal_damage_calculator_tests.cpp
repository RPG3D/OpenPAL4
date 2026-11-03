#include "pal4/combat_normal_damage_calculator.h"

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

void Near(float actual, float expected, const char* message) {
    Check(std::fabs(actual - expected) < 0.001f, message);
}

struct Runtime final : pal4::ICombatNormalDamageRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, float> parameters{
        {0u, 2.0f}, {1u, 0.8f}, {2u, 1.2f}, {3u, 1.0f},
        {5u, 1.0f}, {6u, 0.9f}, {7u, 1.1f}};
    std::vector<std::string> calls;
    std::vector<std::uint32_t> parameter_indices;
    std::vector<std::uint32_t> defender_elements;
    std::uint32_t sample = 500u;
    int database_reads = 0;
    int random_reads = 0;
    int half_reads = 0;

    Runtime() { words[0x3000u + 660u] = 1u; }

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words[address];
    }
    std::int32_t ActorStat(std::uint32_t actor,
                           std::uint32_t index) override {
        calls.push_back("stat:" + std::to_string(actor) + ":" +
                        std::to_string(index));
        if (actor == 0x2000u && index == 3u) return 10;
        if (actor == 0x3000u && index == 4u) return 3;
        if (actor == 0x2000u && index == 7u) return 4;
        throw std::runtime_error("unexpected actor stat");
    }
    std::int32_t ActorBonus(std::uint32_t actor,
                            std::uint32_t index) override {
        Check((actor == 0x2000u && index <= 5u) ||
                  (actor == 0x3000u && index == 0u),
              "actor bonus slot");
        calls.push_back("bonus:" + std::to_string(index));
        return index == 0u ? 2 : 10;
    }
    std::int32_t ActorElement(std::uint32_t actor,
                              std::uint32_t index) override {
        Check(index < 5u, "element slot in five-element range");
        calls.push_back("element:" + std::to_string(actor) + ":" +
                        std::to_string(index));
        if (actor == 0x3000u)
            defender_elements.push_back(index);
        else
            Check(actor == 0x2000u, "element actor");
        return 0;
    }
    double ActorDamageFactor(std::uint32_t actor,
                             std::uint32_t mode) override {
        Check(actor == 0x2000u && mode <= 1u, "damage factor mode");
        calls.push_back("factor:" + std::to_string(mode));
        return 1.0;
    }
    double ActorTargetFactor(std::uint32_t actor,
                             std::uint32_t target_element) override {
        Check(actor == 0x2000u && target_element == 0x55u,
              "target element factor");
        calls.emplace_back("target-factor");
        return 1.0;
    }
    double DefenderSecondaryFactor(std::uint32_t defender,
                                   std::uint32_t mode) override {
        Check(defender == 0x3000u && (mode == 0u || mode == 5u),
              "defender secondary factor");
        calls.emplace_back("secondary-factor");
        return 0.2;
    }
    double DefenderReduction(std::uint32_t defender,
                             std::uint32_t mode) override {
        Check(defender == 0x3000u && (mode == 0u || mode == 5u),
              "defender reduction factor");
        calls.emplace_back("reduction");
        return 0.1;
    }
    std::uint32_t GameDatabase() override {
        calls.emplace_back("database");
        ++database_reads;
        return 0x4000u;
    }
    float FloatArrayElement(std::uint32_t table,
                            std::uint32_t index) override {
        Check(table == 0x4000u + 184u, "GameDB float table offset");
        calls.push_back("parameter:" + std::to_string(index));
        parameter_indices.push_back(index);
        return parameters.at(index);
    }
    std::uint32_t RandomNumber() override {
        calls.emplace_back("random");
        ++random_reads;
        return sample;
    }
    double HalfValue(std::uint32_t attacker,
                      std::uint32_t defender) override {
        Check(attacker == 0x2000u && defender == 0x3000u,
              "half-value arguments");
        calls.emplace_back("half");
        ++half_reads;
        return 0.5;
    }
};

std::size_t Index(const Runtime& runtime, const std::string& call) {
    for (std::size_t i = 0; i < runtime.calls.size(); ++i)
        if (runtime.calls[i] == call) return i;
    throw std::runtime_error("missing expected call");
}

pal4::CombatDamagePair Calculate(Runtime& runtime, bool skip_half = false) {
    pal4::CombatDamagePair result{123.0f, 456.0f};
    const auto returned = pal4::CombatNormalDamageCalculator::Calculate(
        result, 0x2000u, 0x3000u, skip_half, runtime);
    Check(returned == &result, "damage calculator returns output pointer");
    return result;
}

void FormulaAndCallShape() {
    Runtime runtime;
    runtime.words[0x3000u + 340u] = 0x55u;
    const auto result = Calculate(runtime);
    Near(result.main, -11.65f, "normal damage main result");
    Near(result.secondary, -3.4f, "normal damage secondary result");
    Check(runtime.half_reads == 1 && runtime.random_reads == 6 &&
              runtime.database_reads == 18,
          "half, random and GameDB call counts");
    Check(runtime.parameter_indices == std::vector<std::uint32_t>{
              0u, 1u, 2u, 5u, 6u, 7u, 5u, 6u, 7u,
              5u, 6u, 7u, 5u, 6u, 7u, 5u, 6u, 7u},
          "GameDB parameter read order");
    Check(runtime.defender_elements ==
              std::vector<std::uint32_t>{1u, 2u, 3u, 4u, 0u},
          "defender elemental indices wrap after four");
    Check(Index(runtime, "stat:8192:3") < Index(runtime, "parameter:0") &&
              Index(runtime, "parameter:0") < Index(runtime, "stat:12288:4") &&
              Index(runtime, "random") < Index(runtime, "factor:1") &&
              Index(runtime, "secondary-factor") < Index(runtime, "reduction") &&
              Index(runtime, "reduction") < Index(runtime, "half") &&
              Index(runtime, "half") < Index(runtime, "bonus:0") &&
              Index(runtime, "bonus:0") < Index(runtime, "stat:8192:7"),
          "main term and element loop call order");
}

void HalfGateAndClamp() {
    Runtime skipped;
    skipped.words[0x3000u + 340u] = 0x55u;
    const auto full = Calculate(skipped, true);
    Near(full.main, -19.3f, "skip-half keeps full damage");
    Check(skipped.half_reads == 0 &&
              Index(skipped, "reduction") < Index(skipped, "bonus:0"),
          "skip-half bypasses helper");
    Check(skipped.words.find(0x3000u + 660u) != skipped.words.end(),
          "half flag remains unmodified");
    for (const auto& call : skipped.calls)
        Check(call != "read:12948", "skip-half does not read half flag");

    Runtime flag_off;
    flag_off.words[0x3000u + 340u] = 0x55u;
    flag_off.words[0x3000u + 660u] = 0u;
    Near(Calculate(flag_off).main, -19.3f,
         "half flag zero keeps full damage");
    Check(flag_off.half_reads == 0, "flag zero skips helper");

    Runtime clamped;
    clamped.words[0x3000u + 340u] = 0x55u;
    clamped.parameters[0u] = 0.0f;
    const auto result = Calculate(clamped);
    Near(result.main, -4.0f, "negative stat delta clamps to zero");
    Near(result.secondary, 0.0f, "clamped secondary damage is zero");
}

void RangeSortingAndModulo() {
    Runtime forward;
    forward.words[0x3000u + 340u] = 0x55u;
    forward.sample = 250u;
    const auto expected = Calculate(forward);

    Runtime reversed;
    reversed.words[0x3000u + 340u] = 0x55u;
    reversed.sample = 1250u;
    reversed.parameters[1u] = 1.2f;
    reversed.parameters[2u] = 0.8f;
    reversed.parameters[6u] = 1.1f;
    reversed.parameters[7u] = 0.9f;
    const auto actual = Calculate(reversed);
    Near(actual.main, expected.main,
         "reversed parameter endpoints produce same main damage");
    Near(actual.secondary, expected.secondary,
         "random sample modulo 1000 and reversed endpoints");
}

void ExtendedFormula() {
    Runtime runtime;
    runtime.words[0x3000u + 340u] = 0x55u;
    pal4::CombatDamagePair output{};
    Check(pal4::CombatNormalDamageCalculator::CalculateExtended(
              output, 0x2000u, 0x3000u, false, runtime) == &output,
          "extended damage returns output pointer");
    Near(output.main, -11.65f, "extended main damage");
    Near(output.secondary, -3.4f, "extended secondary damage");
    Check(runtime.database_reads == 17 && runtime.random_reads == 5 &&
              runtime.half_reads == 1 &&
              runtime.parameter_indices == std::vector<std::uint32_t>{
                  0u, 3u, 5u, 6u, 7u, 5u, 6u, 7u,
                  5u, 6u, 7u, 5u, 6u, 7u, 5u, 6u, 7u},
          "extended formula uses parameter 3 and five element samples");
    Check(Index(runtime, "database") < Index(runtime, "stat:8192:3") &&
              Index(runtime, "stat:12288:4") < Index(runtime, "parameter:3") &&
              Index(runtime, "parameter:3") < Index(runtime, "factor:1"),
          "extended formula reads GameDB before attack stat");

    Runtime skipped;
    skipped.words[0x3000u + 340u] = 0x55u;
    pal4::CombatDamagePair full{};
    (void)pal4::CombatNormalDamageCalculator::CalculateExtended(
        full, 0x2000u, 0x3000u, true, skipped);
    Near(full.main, -19.3f, "extended skip-half result");
    Check(skipped.half_reads == 0, "extended skip-half bypasses helper");
}

void SkillFormula() {
    Runtime runtime;
    pal4::CombatDamagePair output{};
    Check(pal4::CombatNormalDamageCalculator::CalculateSkill(
              output, 0x2000u, 0x3000u, 4u, 2.0f, runtime) == &output,
          "skill damage returns output pointer");
    Near(output.main, -3.6f, "skill main damage");
    Near(output.secondary, -1.6f, "skill secondary damage");
    Check(runtime.database_reads == 3 && runtime.random_reads == 1 &&
              runtime.half_reads == 1 &&
              runtime.parameter_indices ==
                  std::vector<std::uint32_t>{5u, 6u, 7u} &&
              runtime.defender_elements == std::vector<std::uint32_t>{0u},
          "skill formula uses one random sample and wrapped target element");
    Check(Index(runtime, "stat:8192:7") < Index(runtime, "parameter:5") &&
              Index(runtime, "parameter:5") < Index(runtime, "element:8192:4") &&
              Index(runtime, "element:8192:4") <
                  Index(runtime, "element:12288:0") &&
              Index(runtime, "element:12288:0") < Index(runtime, "random") &&
              Index(runtime, "random") < Index(runtime, "factor:0") &&
              Index(runtime, "factor:0") < Index(runtime, "reduction") &&
              Index(runtime, "reduction") < Index(runtime, "secondary-factor") &&
              Index(runtime, "secondary-factor") < Index(runtime, "half"),
          "skill formula operand and factor order");

    Runtime no_half;
    no_half.words[0x3000u + 660u] = 0u;
    pal4::CombatDamagePair full{};
    (void)pal4::CombatNormalDamageCalculator::CalculateSkill(
        full, 0x2000u, 0x3000u, 4u, 2.0f, no_half);
    Near(full.main, -7.2f, "skill damage without half flag");
    Check(no_half.half_reads == 0, "skill half flag gate");

    Runtime negative;
    pal4::CombatDamagePair clamped{};
    (void)pal4::CombatNormalDamageCalculator::CalculateSkill(
        clamped, 0x2000u, 0x3000u, 4u, -2.0f, negative);
    Near(clamped.main, 0.0f, "negative skill damage clamps main");
    Near(clamped.secondary, 0.0f, "negative skill damage clamps secondary");
}

void MagicFormula() {
    Runtime runtime;
    runtime.words[0x3000u + 340u] = 0x55u;
    pal4::CombatDamagePair output{};
    Check(pal4::CombatNormalDamageCalculator::CalculateMagic(
              output, 0x2000u, 0x3000u, 1.0f, runtime) == &output,
          "magic damage returns output pointer");
    Near(output.main, -9.65f, "magic main damage");
    Near(output.secondary, -3.4f, "magic secondary damage");
    Check(runtime.database_reads == 1 && runtime.random_reads == 0 &&
              runtime.parameter_indices == std::vector<std::uint32_t>{0u} &&
              runtime.half_reads == 1 &&
              Index(runtime, "stat:8192:3") < Index(runtime, "parameter:0") &&
              Index(runtime, "reduction") < Index(runtime, "half") &&
              Index(runtime, "half") < Index(runtime, "bonus:0"),
          "magic formula uses one database factor and defender bonus last");

    Runtime full;
    full.words[0x3000u + 340u] = 0x55u;
    full.words[0x3000u + 660u] = 0u;
    pal4::CombatDamagePair full_output{};
    (void)pal4::CombatNormalDamageCalculator::CalculateMagic(
        full_output, 0x2000u, 0x3000u, 1.0f, full);
    Near(full_output.main, -17.3f, "magic half flag zero");
    Check(full.half_reads == 0, "magic half helper gate");

    Runtime clamped;
    clamped.words[0x3000u + 340u] = 0x55u;
    clamped.parameters[0u] = 0.0f;
    pal4::CombatDamagePair clamped_output{};
    (void)pal4::CombatNormalDamageCalculator::CalculateMagic(
        clamped_output, 0x2000u, 0x3000u, 1.0f, clamped);
    Near(clamped_output.main, -2.0f,
         "magic negative base clamps before defender flat bonus");
    Near(clamped_output.secondary, 0.0f,
         "magic secondary clamps with base");
}

void HalfConstant() {
    Check(pal4::CombatDamageConstants::HalfValue() == 0.5,
          "IDA half-value helper returns exact one half");
}

}  // namespace

int main() {
    try {
        FormulaAndCallShape();
        HalfGateAndClamp();
        RangeSortingAndModulo();
        ExtendedFormula();
        SkillFormula();
        MagicFormula();
        HalfConstant();
        std::cout << "combat normal damage calculator tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
