#include "pal4/combat_action_lifecycle.h"

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

struct Runtime final : pal4::ICombatActionLifecycleRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> candidates;
    std::vector<std::string> calls;
    std::size_t position = 0;
    std::uint32_t child = 0x6000u;
    std::uint32_t task = 0x7000u;
    std::uint32_t iterator = 0x9000u;
    std::int32_t source_slot = 2;
    std::int32_t candidate_slot = 3;
    bool ready = true;
    bool ending = true;
    bool state_three = false;
    bool state_zero = true;
    bool throw_on_candidate = false;
    int child_reads = 0;
    int destroy_count = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("read-byte:" + std::to_string(address));
        return bytes[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.push_back("byte:" + std::to_string(address));
        bytes[address] = value;
    }
    std::uint32_t ActorFromFacet(std::uint32_t event) override {
        Check(event == 0x1000u, "facet event");
        calls.emplace_back("facet");
        return 0x3000u;
    }
    std::uint32_t ActorManager() override {
        calls.emplace_back("manager");
        return 0x4000u;
    }
    std::uint32_t CreateActorIterator(std::uint32_t collection) override {
        Check(collection == 0x4020u, "actor collection offset");
        calls.emplace_back("create-iterator");
        return iterator;
    }
    bool IteratorAtEnd(std::uint32_t value) override {
        Check(value == iterator, "iterator end handle");
        calls.emplace_back("end");
        return position >= candidates.size();
    }
    std::uint32_t IteratorCurrent(std::uint32_t value) override {
        Check(value == iterator, "iterator current handle");
        calls.emplace_back("current");
        if (throw_on_candidate) throw std::runtime_error("candidate failure");
        return candidates.at(position);
    }
    void IteratorAdvance(std::uint32_t value) override {
        Check(value == iterator, "iterator advance handle");
        calls.emplace_back("advance");
        ++position;
    }
    std::uint8_t DestroyIterator(std::uint32_t value) override {
        Check(value == iterator, "iterator destroy handle");
        calls.emplace_back("destroy");
        ++destroy_count;
        return 0xa5u;
    }
    bool CheckStateThree(std::uint32_t actor) override {
        Check(actor != 0x3000u, "source must be skipped");
        calls.emplace_back("state-three");
        return state_three;
    }
    bool IsStateZero(std::uint32_t actor) override {
        Check(actor != 0x3000u, "source must be skipped");
        calls.emplace_back("state-zero");
        return state_zero;
    }
    std::int32_t ActorSlot(std::uint32_t actor) override {
        calls.emplace_back("slot");
        return actor == 0x3000u ? source_slot : candidate_slot;
    }
    bool IsActorReady(std::uint32_t actor) override {
        Check(actor == 0x3000u, "ready actor");
        calls.emplace_back("ready");
        return ready;
    }
    void SetActorState(std::uint32_t actor, std::uint32_t state,
                       std::uint32_t mode) override {
        Check(actor == 0x3000u && state == 0u && mode == 1u,
              "source state transition");
        calls.emplace_back("set-state");
    }
    void FinalizeActor(std::uint32_t actor) override {
        Check(actor == 0x3000u, "finalized actor");
        calls.emplace_back("finalize");
    }
    void InitializeEffects(std::uint32_t actor, std::int32_t slot) override {
        Check(actor != 0x3000u && slot == source_slot,
              "candidate receives source slot");
        calls.emplace_back("effects");
    }
    std::uint32_t GetOwnedChild(std::uint32_t actor) override {
        Check(actor == 0x3000u, "owned-child actor");
        calls.emplace_back("child");
        ++child_reads;
        return child;
    }
    std::uint32_t CreateFollowupTask(std::uint32_t selected_child,
                                     std::uint32_t actor) override {
        Check(selected_child == child && actor == 0x3000u,
              "followup factory arguments");
        calls.emplace_back("followup");
        return task;
    }
    void RegisterTask(std::uint32_t queue, std::uint32_t value) override {
        Check(queue == 0x4060u && value == task, "queue registration");
        calls.emplace_back("register");
    }
    void ResetActionComponent(std::uint32_t component) override {
        Check(component == 0x3000u + 716u || component == 0x5000u + 716u,
              "action component address");
        calls.emplace_back("reset");
    }
    std::uint32_t ScriptEventRouter() override {
        calls.emplace_back("router");
        return 0x8000u;
    }
    void DispatchEvent(std::uint32_t router, std::uint32_t descriptor,
                       std::uint32_t event, std::uint32_t mode) override {
        Check(router == 0x8000u && event == 0x1000u && mode == 1u,
              "script event arguments");
        Check(descriptor == 0x8b8b78u || descriptor == 0x8b62d8u,
              "script event descriptor");
        calls.emplace_back(descriptor == 0x8b8b78u ? "stop" : "close");
    }
    std::uint32_t CombatSystem() override {
        calls.emplace_back("system");
        return 0xa000u;
    }
    bool IsCombatStateEnding(std::uint32_t system) override {
        Check(system == 0xa000u, "combat system argument");
        calls.emplace_back("ending");
        return ending;
    }
};

std::size_t Index(const Runtime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "missing expected call");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

void ResetAndState() {
    Runtime runtime;
    Check(pal4::CombatActionLifecycle::ResetEventData(0x2000u, runtime) == 0u,
          "reset return");
    Check(runtime.calls == std::vector<std::string>{"write:8256:0",
                                                      "write:8260:0"},
          "reset clears two words in order");
    Check(pal4::CombatActionLifecycle::SetStateToFive(runtime) == 0xa000u &&
              runtime.words[0xa000u + 508u] == 5u,
          "set state five returns combat system");
}

