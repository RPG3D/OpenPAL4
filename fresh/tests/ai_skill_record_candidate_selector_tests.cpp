#include "pal4/ai_action_task.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kTask = 0x4000u;
constexpr std::uint32_t kActor = 0x5000u;
constexpr std::uint32_t kRecordBase = 0x7000u;

struct Runtime final : pal4::IAiSkillRecordCandidateRuntime {
    std::map<std::uint32_t, std::uint32_t> words{{kTask + 4u, kActor}};
    std::vector<std::string> calls;
    pal4::AiPipelineKind expected_kind = pal4::AiPipelineKind::Magic;
    std::uint32_t expected_capability = 3u;
    std::uint32_t random = 999u;
    double converted_value = -1.0;
    std::int32_t converted_index = 0;
    bool capability = true;
    bool fail_on_use = false;
    bool convert_actual = false;
    std::uint32_t rejected_key = 0u;
    std::uint32_t unusable_key = 0u;

    Runtime() {
        for (std::uint32_t key = 1u; key <= 5u; ++key) {
            words[kActor + 892u + 4u * key] = key;
            words[kRecordBase + key * 0x20u + 8u] = 7u;
            words[kRecordBase + key * 0x20u + 4u] = 9u;
        }
    }

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words.at(address);
    }
    bool ActorCapability(std::uint32_t actor,
                         std::uint32_t code) override {
        Check(actor == kActor && code == expected_capability,
              "capability uses actor and skill-specific code");
        calls.emplace_back("capability");
        return capability;
    }
    bool HasSkill(pal4::AiPipelineKind kind,
                  std::uint32_t key) override {
        Check(kind == expected_kind && key >= 1u && key <= 5u,
              "catalog membership uses each actor skill key");
        calls.emplace_back("has:" + std::to_string(key));
        return key != rejected_key;
    }
    std::uint32_t FindSkill(pal4::AiPipelineKind kind,
                            std::uint32_t key) override {
        Check(kind == expected_kind,
              "catalog lookup uses the selected skill kind");
        calls.emplace_back("find:" + std::to_string(key));
        return kRecordBase + key * 0x20u;
    }
    bool CanUseSkill(std::uint32_t actor,
                     pal4::AiPipelineKind kind,
                     std::uint32_t skill) override {
        Check(actor == kActor && kind == expected_kind,
              "actor virtual gate receives actor and skill kind");
        if (fail_on_use) throw std::runtime_error("actor use failed");
        calls.emplace_back("use:" + std::to_string(skill));
        return skill != kRecordBase + unusable_key * 0x20u;
    }
    std::uint32_t RandomNumber() override {
        calls.emplace_back("random");
        return random;
    }
    std::int32_t ConvertToInt(double value) override {
        calls.emplace_back("convert");
        converted_value = value;
        return convert_actual ? static_cast<std::int32_t>(value) : converted_index;
    }
};

void MagicCandidateFiltering() {
    Runtime runtime;
    runtime.rejected_key = 2u;
    runtime.unusable_key = 4u;
    runtime.words[kRecordBase + 3u * 0x20u + 8u] = 8u;
    Check(pal4::AiSkillRecordCandidateSelector::SelectMagic(
              kTask, 7u, runtime) == kRecordBase + 0x20u &&
          runtime.converted_value ==
              2.0 * (999.0 * static_cast<double>(0.001f)) &&
          runtime.calls.front() == "read:" + std::to_string(kTask + 4u) &&
          runtime.calls[1] == "capability" &&
          runtime.calls[runtime.calls.size() - 2u] == "random" &&
          runtime.calls.back() == "convert",
          "magic filters membership, state and actor gate before conversion");
    Check(runtime.calls.size() == 25u &&
          runtime.calls[4] == "find:1" &&
          runtime.calls[8] == "has:2" &&
          runtime.calls[11] == "find:3" &&
          runtime.calls[17] == "use:" +
              std::to_string(kRecordBase + 4u * 0x20u),
          "magic scans all five slots in order with short circuits");

    runtime = Runtime{};
    runtime.random = 1999u;
    runtime.converted_index = 1;
    Check(pal4::AiSkillRecordCandidateSelector::SelectMagic(
              kTask, 7u, runtime) == kRecordBase + 2u * 0x20u &&
          runtime.converted_value ==
              5.0 * (999.0 * static_cast<double>(0.001f)),
          "CRT conversion result indexes candidates after modulo 1000");
}

