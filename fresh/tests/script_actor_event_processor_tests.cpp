#include "pal4/script_actor_event_processor.h"

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

struct Runtime final : pal4::IScriptActorEventRuntime {
    std::map<std::uint32_t, std::int32_t> states{{1u, 2}, {2u, 1}};
    std::map<std::uint32_t, bool> allowed;
    std::map<std::uint32_t, bool> state_ok;
    std::vector<std::uint32_t> actors;
    std::vector<std::string> calls;
    std::size_t position = 0u;
    std::uint32_t random = 0u;
    bool blocked = false;
    bool throw_on_affect = false;

    std::int32_t ActorState(std::uint32_t actor) override {
        calls.push_back("state:" + std::to_string(actor));
        return states.at(actor);
    }
    bool SourceBlocksEvent(std::uint32_t source) override {
        Check(source == 1u, "source gate receives source actor");
        calls.push_back("blocked");
        return blocked;
    }
    std::uint32_t OpenTargetIterator(std::uint32_t target) override {
        Check(target == 2u, "iterator belongs to target");
        calls.push_back("open");
        return 0x7000u;
    }
    bool IteratorAtEnd(std::uint32_t iterator) override {
        Check(iterator == 0x7000u, "iterator end receives iterator");
        calls.push_back("end");
        return position == actors.size();
    }
    std::uint32_t IteratorCurrent(std::uint32_t iterator) override {
        Check(iterator == 0x7000u, "iterator current receives iterator");
        calls.push_back("current");
        return actors.at(position);
    }
    void IteratorNext(std::uint32_t iterator) override {
        Check(iterator == 0x7000u, "iterator advance receives iterator");
        calls.push_back("next");
        ++position;
    }
    bool CandidateStateNotThree(std::uint32_t candidate) override {
        calls.push_back("eligible:" + std::to_string(candidate));
        return state_ok[candidate];
    }
    bool CandidateCanAffect(std::uint32_t candidate,
                            std::uint32_t target) override {
        Check(target == 2u, "candidate receives target");
        calls.push_back("affect:" + std::to_string(candidate));
        if (throw_on_affect) throw std::runtime_error("candidate failure");
        return allowed[candidate];
    }
    std::uint32_t GenerateRandomNumber() override {
        calls.push_back("random");
        return random;
    }
    void DeleteIterator(std::uint32_t iterator,
                        std::uint32_t flags) override {
        Check(iterator == 0x7000u && flags == 1u,
              "iterator uses deleting destructor flag");
        calls.push_back("delete");
    }
};

void RejectedBeforeIteration() {
    Runtime runtime;
    runtime.states[1u] = 1;
    Check(pal4::ScriptActorEventProcessor::SelectTarget(1u, 2u, runtime) ==
              0u && runtime.calls.size() == 2u,
          "matching states return before source gate");
    runtime.calls.clear();
    runtime.states[1u] = 2;
    runtime.states[2u] = 3;
    Check(pal4::ScriptActorEventProcessor::SelectTarget(1u, 2u, runtime) ==
              0u && runtime.calls.size() == 2u,
          "target state must equal one");
    runtime.calls.clear();
    runtime.states[2u] = 1;
    runtime.blocked = true;
    Check(pal4::ScriptActorEventProcessor::SelectTarget(1u, 2u, runtime) ==
              0u && runtime.calls.back() == "blocked",
          "blocked source does not open an iterator");
}

void FilterAndSelect() {
    Runtime runtime;
    runtime.actors = {3u, 2u, 4u, 5u, 6u};
    runtime.state_ok = {{3u, false}, {2u, true}, {4u, true},
                        {5u, true}, {6u, true}};
    runtime.allowed = {{4u, false}, {5u, true}, {6u, true}};
    runtime.random = 3u;
    Check(pal4::ScriptActorEventProcessor::SelectTarget(1u, 2u, runtime) ==
              6u && runtime.position == runtime.actors.size(),
          "random remainder selects from filtered candidates");
    Check(runtime.calls.front() == "state:2" &&
          runtime.calls[1] == "state:1" &&
          runtime.calls[2] == "blocked" &&
          runtime.calls[3] == "open" &&
          runtime.calls.back() == "delete",
          "gate and iterator cleanup follow native order");
    for (const auto& call : runtime.calls)
        Check(call != "affect:3" && call != "affect:2",
              "state gate and self-exclusion short-circuit affect check");
}

void EmptyCandidates() {
    Runtime runtime;
    runtime.actors = {3u, 2u};
    runtime.state_ok = {{3u, false}, {2u, true}};
    Check(pal4::ScriptActorEventProcessor::SelectTarget(1u, 2u, runtime) ==
              0u && runtime.calls.back() == "delete",
          "empty candidate list returns zero and releases iterator");
    for (const auto& call : runtime.calls)
        Check(call != "random", "empty list does not request a random number");
}

void ExceptionReleasesIterator() {
    Runtime runtime;
    runtime.actors = {3u};
    runtime.state_ok[3u] = true;
    runtime.throw_on_affect = true;
    bool caught = false;
    try {
        (void)pal4::ScriptActorEventProcessor::SelectTarget(1u, 2u,
                                                              runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls.back() == "delete",
          "exception cleanup releases iterator");
}

}  // namespace

int main() {
    RejectedBeforeIteration();
    FilterAndSelect();
    EmptyCandidates();
    ExceptionReleasesIterator();
    std::cout << "Script actor event processor tests passed\n";
}
