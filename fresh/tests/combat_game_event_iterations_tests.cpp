#include "pal4/combat_game_event_iterations.h"

#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

constexpr std::uint32_t kObject = 0x1000u;
constexpr std::uint32_t kManager = 0x7000u;
constexpr std::uint32_t kFirstActor = 0xa000u;
constexpr std::uint32_t kSecondActor = 0xa100u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatGameEventIterationRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::size_t> positions;
    std::unordered_map<std::uint32_t, std::uint32_t> states;
    std::unordered_map<std::uint32_t, std::uint32_t> first_properties;
    std::unordered_map<std::uint32_t, std::uint32_t> second_properties;
    std::unordered_map<std::uint32_t, std::uint32_t> metrics;
    std::vector<std::uint32_t> actors;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> updates;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> destroyed;
    std::vector<std::uint32_t> average_arguments;
    std::uint32_t manager_calls = 0u;
    std::uint32_t iterator_calls = 0u;
    std::int32_t first_average = 10;
    std::int32_t second_average = 20;
    std::uint32_t throw_on_stat = 0xffffffffu;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t ActorFromFacet(std::uint32_t) override { return 0u; }
    std::uint32_t UpdateStat(std::uint32_t object,
                             std::uint32_t stat,
                             std::uint32_t increment) override {
        Check(object == kObject, "iteration updates event handler stats");
        if (stat == throw_on_stat) throw std::runtime_error("stat failed");
        updates.emplace_back(stat, increment);
        return 0x9000u + stat;
    }
    std::uint32_t GetActorManager() override {
        ++manager_calls;
        return kManager;
    }
    std::uint32_t CreateIterator(std::uint32_t collection) override {
        Check(collection == kManager + 32u,
              "iteration uses actor manager Player first collection at +32");
        const auto iterator = 0x8000u + iterator_calls++ * 0x100u;
        positions[iterator] = 0u;
        return iterator;
    }
    std::uint32_t IteratorDone(std::uint32_t iterator) override {
        return positions.at(iterator) >= actors.size() ? 1u : 0u;
    }
    std::uint32_t IteratorCurrent(std::uint32_t iterator) override {
        return actors.at(positions.at(iterator));
    }
    void IteratorAdvance(std::uint32_t iterator) override {
        ++positions.at(iterator);
    }
    std::uint32_t DestroyIterator(std::uint32_t iterator,
                                  std::uint32_t flag) override {
        destroyed.emplace_back(iterator, flag);
        return 0xf000u + iterator_calls;
    }
    std::uint32_t MonsterState(std::uint32_t actor) override {
        return states.at(actor);
    }
    std::uint32_t ActorProperty52(std::uint32_t actor,
                                  std::uint32_t selector) override {
        Check(selector == 7u, "first actor property uses selector 7");
        return first_properties.at(actor);
    }
    std::uint32_t ActorProperty48(std::uint32_t actor,
                                  std::uint32_t selector) override {
        Check(selector == 0u, "second actor property uses selector 0");
        return second_properties.at(actor);
    }
    std::uint32_t ActorMetric252(std::uint32_t actor) override {
        return metrics.at(actor);
    }
    std::int32_t AverageProperty4(std::uint32_t collection) override {
        average_arguments.push_back(collection);
        Check(collection == kManager || collection == kManager + 32u,
              "average uses actor manager or its collection");
        return collection == kManager ? first_average : second_average;
    }
};

void MonsterFleeIteration() {
    Runtime runtime;
    runtime.actors = {kFirstActor, kSecondActor, 0xa200u};
    runtime.states = {{kFirstActor, 2u}, {kSecondActor, 1u},
                      {0xa200u, 2u}};
    Check(pal4::CombatGameEventIterations::OnMonsterFlee(
              kObject, runtime) == 0xf001u &&
              runtime.updates ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {14u, 1u}, {14u, 1u}} &&
              runtime.positions[0x8000u] == 3u &&
              runtime.destroyed ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x8000u, 1u}},
          "monster flee scans actors, counts state 2 and deletes iterator");

    Runtime failing;
    failing.actors = {kFirstActor};
    failing.states[kFirstActor] = 2u;
    failing.throw_on_stat = 14u;
    bool threw = false;
    try {
        (void)pal4::CombatGameEventIterations::OnMonsterFlee(
            kObject, failing);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && failing.destroyed.size() == 1u &&
              failing.destroyed[0] == std::pair{0x8000u, 1u},
          "iterator is released when a stat update fails");
}