void ExtendedGatesAndSelection() {
    Runtime closed;
    Check(pal4::CombatActionLifecycle::ProcessExtended(
              0x2000u, 0x1000u, closed) == 0u &&
              closed.calls == std::vector<std::string>{"read-byte:8264"},
          "clear latch exits before actor access");

    Runtime empty;
    empty.bytes[0x2000u + 72u] = 1u;
    Check(pal4::CombatActionLifecycle::ProcessExtended(
              0x2000u, 0x1000u, empty) == 0xa5u &&
              empty.destroy_count == 1 &&
              Index(empty, "create-iterator") < Index(empty, "destroy"),
          "empty actor iterator is destroyed");

    Runtime selected;
    selected.bytes[0x2000u + 72u] = 1u;
    selected.words[0x2000u + 68u] = 0x5000u;
    selected.candidates = {0x3000u, 0x6000u};
    Check(pal4::CombatActionLifecycle::ProcessExtended(
              0x2000u, 0x1000u, selected) == 0xa5u &&
              selected.position == 1u && selected.destroy_count == 1 &&
              selected.bytes[0x2000u + 72u] == 0u &&
              selected.words[0xa000u + 508u] == 2u,
          "candidate transfer and ending state");
    Check(Index(selected, "set-state") < Index(selected, "finalize") &&
              Index(selected, "finalize") < Index(selected, "effects") &&
              Index(selected, "effects") < Index(selected, "reset") &&
              Index(selected, "reset") < Index(selected, "stop") &&
              Index(selected, "stop") < Index(selected, "close") &&
              Index(selected, "close") < Index(selected, "destroy"),
          "extended actor and event ordering");

    Runtime no_selected;
    no_selected.bytes[0x2000u + 72u] = 1u;
    no_selected.candidates = {0x6000u};
    no_selected.ready = false;
    (void)pal4::CombatActionLifecycle::ProcessExtended(
        0x2000u, 0x1000u, no_selected);
    Check(Index(no_selected, "effects") < Index(no_selected, "destroy") &&
              std::find(no_selected.calls.begin(), no_selected.calls.end(),
                        "stop") == no_selected.calls.end() &&
              no_selected.bytes[0x2000u + 72u] == 1u,
          "missing selected actor leaves selection active");

    Runtime throwing;
    throwing.bytes[0x2000u + 72u] = 1u;
    throwing.candidates = {0x6000u};
    throwing.throw_on_candidate = true;
    try {
        (void)pal4::CombatActionLifecycle::ProcessExtended(
            0x2000u, 0x1000u, throwing);
        Check(false, "expected iterator failure");
    } catch (const std::runtime_error&) {
        Check(throwing.destroy_count == 1, "exception releases iterator");
    }
}

void FinalGatesAndCompletion() {
    Runtime runtime;
    pal4::CombatActionLifecycle::ProcessFinal(0x2000u, 0x1000u, runtime);
    Check(runtime.calls == std::vector<std::string>{"read:8256"},
          "missing actor short circuits");

    runtime.calls.clear();
    runtime.words[0x2000u + 64u] = 0x3000u;
    pal4::CombatActionLifecycle::ProcessFinal(0x2000u, 0x1000u, runtime);
    Check(runtime.calls.size() == 2u, "clear latch short circuits");

    runtime.calls.clear();
    runtime.bytes[0x2000u + 72u] = 1u;
    runtime.words[0x3000u + 804u] = 2u;
    pal4::CombatActionLifecycle::ProcessFinal(0x2000u, 0x1000u, runtime);
    Check(runtime.child_reads == 0, "non-player actor has no task lookup");

    runtime.calls.clear();
    runtime.words[0x3000u + 804u] = 1u;
    runtime.child = 0u;
    pal4::CombatActionLifecycle::ProcessFinal(0x2000u, 0x1000u, runtime);
    Check(runtime.child_reads == 1 && runtime.bytes[0x2000u + 72u] == 1u,
          "missing child preserves selection");

    runtime.child = 0x6000u;
    runtime.task = 0u;
    pal4::CombatActionLifecycle::ProcessFinal(0x2000u, 0x1000u, runtime);
    Check(runtime.child_reads == 3 && runtime.bytes[0x2000u + 72u] == 1u,
          "null followup preserves selection after second child lookup");

    Runtime complete;
    complete.words[0x2000u + 64u] = 0x3000u;
    complete.words[0x2000u + 68u] = 0x5000u;
    complete.words[0x3000u + 804u] = 1u;
    complete.bytes[0x2000u + 72u] = 1u;
    pal4::CombatActionLifecycle::ProcessFinal(0x2000u, 0x1000u, complete);
    Check(complete.child_reads == 2 &&
              complete.words[0x2000u + 64u] == 0u &&
              complete.words[0x2000u + 68u] == 0u &&
              complete.bytes[0x2000u + 72u] == 0u &&
              complete.words[0xa000u + 508u] == 2u,
          "followup clears selection and restores ending state");
    Check(Index(complete, "followup") < Index(complete, "register") &&
              Index(complete, "register") < Index(complete, "reset") &&
              Index(complete, "reset") < Index(complete, "stop") &&
              Index(complete, "stop") < Index(complete, "close") &&
              Index(complete, "close") < Index(complete, "byte:8264") &&
              Index(complete, "byte:8264") < Index(complete, "ending"),
          "followup queue and cleanup ordering");
}

}  // namespace

int main() {
    try {
        ResetAndState();
        ExtendedGatesAndSelection();
        FinalGatesAndCompletion();
        std::cout << "combat action lifecycle tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
