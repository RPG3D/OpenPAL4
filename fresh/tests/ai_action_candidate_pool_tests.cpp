#include "pal4/ai_action_task.h"

#include <algorithm>
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
constexpr std::uint32_t kMate = 0x6000u;
constexpr std::uint32_t kRejected = 0x6100u;
constexpr std::uint32_t kEnemy = 0x7000u;
constexpr std::uint32_t kSupplied = 0x8000u;
constexpr std::uint32_t kTeam = 0x8100u;
constexpr std::uint32_t kEnemies = 0x8200u;

struct Runtime final : pal4::IAiActionPoolRuntime {
    std::uint32_t animation_id = 97u;
    bool throw_capability = false;
    std::map<std::uint32_t, std::vector<std::uint32_t>> groups{
        {kSupplied, {kActor, kMate, kRejected, kEnemy}},
        {kTeam, {kActor, kMate, kRejected}},
        {kEnemies, {kEnemy, kRejected}}};
    std::map<std::uint32_t, std::size_t> cursors{
        {kSupplied, 3u}, {kTeam, 0u}, {kEnemies, 0u}};
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == kTask + 4u, "pool uses task actor");
        calls.emplace_back("actor");
        return kActor;
    }
    std::uint32_t ActorAnimationId(std::uint32_t actor) override {
        Check(actor == kActor, "pool checks actor animation ID");
        calls.emplace_back("id");
        return animation_id;
    }
    std::uint32_t CreateActorIterator(
        std::uint32_t actor, pal4::AiActorGroup group) override {
        Check(actor == kActor, "new iterator uses actor");
        const auto iterator = group == pal4::AiActorGroup::Team
                                  ? kTeam : kEnemies;
        cursors[iterator] = 0u;
        calls.emplace_back(group == pal4::AiActorGroup::Team
                               ? "team" : "enemy");
        return iterator;
    }
    void IteratorReset(std::uint32_t iterator) override {
        Check(groups.count(iterator) == 1u,
              "reset uses supplied or created iterator");
        cursors[iterator] = 0u;
        calls.emplace_back("reset");
    }
    bool IteratorEnd(std::uint32_t iterator) override {
        Check(groups.count(iterator) == 1u, "iterator end handle");
        return cursors[iterator] == groups.at(iterator).size();
    }
    std::uint32_t IteratorCurrent(std::uint32_t iterator) override {
        Check(groups.count(iterator) == 1u &&
                  cursors[iterator] < groups.at(iterator).size(),
              "iterator current handle");
        calls.emplace_back("current");
        return groups.at(iterator)[cursors[iterator]];
    }
    void IteratorNext(std::uint32_t iterator) override {
        Check(groups.count(iterator) == 1u &&
                  cursors[iterator] < groups.at(iterator).size(),
              "iterator advance handle");
        ++cursors[iterator];
        calls.emplace_back("next");
    }
    void ReleaseIterator(std::uint32_t iterator) noexcept override {
        calls.emplace_back(iterator == kTeam ? "release-team"
                                            : "release-enemy");
    }
    bool IsEligible(std::uint32_t actor) override {
        calls.emplace_back("eligible");
        return actor != kRejected;
    }
    bool ActorCapability(std::uint32_t actor,
                         std::uint32_t mode) override {
        Check(mode == 7u, "candidate capability uses selected mode");
        calls.emplace_back("capability");
        if (throw_capability)
            throw std::runtime_error("capability failure");
        return actor != kEnemy;
    }
};

std::vector<std::uint32_t> Build(Runtime& runtime) {
    return pal4::AiActionCandidatePool::Build(
        kTask, kSupplied, 7u, runtime);
}

void SuppliedIterator() {
    Runtime runtime;
    Check(Build(runtime) ==
              std::vector<std::uint32_t>{kActor, kMate} &&
          runtime.calls[0] == "actor" &&
          runtime.calls[1] == "id" &&
          runtime.calls[2] == "id" &&
          runtime.calls[3] == "reset" &&
          runtime.cursors[kSupplied] == 4u &&
          runtime.calls.back() == "next" &&
          runtime.calls.end() ==
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "release-team"),
          "ordinary animation resets and filters borrowed iterator");
}

void CombinedGroup() {
    for (const auto animation : {98u, 99u}) {
        Runtime runtime;
        runtime.animation_id = animation;
        Check(Build(runtime) ==
                  std::vector<std::uint32_t>{kMate} &&
              runtime.calls[0] == "actor" &&
              runtime.calls[1] == "id" &&
              runtime.calls[animation == 98u ? 2u : 3u] == "team" &&
              runtime.calls.end() !=
                  std::find(runtime.calls.begin(), runtime.calls.end(),
                            "release-team") &&
              runtime.calls.back() == "release-enemy" &&
              runtime.cursors[kSupplied] == 3u,
              "special animations scan owned team then enemy iterators");
    }

    Runtime runtime;
    runtime.animation_id = 98u;
    runtime.throw_capability = true;
    bool threw = false;
    try {
        (void)Build(runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.calls.back() == "release-team" &&
          runtime.calls.end() ==
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "release-enemy"),
          "special pool creation releases first iterator on failure");
}

}  // namespace

int main() {
    try {
        SuppliedIterator();
        CombinedGroup();
        std::cout << "ai_action_candidate_pool_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
