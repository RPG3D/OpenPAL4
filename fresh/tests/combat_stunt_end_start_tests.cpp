#include "pal4/combat_stunt_sequence_task.h"
#include "pal4/check_protect_task.h"
#include "pal4/start_attack_task.h"

#include <array>
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

constexpr std::uint32_t kTask = 0x1000u;
constexpr std::uint32_t kSource = 0x2000u;
constexpr std::uint32_t kTarget = 0x3000u;
constexpr std::uint32_t kStunt = 0x4000u;

struct AddedTask {
    std::uint32_t child;
    std::uint32_t text;
    std::array<std::uint32_t, 3> timing;
};

struct Runtime final : pal4::IEndStuntStartRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 44u, kSource}, {kTask + 48u, kTarget},
        {kTask + 52u, kStunt}, {kStunt + 132u, 1u},
        {kSource + 804u, 1u}, {kTarget + 804u, 2u},
        {0x8f9784u, 0x9100u}, {0x8f97acu, 0x9200u},
        {0x8f9798u, 0x9300u}};
    std::vector<std::uint32_t> factories;
    std::vector<AddedTask> added;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> run_sources;
    bool change_source_on_add = false;
    std::size_t fail_child = 99u;
    std::uint32_t run_result = 0x8800u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    std::uint32_t CreateChildTask(std::uint32_t factory) override {
        factories.push_back(factory);
        calls.emplace_back("create");
        return factories.size() == fail_child ? 0u :
               static_cast<std::uint32_t>(0x2000u + factories.size() * 0x1000u);
    }
    void AddTaskAndInitialize(
        std::uint32_t container, std::uint32_t child,
        std::uint32_t text,
        const std::array<std::uint32_t, 3>& timing) override {
        Check(container == kTask + 4u &&
                  timing == std::array<std::uint32_t, 3>{1u, 0u, 0u},
              "EndStunt child GOM arguments");
        added.push_back({child, text, timing});
        calls.emplace_back("add");
        if (change_source_on_add) words[kTask + 44u] = 0x2100u;
    }
    pal4::StuntActorPosition ActorPosition(
        std::uint32_t source) override {
        Check(source == kSource || source == 0x2100u,
              "EndStunt actor position source");
        calls.emplace_back("actor_position");
        return {0x7000u, {1.0f, 2.0f, 3.0f}};
    }
    std::uint32_t SetApproachTaskTarget(
        std::uint32_t child, std::uint32_t source,
        const pal4::StuntActorPosition& position) override {
        Check(child == added.back().child &&
                  source == words.at(kTask + 44u) &&
                  position.address == 0x7000u &&
                  position.values ==
                      std::array<float, 3>{1.0f, 2.0f, 3.0f},
              "EndStunt RunTask position arguments");
        calls.emplace_back("set_run");
        run_sources.push_back(source);
        return run_result;
    }
};

void NoOpReturns() {
    Runtime runtime;
    runtime.words[kStunt + 132u] = 7u;
    Check(pal4::EndStuntSequenceTask::Start(kTask, runtime) == 7u &&
              runtime.factories.empty() &&
              runtime.calls == std::vector<std::string>{
                  "read:4148", "read:16516"},
          "unknown mode returns mode without actor reads");
    for (const auto mode : {1u, 2u}) {
        runtime = Runtime{};
        runtime.words[kStunt + 132u] = mode;
        runtime.words[kTarget + 804u] = 1u;
        Check(pal4::EndStuntSequenceTask::Start(kTask, runtime) == 1u &&
                  runtime.factories.empty(),
              "same side returns source side without children");
    }
}

void ModeOneRuns() {
    Runtime runtime;
    Check(pal4::EndStuntSequenceTask::Start(kTask, runtime) == 0x8800u &&
              runtime.factories == std::vector<std::uint32_t>{
                  pal4::StartAttackApproachFactory::kPrimaryVtable,
                  pal4::StartAttackApproachFactory::kSecondaryVtable} &&
              runtime.added.at(0).text == 0x9100u &&
              runtime.added.at(1).text == 0x9200u &&
              runtime.run_sources ==
                  std::vector<std::uint32_t>{kSource, kSource},
          "mode one schedules two RunTask children and forwards final result");
    runtime = Runtime{};
    runtime.change_source_on_add = true;
    Check(pal4::EndStuntSequenceTask::Start(kTask, runtime) == 0x8800u &&
              runtime.run_sources ==
                  std::vector<std::uint32_t>{0x2100u, 0x2100u},
          "RunTask reads source again after GOM add");
    runtime = Runtime{};
    runtime.run_result = 0u;
    Check(pal4::EndStuntSequenceTask::Start(kTask, runtime) == 0u &&
              runtime.factories.size() == 2u,
          "zero RunTask result still schedules second child");
    runtime = Runtime{};
    runtime.words[0x8f9784u] = 0u;
    runtime.words[0x8f97acu] = 0u;
    Check(pal4::EndStuntSequenceTask::Start(kTask, runtime) == 0x8800u &&
              runtime.added.at(0).text == 0x841964u &&
              runtime.added.at(1).text == 0x841964u,
          "both RunTask texts fall back to empty global");
}

void ModeTwoPosition() {
    Runtime runtime;
    runtime.words[kStunt + 132u] = 2u;
    Check(pal4::EndStuntSequenceTask::Start(kTask, runtime) == 0x7000u &&
              runtime.factories == std::vector<std::uint32_t>{
                  pal4::CheckProtectSubtaskFactory::kPositionVtable} &&
              runtime.added.at(0).text == 0x9300u &&
              runtime.words.at(runtime.added.at(0).child + 72u) == kSource &&
              runtime.words.at(runtime.added.at(0).child + 60u) ==
                  std::bit_cast<std::uint32_t>(1.0f) &&
              runtime.words.at(runtime.added.at(0).child + 64u) ==
                  std::bit_cast<std::uint32_t>(2.0f) &&
              runtime.words.at(runtime.added.at(0).child + 68u) ==
                  std::bit_cast<std::uint32_t>(3.0f),
          "mode two copies actor position and returns position pointer");
    runtime = Runtime{};
    runtime.words[kStunt + 132u] = 2u;
    runtime.words[0x8f9798u] = 0u;
    runtime.change_source_on_add = true;
    Check(pal4::EndStuntSequenceTask::Start(kTask, runtime) == 0x7000u &&
              runtime.added.at(0).text == 0x841964u &&
              runtime.words.at(runtime.added.at(0).child + 72u) == 0x2100u,
          "position child rereads source after add and uses fallback text");
}

void NullChildStages() {
    for (const auto stage : {1u, 2u}) {
        Runtime runtime;
        runtime.fail_child = stage;
        Check(pal4::EndStuntSequenceTask::Start(kTask, runtime) == 0u &&
                  runtime.factories.size() == stage &&
                  runtime.added.size() == stage - 1u,
              "null mode-one child exits before native invalid write");
    }
    Runtime runtime;
    runtime.words[kStunt + 132u] = 2u;
    runtime.fail_child = 1u;
    Check(pal4::EndStuntSequenceTask::Start(kTask, runtime) == 0u &&
              runtime.added.empty(),
          "null position child exits before native invalid write");
}

}  // namespace

int main() {
    try {
        NoOpReturns();
        ModeOneRuns();
        ModeTwoPosition();
        NullChildStages();
        std::cout << "Combat EndStunt start tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
