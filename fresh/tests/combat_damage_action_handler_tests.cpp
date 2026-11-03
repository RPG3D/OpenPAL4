#include "pal4/combat_damage_action_handler.h"

#include <bit>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatDamageActionRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::int32_t> stat_seven;
    std::map<std::uint32_t, std::int32_t> parameter_seven;
    std::vector<std::string> calls;
    std::uint32_t gate_random = 0u;
    bool throw_on_candidate = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return words[address];
    }
    std::int32_t ActorStat(std::uint32_t actor,
                           std::uint32_t index) override {
        calls.push_back("stat " + std::to_string(actor) + " " +
                        std::to_string(index));
        if (index == 7u) return parameter_seven.contains(actor)
            ? parameter_seven.at(actor) : stat_seven[actor];
        if (index == 3u) return 20;
        return 0;
    }
    std::int32_t CurrentStat(std::uint32_t actor,
                             std::uint32_t index) override {
        calls.push_back("current " + std::to_string(actor) + " " +
                        std::to_string(index));
        Check(index == 7u, "magic current HP receiver");
        return stat_seven[actor];
    }
    std::int32_t ActorBonus(std::uint32_t,
                            std::uint32_t) override {
        return 0;
    }
    std::int32_t ActorElement(std::uint32_t,
                              std::uint32_t) override {
        return 0;
    }
    double ActorDamageFactor(std::uint32_t,
                             std::uint32_t) override {
        return 1.0;
    }
    double ActorTargetFactor(std::uint32_t,
                             std::uint32_t) override {
        return 1.0;
    }
    double DefenderSecondaryFactor(std::uint32_t,
                                   std::uint32_t) override {
        return 0.5;
    }
    double DefenderReduction(std::uint32_t,
                             std::uint32_t) override {
        return 0.0;
    }
    std::uint32_t GameDatabase() override {
        return 0x4000u;
    }
    float FloatArrayElement(std::uint32_t table,
                            std::uint32_t index) override {
        Check(table == 0x4000u + 184u, "damage database table");
        calls.push_back("factor " + std::to_string(index));
        return 1.0f;
    }
    std::uint32_t RandomNumber() override {
        calls.push_back("random");
        return gate_random;
    }
    double HalfValue(std::uint32_t,
                     std::uint32_t) override {
        return 0.5;
    }
    std::int64_t FloatToInt(float value) override {
        calls.push_back("convert " + std::to_string(value));
        return static_cast<std::int64_t>(value);
    }
    std::int32_t AddStat(std::uint32_t actor, std::uint32_t stat,
                         std::int64_t delta) override {
        calls.push_back("add " + std::to_string(actor) + " " +
                        std::to_string(stat) + " " +
                        std::to_string(delta));
        if (throw_on_candidate && actor == 111u)
            throw std::runtime_error("candidate add failed");
        if (stat == 7u) stat_seven[actor] += static_cast<std::int32_t>(delta);
        return 31;
    }
    void TargetFallback(std::uint32_t target,
                        std::uint32_t source) override {
        calls.push_back("fallback " + std::to_string(target) + " " +
                        std::to_string(source));
    }
    std::uint32_t EnumerateTargets(std::uint32_t target,
                                    std::uint8_t selector) override {
        calls.push_back("enumerate " + std::to_string(target) + " " +
                        std::to_string(selector));
        return 1u;
    }
    std::uint32_t VectorBegin(std::uint32_t vector) override {
        Check(vector == 1u, "damage vector handle");
        return 8000u;
    }
    std::uint32_t VectorEnd(std::uint32_t vector) override {
        Check(vector == 1u, "damage vector handle");
        return 8016u;
    }
    std::uint32_t CandidateStatus(std::uint32_t candidate) override {
        calls.push_back("status " + std::to_string(candidate));
        return candidate == 222u ? 4u : 1u;
    }
    bool CheckCandidateActive(std::uint32_t candidate) override {
        calls.push_back("active " + std::to_string(candidate));
        return candidate != 333u;
    }
    std::uint32_t FreeVector(std::uint32_t vector) override {
        Check(vector == 1u, "damage vector handle");
        calls.push_back("free");
        return 0x55u;
    }
};

constexpr std::uint32_t kAction = 1000u;
constexpr std::uint32_t kSource = 3000u;
constexpr std::uint32_t kTarget = 4000u;

void SetFloat(Runtime& runtime, std::uint32_t offset, float value) {
    runtime.words[kAction + offset] = std::bit_cast<std::uint32_t>(value);
}

void Prepare(Runtime& runtime) {
    runtime.stat_seven[kSource] = 10;
    runtime.stat_seven[kTarget] = 30;
    runtime.words[8000u] = 111u;
    runtime.words[8004u] = 0u;
    runtime.words[8008u] = 222u;
    runtime.words[8012u] = 333u;
    SetFloat(runtime, 20u, 0.0f);
    SetFloat(runtime, 24u, 0.0f);
    SetFloat(runtime, 28u, 0.5f);
    SetFloat(runtime, 32u, 1.0f);
    SetFloat(runtime, 36u, 0.0f);
}

std::size_t Find(const std::vector<std::string>& calls,
                 const std::string& expected) {
    for (std::size_t i = 0u; i < calls.size(); ++i)
        if (calls[i] == expected) return i;
    return calls.size();
}

