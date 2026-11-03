#include "pal4/combat_actor_state_queries.h"

#include <algorithm>
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

struct Runtime final : pal4::ICombatActorStateQueryRuntime {
    std::vector<std::uint32_t> event_actors;
    std::vector<std::uint32_t> monsters;
    std::vector<std::uint32_t> ready;
    std::vector<std::uint32_t> zero;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::size_t position = 0u;
    bool fail_read = false;
    bool fail_ready = false;

    std::uint32_t ActorManager() override {
        calls.emplace_back("manager");
        return 0x2000u;
    }
    std::uint32_t FirstEvent37Iterator(
        std::uint32_t collection) override {
        Check(collection == 0x2000u + 32u,
              "event37 collection offset");
        calls.emplace_back("event-iterator");
        position = 0u;
        return 0x3000u;
    }
    std::uint32_t FirstMonsterIterator(
        std::uint32_t manager) override {
        Check(manager == 0x2000u, "monster manager identity");
        calls.emplace_back("monster-iterator");
        position = 0u;
        return 0x4000u;
    }
    const std::vector<std::uint32_t>& List(
        std::uint32_t iterator) const {
        Check(iterator == 0x3000u || iterator == 0x4000u,
              "iterator identity");
        return iterator == 0x3000u ? event_actors : monsters;
    }
    bool IteratorAtEnd(std::uint32_t iterator) override {
        calls.emplace_back("end");
        return position >= List(iterator).size();
    }
    std::uint32_t IteratorCurrent(
        std::uint32_t iterator) override {
        Check(position < List(iterator).size(), "iterator current bound");
        calls.emplace_back("current");
        return List(iterator)[position];
    }
    void IteratorAdvance(std::uint32_t iterator) override {
        (void)List(iterator);
        calls.emplace_back("advance");
        ++position;
    }
    void DestroyIterator(std::uint32_t iterator) override {
        (void)List(iterator);
        calls.emplace_back(iterator == 0x3000u ?
                           "destroy-event" : "destroy-monster");
    }
    bool IsActorReady(std::uint32_t actor) override {
        calls.emplace_back("ready:" + std::to_string(actor));
        if (fail_ready) throw std::runtime_error("ready failed");
        return std::find(ready.begin(), ready.end(), actor) != ready.end();
    }
    bool IsStateZero(std::uint32_t actor) override {
        calls.emplace_back("zero:" + std::to_string(actor));
        return std::find(zero.begin(), zero.end(), actor) != zero.end();
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        if (fail_read) throw std::runtime_error("read failed");
        return words[address];
    }
};

void ExtendedResults() {
    Runtime runtime;
    Check(pal4::CombatActorStateQueries::CheckExtended(runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "manager", "event-iterator", "end", "destroy-event"},
          "empty event collection returns zero without monster iterator");

    runtime.calls.clear();
    runtime.event_actors = {0x5000u};
    runtime.ready = {0x5000u};
    Check(pal4::CombatActorStateQueries::CheckExtended(runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "manager", "event-iterator", "end", "current",
                  "ready:20480", "destroy-event", "manager",
                  "monster-iterator", "end", "destroy-monster"},
          "first ready actor skips state-zero check; empty monsters yield one");

    runtime.calls.clear();
    runtime.ready.clear();
    runtime.zero = {0x5000u};
    runtime.monsters = {0x6000u, 0x7000u};
    runtime.ready = {0x7000u};
    Check(pal4::CombatActorStateQueries::CheckExtended(runtime) == 3u &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "manager") == 2 &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "advance") != runtime.calls.end() &&
              runtime.calls.back() == "destroy-monster",
          "state-zero event actor and later ready monster yield three");

    runtime.calls.clear();
    runtime.fail_ready = true;
    bool failed = false;
    try {
        (void)pal4::CombatActorStateQueries::CheckExtended(runtime);
    } catch (const std::runtime_error&) {
        failed = true;
    }
    Check(failed && runtime.calls.back() == "destroy-event",
          "event iterator is released on readiness failure");
}

void AnyTypeTwo() {
    Runtime runtime;
    runtime.event_actors = {0x5000u};
    runtime.words[0x5000u + 828u] = 2u;
    Check(pal4::CombatActorStateQueries::CheckActorsReady(runtime) &&
              runtime.calls == std::vector<std::string>{
                  "manager", "event-iterator", "end", "current",
                  "read:21308", "destroy-event"},
          "type-two event actor short-circuits monster scan");

    runtime.calls.clear();
    runtime.words[0x5000u + 828u] = 1u;
    runtime.monsters = {0x6000u};
    runtime.words[0x6000u + 828u] = 2u;
    Check(pal4::CombatActorStateQueries::CheckActorsReady(runtime) &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "manager") == 2 &&
              runtime.calls.back() == "destroy-monster",
          "monster type-two actor is searched after event collection");

    runtime.calls.clear();
    runtime.words[0x6000u + 828u] = 3u;
    Check(!pal4::CombatActorStateQueries::CheckActorsReady(runtime) &&
              runtime.calls.back() == "destroy-monster",
          "no type-two actor returns false after both collections");

    runtime.calls.clear();
    runtime.fail_read = true;
    bool failed = false;
    try {
        (void)pal4::CombatActorStateQueries::CheckActorsReady(runtime);
    } catch (const std::runtime_error&) {
        failed = true;
    }
    Check(failed && runtime.calls.back() == "destroy-event",
          "event iterator is released on type read failure");
}

}  // namespace

int main() {
    try {
        ExtendedResults();
        AnyTypeTwo();
        std::cout << "combat actor state queries tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
