#include "pal4/actor_manager_player_monster_gate.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IActorManagerPlayerMonsterGateRuntime {
    std::vector<std::string> trace;
    std::vector<bool> qualifies;
    std::size_t position = 0u;
    std::uint32_t iterator = 0x3000u;
    bool combat_active = false;
    bool throw_at_qualify = false;
    std::int32_t result532 = 11;
    std::int32_t result536 = 22;
    std::int32_t destroy_result = 33;

    std::uint32_t SingletonManager() override {
        trace.emplace_back("singleton");
        return 0x2000u;
    }
    std::uint32_t FirstIterator(std::uint32_t manager) override {
        Check(manager == 0x2000u, "first monster iterator manager");
        trace.emplace_back("first");
        return iterator;
    }
    bool IteratorDone(std::uint32_t actual_iterator) override {
        Check(actual_iterator == iterator, "iterator done receiver");
        trace.emplace_back("done");
        return position >= qualifies.size();
    }
    std::uint32_t IteratorValue(std::uint32_t actual_iterator) override {
        Check(actual_iterator == iterator && position < qualifies.size(),
              "iterator value receiver");
        trace.emplace_back("value");
        return 0x4000u + static_cast<std::uint32_t>(position);
    }
    void IteratorAdvance(std::uint32_t actual_iterator) override {
        Check(actual_iterator == iterator, "iterator advance receiver");
        trace.emplace_back("advance");
        ++position;
    }
    bool MonsterSlot412(std::uint32_t monster) override {
        Check(monster == 0x4000u + position,
              "monster virtual slot 412 receiver");
        trace.emplace_back("qualify");
        if (throw_at_qualify) throw std::runtime_error("qualify failed");
        return qualifies[position];
    }
    bool CombatActive() override {
        trace.emplace_back("combat");
        return combat_active;
    }
    std::int32_t ActorSlot532(std::uint32_t actor) override {
        Check(actor == 0x1000u, "player actor slot 532 receiver");
        trace.emplace_back("slot532");
        return result532;
    }
    std::int32_t ActorSlot536(std::uint32_t actor) override {
        Check(actor == 0x1000u, "player actor slot 536 receiver");
        trace.emplace_back("slot536");
        return result536;
    }
    std::int32_t DestroyIterator(std::uint32_t actual_iterator,
                                 std::uint32_t flags) override {
        Check(actual_iterator == iterator && flags == 1u,
              "iterator deleting destructor flag");
        trace.emplace_back("destroy");
        return destroy_result;
    }
};

}  // namespace

int main() {
    try {
        Runtime empty;
        Check(pal4::ActorManagerPlayerMonsterGate::SelectAction(
                  0x1000u, empty) == empty.destroy_result &&
                  empty.trace == std::vector<std::string>{
                      "singleton", "first", "done", "combat",
                      "slot532", "destroy"},
              "empty inactive collection selects slot 532 then returns destructor");

        Runtime active;
        active.combat_active = true;
        Check(pal4::ActorManagerPlayerMonsterGate::SelectAction(
                  0x1000u, active) == active.destroy_result &&
                  active.trace == std::vector<std::string>{
                      "singleton", "first", "done", "combat",
                      "slot536", "destroy"},
              "empty active collection selects slot 536");

        Runtime found;
        found.qualifies = {false, false, true};
        Check(pal4::ActorManagerPlayerMonsterGate::SelectAction(
                  0x1000u, found) == found.destroy_result &&
                  found.position == 2u &&
                  std::find(found.trace.begin(), found.trace.end(),
                            "combat") == found.trace.end() &&
                  found.trace[found.trace.size() - 2u] == "slot536" &&
                  found.trace.back() == "destroy",
              "qualifying monster selects slot 536 without combat query");

        Runtime exhausted;
        exhausted.qualifies = {false, false};
        Check(pal4::ActorManagerPlayerMonsterGate::SelectAction(
                  0x1000u, exhausted) == exhausted.destroy_result &&
                  exhausted.position == 2u &&
                  exhausted.trace[exhausted.trace.size() - 3u] == "combat" &&
                  exhausted.trace[exhausted.trace.size() - 2u] == "slot532",
              "exhausted collection queries combat after two advances");

        Runtime null_iterator;
        null_iterator.iterator = 0u;
        Check(pal4::ActorManagerPlayerMonsterGate::SelectAction(
                  0x1000u, null_iterator) == null_iterator.result532 &&
                  std::find(null_iterator.trace.begin(),
                            null_iterator.trace.end(), "destroy") ==
                      null_iterator.trace.end(),
              "null iterator leaves selected actor return intact");

        Runtime throwing;
        throwing.qualifies = {false};
        throwing.throw_at_qualify = true;
        try {
            (void)pal4::ActorManagerPlayerMonsterGate::SelectAction(
                0x1000u, throwing);
            throw std::runtime_error("selection should throw");
        } catch (const std::runtime_error& error) {
            Check(std::string(error.what()) == "qualify failed" &&
                      throwing.trace.back() == "destroy",
                  "exception unwinding destroys the live iterator");
        }
        std::cout << "actor_manager_player_monster_gate_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
