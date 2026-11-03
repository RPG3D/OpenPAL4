#include "pal4/combat_report_preparer.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kOutput = 0x1000u;
constexpr std::uint32_t kScratch = 0x2000u;
constexpr std::uint32_t kIterator = 0x3000u;
constexpr std::uint32_t kActorA = 0x4000u;
constexpr std::uint32_t kActorB = 0x5000u;
constexpr std::uint32_t kActorC = 0x6000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatReportRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, float> floats;
    std::vector<std::uint32_t> actors;
    std::vector<std::uint32_t> rolls;
    std::vector<std::string> calls;
    std::size_t actor_index = 0u;
    std::size_t roll_index = 0u;
    bool throw_on_reward = false;
    bool iterator_destroyed = false;
    bool report_destroyed = false;

    void ConstructReport(std::uint32_t report, std::uint8_t tree_seed,
                         std::uint8_t record_seed) override {
        Check(report == kScratch && tree_seed == 0x12u &&
                  record_seed == 0x34u,
              "scratch report and explicit stack seeds");
        calls.emplace_back("construct");
        for (std::uint32_t offset = 0u; offset < 16u; offset += 4u)
            words[report + offset] = 0u;
        words[report + 16u] = 3u;
    }
    void DestroyReport(std::uint32_t report) override {
        Check(report == kScratch, "destroy scratch report");
        report_destroyed = true;
        calls.emplace_back("destroy-report");
    }
    std::uint32_t CombatStats() override {
        calls.emplace_back("stats");
        return 0x7000u;
    }
    std::int32_t TotalStat(std::uint32_t stats) override {
        Check(stats == 0x7000u, "total stat source");
        calls.emplace_back("total");
        return 88;
    }
    std::int32_t BattleRank(std::uint32_t stats,
                            std::int32_t total) override {
        Check(stats == 0x7000u && total == 88, "battle rank inputs");
        calls.emplace_back("rank");
        return 2;
    }
    std::uint32_t FirstMonsterIterator() override {
        calls.emplace_back("first");
        return kIterator;
    }
    bool IteratorDone(std::uint32_t iterator) override {
        Check(iterator == kIterator, "iterator identity");
        calls.emplace_back("done");
        return actor_index == actors.size();
    }
    std::uint32_t IteratorActor(std::uint32_t iterator) override {
        Check(iterator == kIterator, "current iterator identity");
        calls.emplace_back("actor");
        return actors.at(actor_index);
    }
    void AdvanceIterator(std::uint32_t iterator) override {
        Check(iterator == kIterator, "advance iterator identity");
        ++actor_index;
        calls.emplace_back("next");
    }
    void DestroyIterator(std::uint32_t iterator) override {
        Check(iterator == kIterator, "delete iterator identity");
        iterator_destroyed = true;
        calls.emplace_back("destroy-iterator");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::int32_t ReadInt(std::uint32_t address) override {
        return static_cast<std::int32_t>(words[address]);
    }
    float ReadFloat(std::uint32_t address) override {
        return floats[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write:" + std::to_string(address));
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == kOutput && value ==
                  static_cast<std::uint8_t>(words[kScratch]),
              "low score byte written before score word");
        calls.emplace_back("byte:" + std::to_string(address));
    }
    std::uint32_t RandomNumber() override {
        calls.emplace_back("random");
        return rolls.at(roll_index++);
    }
    std::int32_t FloatToInt(double value) override {
        calls.emplace_back("ftol");
        return static_cast<std::int32_t>(value);
    }
    std::int32_t ActorMetric(std::uint32_t actor,
                             std::uint32_t key) override {
        Check(key == 5u, "actor metric selector");
        calls.emplace_back("metric");
        return actor == kActorA ? 7 : 11;
    }
    std::uint32_t FindOrInsertReward(std::uint32_t tree,
                                     std::uint32_t item,
                                     std::uint32_t zero) override {
        Check(tree == kScratch + 20u && zero == 0u,
              "reward tree and zero payload");
        if (throw_on_reward) throw std::runtime_error("insert failed");
        calls.emplace_back("reward:" + std::to_string(item));
        return 0x8000u + item;
    }
    void AssignTree(std::uint32_t destination,
                    std::uint32_t source) override {
        calls.emplace_back("tree:" + std::to_string(destination) + ":" +
                           std::to_string(source));
    }
    void CopyReward(std::uint32_t destination,
                    std::uint32_t source) override {
        calls.emplace_back("copy:" + std::to_string(destination) + ":" +
                           std::to_string(source));
    }
};

