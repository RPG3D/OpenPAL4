#include "pal4/ai_action_task.h"

#include <cmath>
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

constexpr std::uint32_t kSkill = 0x4000u;
constexpr std::uint32_t kActor1 = 0x5000u;
constexpr std::uint32_t kActor2 = 0x6000u;
constexpr std::uint32_t kActor3 = 0x7000u;

struct Runtime final : pal4::IAiTargetWeightRuntime {
    std::map<std::uint32_t, float> fields{
        {kSkill + 20u, 7.0f},
        {kSkill + 56u, 0.5f},
        {kSkill + 60u, 0.2f},
        {kSkill + 64u, 0.2f}};
    std::map<std::uint32_t, std::int32_t> hp{
        {kActor1, 100}, {kActor2, 25}, {kActor3, 40}};
    std::map<std::uint32_t, std::int32_t> maximum{
        {kActor1, 100}, {kActor2, 100}, {kActor3, 100}};
    std::map<std::uint32_t, std::int32_t> selectors{
        {kActor1, 7}, {kActor2, 8}, {kActor3, 7}};
    std::vector<std::uint32_t> random{100u};
    std::size_t random_index = 0u;
    std::vector<std::string> calls;

    float ReadFloat(std::uint32_t address) override {
        Check(fields.count(address) == 1u,
              "skill property uses known offset");
        calls.emplace_back("float");
        return fields.at(address);
    }
    std::uint32_t RandomNumber() override {
        Check(random_index < random.size(), "random call count");
        calls.emplace_back("random");
        return random[random_index++];
    }
    std::int32_t ConvertToInt(double value) override {
        calls.emplace_back("convert");
        return static_cast<std::int32_t>(value);
    }
    std::int32_t CurrentStat(std::uint32_t actor,
                             std::uint32_t stat) override {
        Check(stat == 7u && hp.count(actor) == 1u,
              "HP stat uses virtual +52 and ID 7");
        calls.emplace_back("hp");
        return hp.at(actor);
    }
    std::int32_t MaxStat(std::uint32_t actor,
                         std::uint32_t stat) override {
        Check(stat == 0u && maximum.count(actor) == 1u,
              "max HP uses virtual +48 and ID 0");
        calls.emplace_back("max-hp");
        return maximum.at(actor);
    }
    std::int32_t ActorSelector(std::uint32_t actor) override {
        Check(selectors.count(actor) == 1u,
              "selector uses actor virtual +256");
        calls.emplace_back("selector");
        return selectors.at(actor);
    }
};

void HelperFunctions() {
    Runtime runtime;
    Check(pal4::AiTargetWeightSelector::SkillProperty(
              kSkill, 11, runtime) == 0.2f &&
          runtime.calls == std::vector<std::string>{"float"},
          "property 11 reads skill +60");
    runtime.calls.clear();
    Check(pal4::AiTargetWeightSelector::SkillProperty(
              kSkill, 12, runtime) == 0.2f &&
          pal4::AiTargetWeightSelector::SkillProperty(
              kSkill, 13, runtime) == 13.0 &&
          pal4::AiTargetWeightSelector::SkillProperty(
              kSkill, -1, runtime) == -1.0 &&
          runtime.calls == std::vector<std::string>{"float"},
          "property 12 and unsigned out-of-range fallback");

    runtime = Runtime{};
    runtime.random = {1200u};
    const auto expected = 2.0 + 200.0 *
        static_cast<double>(0.001f) * 10.0;
    Check(std::abs(pal4::AiTargetWeightSelector::RandomRange(
              12.0f, 2.0f, runtime) - expected) < 1e-10 &&
          runtime.random_index == 1u,
          "random range swaps descending endpoints and uses modulo 1000");
}

void CandidateBranches() {
    Runtime runtime;
    std::vector<std::uint32_t> candidates;
    Check(pal4::AiTargetWeightSelector::Select(
              candidates, kSkill, 1u, runtime) == 0u &&
          runtime.calls.empty(),
          "empty candidates return before random number");

    runtime = Runtime{};
    candidates = {kActor1, kActor2, kActor3};
    runtime.random = {100u, 999u};
    Check(pal4::AiTargetWeightSelector::Select(
              candidates, kSkill, 2u, runtime) == kActor3 &&
          candidates ==
              std::vector<std::uint32_t>{kActor2, kActor3} &&
          runtime.random_index == 2u,
          "mode two filters full HP then selector branch chooses match");

    runtime = Runtime{};
    candidates = {kActor1, kActor2};
    runtime.random = {100u};
    runtime.fields[kSkill + 20u] = 99.0f;
    Check(pal4::AiTargetWeightSelector::Select(
              candidates, kSkill, 1u, runtime) == 0u &&
          runtime.random_index == 1u,
          "empty selector match returns zero without second random call");

    runtime = Runtime{};
    candidates = {kActor1, kActor2, kActor3};
    runtime.random = {500u};
    Check(pal4::AiTargetWeightSelector::Select(
              candidates, kSkill, 1u, runtime) == kActor2 &&
          runtime.random_index == 1u,
          "first threshold equality reaches minimum HP branch");

    runtime = Runtime{};
    runtime.fields[kSkill + 56u] = 0.2f;
    runtime.fields[kSkill + 60u] = 0.2f;
    runtime.fields[kSkill + 64u] = 0.3f;
    runtime.random = {500u, 750u};
    candidates = {kActor1, kActor2};
    Check(pal4::AiTargetWeightSelector::Select(
              candidates, kSkill, 1u, runtime) == kActor2 &&
          runtime.random_index == 2u,
          "third weight branch picks a uniformly random candidate");

    runtime = Runtime{};
    runtime.random = {900u};
    candidates = {kActor1, kActor2};
    Check(pal4::AiTargetWeightSelector::Select(
              candidates, kSkill, 1u, runtime) == 0u &&
          runtime.random_index == 1u,
          "uncovered probability tail returns zero");
}

void WoundedGateAndTies() {
    Runtime runtime;
    std::vector<std::uint32_t> candidates{kActor1};
    Check(pal4::AiTargetWeightSelector::Select(
              candidates, kSkill, 2u, runtime) == 0u &&
          candidates == std::vector<std::uint32_t>{kActor1} &&
          runtime.random_index == 0u,
          "mode two returns before random when nobody is wounded");

    runtime = Runtime{};
    runtime.hp[kActor3] = 25;
    runtime.random = {500u};
    candidates = {kActor3, kActor2};
    Check(pal4::AiTargetWeightSelector::Select(
              candidates, kSkill, 1u, runtime) == kActor3,
          "minimum HP branch keeps first candidate on a tie");
}

}  // namespace

int main() {
    try {
        HelperFunctions();
        CandidateBranches();
        WoundedGateAndTies();
        std::cout << "ai_target_weight_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