void BattleEndScansAndAverage() {
    Runtime runtime;
    runtime.actors = {kFirstActor, kSecondActor};
    runtime.words[kObject + 0x248u] = 0u;
    runtime.words[kObject + 0x24cu] = 1u;
    runtime.words[kObject + 0x250u] = 0u;
    runtime.words[kObject + 0x254u] = 3u;
    runtime.first_properties = {{kFirstActor, 5u}, {kSecondActor, 6u}};
    runtime.second_properties = {{kFirstActor, 5u}, {kSecondActor, 7u}};
    runtime.metrics = {{kFirstActor, 0u}, {kSecondActor, 3u}};
    Check(pal4::CombatGameEventIterations::OnBattleEnd(
              kObject, runtime) == 0x9013u &&
              runtime.updates ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {12u, 1u}, {12u, 1u}, {13u, 1u},
                      {15u, 0u}, {15u, 3u}, {19u, 10u}} &&
              runtime.manager_calls == 4u &&
              runtime.iterator_calls == 2u &&
              runtime.destroyed ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x8000u, 1u}, {0x8100u, 1u}} &&
              runtime.average_arguments ==
                  std::vector<std::uint32_t>{kManager, kManager + 32u},
          "battle end checks empty slots, two actor passes and positive average gap");
}

void BattleEndOtherBranches() {
    Runtime runtime;
    runtime.actors = {kFirstActor};
    runtime.first_properties[kFirstActor] = 2u;
    runtime.second_properties[kFirstActor] = 3u;
    runtime.metrics[kFirstActor] = 0u;
    for (std::uint32_t i = 0u; i < 4u; ++i)
        runtime.words[kObject + 0x248u + i * 4u] = 1u;
    runtime.first_average = 30;
    runtime.second_average = 20;
    Check(pal4::CombatGameEventIterations::OnBattleEnd(
              kObject, runtime) == 0x9014u &&
              runtime.updates ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {15u, 0u}, {16u, 1u}, {20u, 10u}},
          "zero metrics count stat 16 and a negative average gap counts stat 20");

    Runtime empty;
    for (std::uint32_t i = 0u; i < 4u; ++i)
        empty.words[kObject + 0x248u + i * 4u] = 1u;
    empty.first_average = 20;
    empty.second_average = 20;
    Check(pal4::CombatGameEventIterations::OnBattleEnd(
              kObject, empty) == 20u &&
              empty.updates ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {16u, 1u}} &&
              empty.destroyed.size() == 2u,
          "empty scans count stat 16 and equal averages return their value");

    Runtime extremes;
    for (std::uint32_t i = 0u; i < 4u; ++i)
        extremes.words[kObject + 0x248u + i * 4u] = 1u;
    extremes.first_average = std::numeric_limits<std::int32_t>::min();
    extremes.second_average = std::numeric_limits<std::int32_t>::max();
    (void)pal4::CombatGameEventIterations::OnBattleEnd(
        kObject, extremes);
    Check(extremes.updates.back() == std::pair{19u, 0xffffffffu},
          "average gap uses native 32-bit wraparound");

    Runtime failing;
    failing.actors = {kFirstActor};
    failing.first_properties[kFirstActor] = 1u;
    failing.second_properties[kFirstActor] = 1u;
    failing.throw_on_stat = 13u;
    bool threw = false;
    try {
        (void)pal4::CombatGameEventIterations::OnBattleEnd(
            kObject, failing);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && failing.destroyed.size() == 1u &&
              failing.destroyed[0] == std::pair{0x8000u, 1u},
          "first battle-end iterator is released after callback failure");
}

}  // namespace

int main() {
    try {
        MonsterFleeIteration();
        BattleEndScansAndAverage();
        BattleEndOtherBranches();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