void CandidateRangeUsesActualCount() {
    for (const auto kind : {pal4::AiPipelineKind::Magic, pal4::AiPipelineKind::Stunt}) {
        for (const auto roll : {0u, 199u, 200u, 499u, 500u, 799u, 800u, 999u, 1999u}) {
            Runtime runtime;
            runtime.expected_kind = kind;
            runtime.expected_capability = kind == pal4::AiPipelineKind::Magic ? 3u : 2u;
            runtime.convert_actual = true;
            runtime.random = roll;
            const auto selected = kind == pal4::AiPipelineKind::Magic
                ? pal4::AiSkillRecordCandidateSelector::SelectMagic(kTask, 7u, runtime)
                : pal4::AiSkillRecordCandidateSelector::SelectStunt(kTask, 9u, runtime);
            const auto expected = kRecordBase + (1u + (roll % 1000u) / 200u) * 0x20u;
            Check(selected == expected, "five-candidate selection reaches every ordered bucket");
            Check(std::count(runtime.calls.begin(), runtime.calls.end(), "random") == 1,
                  "candidate selection consumes exactly one random draw");
        }
        Runtime filtered;
        filtered.expected_kind = kind;
        filtered.expected_capability = kind == pal4::AiPipelineKind::Magic ? 3u : 2u;
        filtered.convert_actual = true;
        filtered.random = 500u;
        filtered.rejected_key = 2u;
        filtered.unusable_key = 4u;
        filtered.words[kRecordBase + 3u * 0x20u +
            (kind == pal4::AiPipelineKind::Magic ? 8u : 4u)] = 99u;
        const auto selected = kind == pal4::AiPipelineKind::Magic
            ? pal4::AiSkillRecordCandidateSelector::SelectMagic(kTask, 7u, filtered)
            : pal4::AiSkillRecordCandidateSelector::SelectStunt(kTask, 9u, filtered);
        Check(selected == kRecordBase + 5u * 0x20u,
              "range scales by eligible candidates rather than five actor slots");
    }
}

void StuntAndEmptyPaths() {
    Runtime runtime;
    runtime.expected_kind = pal4::AiPipelineKind::Stunt;
    runtime.expected_capability = 2u;
    runtime.words[kRecordBase + 0x20u + 8u] = 7u;
    Check(pal4::AiSkillRecordCandidateSelector::SelectStunt(
              kTask, 9u, runtime) == kRecordBase + 0x20u,
          "stunt selects from +4 state and capability 2");

    runtime = Runtime{};
    runtime.capability = false;
    Check(pal4::AiSkillRecordCandidateSelector::SelectMagic(
              kTask, 7u, runtime) == 0u &&
          runtime.calls.size() == 2u,
          "disabled capability skips every skill slot");

    runtime = Runtime{};
    Check(pal4::AiSkillRecordCandidateSelector::SelectMagic(
              kTask, 99u, runtime) == 0u &&
          runtime.calls.size() == 22u &&
          runtime.calls.back() != "random",
          "no matching records return without random selection");

    runtime = Runtime{};
    runtime.fail_on_use = true;
    bool threw = false;
    try {
        (void)pal4::AiSkillRecordCandidateSelector::SelectMagic(
            kTask, 7u, runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw, "actor gate exception propagates with owned vector cleanup");
}

}  // namespace

int main() {
    try {
        MagicCandidateFiltering();
        StuntAndEmptyPaths();
        CandidateRangeUsesActualCount();
        std::cout << "ai_skill_record_candidate_selector_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