void SkillGatesAndAccumulation() {
    Runtime rejected;
    Prepare(rejected);
    rejected.gate_random = 1u;
    Check(pal4::CombatDamageActionHandler::Skill(
              kAction, kSource, kTarget, rejected) == 0 &&
              rejected.calls == std::vector<std::string>{
                  "random", "read 1020"},
          "skill primary gate skips damage formula");

    Runtime direct;
    Prepare(direct);
    Check(pal4::CombatDamageActionHandler::Skill(
              kAction, kSource, kTarget, direct) == 31 &&
              direct.stat_seven[kTarget] == 20 &&
              direct.stat_seven[kSource] == 5 &&
              Find(direct.calls, "add 4000 7 -10") <
                  Find(direct.calls, "add 3000 7 -5") &&
              Find(direct.calls, "enumerate 4000 232") ==
                  direct.calls.size(),
          "skill primary and secondary damage use real formula on failed splash gate");

    Runtime splash;
    Prepare(splash);
    SetFloat(splash, 24u, 1.0f);
    Check(pal4::CombatDamageActionHandler::Skill(
              kAction, kSource, kTarget, splash) == 0x55 &&
              splash.stat_seven[kTarget] == 20 &&
              splash.stat_seven[111u] == -5 &&
              splash.stat_seven[kSource] == 3 &&
              Find(splash.calls, "enumerate 4000 232") <
                  Find(splash.calls, "add 111 7 -5") &&
              Find(splash.calls, "add 111 7 -5") <
                  Find(splash.calls, "add 3000 7 -7") &&
              splash.calls.back() == "free",
          "skill splash adds eligible secondary damage to source");
    Check(Find(splash.calls, "add 222 7 -5") == splash.calls.size() &&
              Find(splash.calls, "add 333 7 -5") == splash.calls.size() &&
              Find(splash.calls, "status 222") <
                  Find(splash.calls, "status 333"),
          "skill splash filters null, type-four and inactive candidates");

    Runtime zero;
    Prepare(zero);
    SetFloat(zero, 32u, 0.0f);
    Check(pal4::CombatDamageActionHandler::Skill(
              kAction, kSource, kTarget, zero) == 31 &&
              Find(zero.calls, "fallback 4000 3000") <
                  Find(zero.calls, "add 3000 7 0"),
          "zero skill damage invokes target fallback before source delta");
}

void MagicGatesAndSplash() {
    Runtime rejected;
    Prepare(rejected);
    rejected.gate_random = 1u;
    Check(pal4::CombatDamageActionHandler::Magic(
              kAction, kSource, kTarget, rejected) == 0 &&
              rejected.calls == std::vector<std::string>{
                  "random", "read 1020"},
          "magic primary gate skips formula");

    Runtime direct;
    Prepare(direct);
    Check(pal4::CombatDamageActionHandler::Magic(
              kAction, kSource, kTarget, direct) == 0 &&
              direct.stat_seven[kTarget] == 10 &&
              direct.stat_seven[kSource] == 0 &&
              Find(direct.calls, "add 4000 7 -20") <
                  Find(direct.calls, "current 4000 7") &&
              Find(direct.calls, "current 4000 7") <
                  Find(direct.calls, "add 3000 7 -10"),
          "magic direct path applies main damage then checks positive target stat");

    Runtime defeated;
    Prepare(defeated);
    defeated.stat_seven[kTarget] = 10;
    defeated.parameter_seven[kTarget] = 100;
    Check(pal4::CombatDamageActionHandler::Magic(
              kAction, kSource, kTarget, defeated) == 0 &&
              defeated.stat_seven[kSource] == 10 &&
              Find(defeated.calls, "add 3000 7 -10") ==
                  defeated.calls.size(),
          "nonpositive target stat suppresses source secondary magic delta");

    Runtime zero;
    Prepare(zero);
    SetFloat(zero, 32u, 0.0f);
    Check(pal4::CombatDamageActionHandler::Magic(
              kAction, kSource, kTarget, zero) == 0 &&
              Find(zero.calls, "fallback 4000 3000") < zero.calls.size() &&
              Find(zero.calls, "current 4000 7") == zero.calls.size(),
          "zero magic damage falls back without stat query");

    Runtime splash;
    Prepare(splash);
    SetFloat(splash, 24u, 1.0f);
    Check(pal4::CombatDamageActionHandler::Magic(
              kAction, kSource, kTarget, splash) == 0x55 &&
              splash.stat_seven[111u] == -10 &&
              splash.stat_seven[kSource] == 0 &&
              Find(splash.calls, "add 111 7 -10") <
                  Find(splash.calls, "free") &&
              splash.calls.back() == "free",
          "magic splash scales primary damage and frees candidate vector");

    Runtime throws;
    Prepare(throws);
    SetFloat(throws, 24u, 1.0f);
    throws.throw_on_candidate = true;
    bool caught = false;
    try {
        static_cast<void>(pal4::CombatDamageActionHandler::Magic(
            kAction, kSource, kTarget, throws));
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throws.calls.back() == "free",
          "magic candidate vector is released on exceptional exit");
}

}  // namespace

int main() {
    try {
        SkillGatesAndAccumulation();
        MagicGatesAndSplash();
        std::cout << "combat damage action handler tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
