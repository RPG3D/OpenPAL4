#include "pal4/ai_action_task.h"

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

constexpr std::uint32_t kTask = 0x4000u;
constexpr std::uint32_t kActor = 0x5000u;

struct Runtime final : pal4::IAiCollectedSkillCandidateRuntime {
    std::map<std::uint32_t, std::uint32_t> words{{kTask + 4u, kActor}};
    std::vector<std::uint32_t> collected{0x7000u, 0x7100u, 0x7200u};
    std::vector<std::string> calls;
    pal4::AiPipelineKind expected_kind = pal4::AiPipelineKind::Magic;
    std::uint32_t expected_capability = 3u;
    std::uint32_t expected_state = 7u;
    std::uint32_t random = 999u;
    double converted_value = -1.0;
    bool capability = true;
    std::uint32_t unusable = 0u;
    bool throw_on_use = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words.at(address);
    }
    bool ActorCapability(std::uint32_t actor,
                         std::uint32_t code) override {
        Check(actor == kActor && code == expected_capability,
              "actor capability matches selected kind");
        calls.emplace_back("capability");
        return capability;
    }
    std::vector<std::uint32_t> CollectRecords(
        std::uint32_t collection, std::uint32_t state,
        pal4::AiPipelineKind kind) override {
        Check(collection == kActor + 920u && state == expected_state &&
                  kind == expected_kind,
              "collection receives actor +920, state and kind");
        calls.emplace_back("collect");
        return collected;
    }
    bool CanUseSkill(std::uint32_t actor,
                     pal4::AiPipelineKind kind,
                     std::uint32_t skill) override {
        Check(actor == kActor && kind == expected_kind,
              "actor skill gate receives original actor and kind");
        if (throw_on_use) throw std::runtime_error("skill gate failed");
        calls.emplace_back("use:" + std::to_string(skill));
        return skill != unusable;
    }
    std::uint32_t RandomNumber() override {
        calls.emplace_back("random");
        return random;
    }
    std::int32_t ConvertToInt(double value) override {
        calls.emplace_back("convert");
        converted_value = value;
        return static_cast<std::int32_t>(value);
    }
};

void MagicSelection() {
    Runtime runtime;
    Check(pal4::AiCollectedSkillCandidateSelector::SelectIndexedMagic(
              kTask, 7u, runtime) == 0x7200u &&
          runtime.converted_value ==
              999.0 * static_cast<double>(0.001f) * 3.0 &&
          runtime.calls == std::vector<std::string>{
              "read:" + std::to_string(kTask + 4u), "capability",
              "collect", "use:28672", "use:28928", "use:29184",
              "random", "convert"},
          "indexed Magic preserves collected order and chooses last at roll 999");

    runtime = Runtime{};
    runtime.random = 1000u;
    runtime.unusable = 0x7100u;
    Check(pal4::AiCollectedSkillCandidateSelector::SelectIndexedMagic(
              kTask, 7u, runtime) == 0x7000u &&
          runtime.converted_value == 0.0,
          "roll wraps modulo 1000 and rejected skill is not a candidate");
}

void StuntSelectionAndGuards() {
    Runtime runtime;
    runtime.expected_kind = pal4::AiPipelineKind::Stunt;
    runtime.expected_capability = 2u;
    runtime.words[0x7000u] = 5509u;
    runtime.words[0x7100u] = 5508u;
    runtime.words[0x7200u] = 5510u;
    Check(pal4::AiCollectedSkillCandidateSelector::SelectFixedStunt(
              kTask, 7u, runtime) == 0x7200u &&
          runtime.converted_value ==
              999.0 * static_cast<double>(0.001f) * 2.0 &&
          runtime.calls[4] == "read:28672",
          "fixed Stunt excludes ID 5509 after actor use gate");

    runtime = Runtime{};
    runtime.capability = false;
    Check(pal4::AiCollectedSkillCandidateSelector::SelectIndexedMagic(
              kTask, 7u, runtime) == 0u &&
          runtime.calls.size() == 2u,
          "capability failure skips collection");

    runtime = Runtime{};
    runtime.collected.clear();
    Check(pal4::AiCollectedSkillCandidateSelector::SelectIndexedMagic(
              kTask, 7u, runtime) == 0u &&
          runtime.calls.size() == 3u,
          "empty source returns before random generation");

    runtime = Runtime{};
    runtime.unusable = 0x7000u;
    runtime.collected = {0x7000u};
    Check(pal4::AiCollectedSkillCandidateSelector::SelectIndexedMagic(
              kTask, 7u, runtime) == 0u &&
          runtime.calls.back() == "use:28672",
          "all rejected records return zero");

    runtime = Runtime{};
    runtime.throw_on_use = true;
    bool threw = false;
    try {
        (void)pal4::AiCollectedSkillCandidateSelector::SelectIndexedMagic(
            kTask, 7u, runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw, "actor gate exception propagates with vector cleanup");
}

}  // namespace

int main() {
    try {
        MagicSelection();
        StuntSelectionAndGuards();
        std::cout << "ai_collected_skill_candidate_selector_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