void WriteActor(Runtime& runtime, std::uint32_t actor,
                std::uint32_t state, std::int32_t first,
                std::int32_t second) {
    runtime.words[actor + 0x33cu] = state;
    runtime.words[actor + 0x394u] = static_cast<std::uint32_t>(first);
    runtime.words[actor + 0x398u] = static_cast<std::uint32_t>(second);
}

void MixedMonstersAndRewards() {
    Runtime runtime;
    runtime.actors = {kActorA, kActorB, kActorC};
    WriteActor(runtime, kActorA, 4u, 20, 10);
    WriteActor(runtime, kActorB, 2u, 100, 100);
    WriteActor(runtime, kActorC, 3u, 4, 8);
    runtime.words[kActorA + 0x3a0u] = 111u;
    runtime.words[kActorA + 0x3a8u] = 111u;
    runtime.words[kActorA + 0x3acu] = 222u;
    runtime.floats[kActorA + 0x3b0u] = 1.0f;
    runtime.floats[kActorA + 0x3b8u] = 0.0f;
    runtime.floats[kActorA + 0x3bcu] = 0.0f;
    runtime.words[kActorC + 0x3a0u] = 111u;
    runtime.floats[kActorC + 0x3b0u] = 1.0f;
    runtime.rolls = {500u, 999u, 0u, 1u, 250u, 100u};

    Check(pal4::CombatReportPreparer::Prepare(
              kOutput, kScratch, 0x12u, 0x34u, runtime) == kOutput,
          "returns destination");
    Check(runtime.roll_index == runtime.rolls.size() &&
              runtime.actor_index == runtime.actors.size(),
          "eligible actors consume only their own random rolls");
    Check(runtime.words[kOutput] == 20u &&
              runtime.words[kOutput + 4u] == 88u &&
              runtime.words[kOutput + 8u] == 2u &&
              runtime.words[kOutput + 12u] == 18u &&
              runtime.words[kOutput + 16u] == 3u,
          "scores, total, rank, metric sum and default field copied");
    Check(runtime.words[0x8000u + 111u + 16u] == 3u &&
              runtime.words[0x8000u + 222u + 16u] == 0u,
          "matching drops increment existing tree node including zero roll");
    Check(runtime.calls.front() == "construct" &&
              runtime.calls[1] == "stats" &&
              runtime.calls[2] == "total" &&
              runtime.calls[3] == "stats" &&
              runtime.calls[4] == "rank" &&
              runtime.calls[5] == "first",
          "report setup follows IDA call order");
    const std::array<std::string, 8> tail{
        "tree:4116:8212", "tree:4132:8228",
        "copy:4148:8244", "copy:4188:8284",
        "copy:4228:8324", "copy:4268:8364",
        "destroy-iterator", "destroy-report"};
    Check(runtime.calls.size() >= tail.size(), "report copy tail exists");
    for (std::size_t i = 0; i < tail.size(); ++i)
        Check(runtime.calls[runtime.calls.size() - tail.size() + i] == tail[i],
              "two tree and four reward copies precede cleanup");
    Check(runtime.iterator_destroyed && runtime.report_destroyed,
          "temporary resources released");
}

void CallbackFailureUnwinds() {
    Runtime runtime;
    runtime.actors = {kActorA};
    WriteActor(runtime, kActorA, 3u, 0, 0);
    runtime.words[kActorA + 0x3a0u] = 7u;
    runtime.floats[kActorA + 0x3b0u] = 1.0f;
    runtime.rolls = {0u, 0u};
    runtime.throw_on_reward = true;
    bool threw = false;
    try {
        (void)pal4::CombatReportPreparer::Prepare(
            kOutput, kScratch, 0x12u, 0x34u, runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.iterator_destroyed && runtime.report_destroyed &&
              runtime.calls[runtime.calls.size() - 2u] == "destroy-iterator" &&
              runtime.calls.back() == "destroy-report",
          "iterator and scratch report unwind after reward failure");
}

void UnorderedDropProbability() {
    Runtime runtime;runtime.actors={kActorA};WriteActor(runtime,kActorA,4u,0,0);
    runtime.words[kActorA+0x3a0u]=7u;
    runtime.floats[kActorA+0x3b0u]=std::numeric_limits<float>::quiet_NaN();runtime.rolls={0u,999u};
    (void)pal4::CombatReportPreparer::Prepare(kOutput,kScratch,0x12u,0x34u,runtime);
    Check(runtime.words[0x8000u+7u+16u]==1u,"original x87 unordered report drop comparison accepts NaN");
}

}  // namespace

int main() {
    try {
        MixedMonstersAndRewards();
        CallbackFailureUnwinds();
        UnorderedDropProbability();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
