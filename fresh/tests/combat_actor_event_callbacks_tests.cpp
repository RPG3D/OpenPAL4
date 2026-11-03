#include "pal4/combat_actor_event_callbacks.h"

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

struct Runtime final : pal4::ICombatActorEventRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> candidates;
    std::vector<std::uint32_t> zero_candidates;
    std::vector<std::uint32_t> ready_candidates;
    std::vector<std::uint32_t> not_three_candidates;
    std::size_t position = 0u;
    std::int32_t slot = 2;
    bool state_three = true;
    bool fail_initialize = false;
    bool fail_destroy = false;
    bool fail_extended_check = false;
    bool combat_active = false;
    std::uint8_t owner_flag = 0u;
    std::uint32_t extended_state = 3u;
    std::uint32_t visual_mode = 1u;
    std::uint32_t script_result = 0x5au;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        Check(address == 0x3000u + 508u && value == 6u,
              "combat state destination");
        calls.emplace_back("state:6");
        words[address] = value;
    }
    std::int32_t ActorSlot(std::uint32_t actor) override {
        Check(actor == 0x2000u, "actor slot source");
        calls.emplace_back("slot");
        return slot;
    }
    std::uint8_t FinalizeActor(std::uint32_t actor) override {
        Check(actor == 0x2000u, "actor finalize source");
        calls.emplace_back("finalize");
        return 0x7eu;
    }
    std::uint32_t CreateCandidateIterator(std::uint32_t actor) override {
        Check(actor == 0x2000u, "candidate source");
        calls.emplace_back("iterator");
        position = 0u;
        return 0x4000u;
    }
    bool IteratorAtEnd(std::uint32_t iterator) override {
        Check(iterator == 0x4000u, "iterator end identity");
        calls.emplace_back("end");
        return position >= candidates.size();
    }
    std::uint32_t IteratorCurrent(std::uint32_t iterator) override {
        Check(iterator == 0x4000u && position < candidates.size(),
              "iterator current identity");
        calls.emplace_back("current");
        return candidates[position];
    }
    void IteratorAdvance(std::uint32_t iterator) override {
        Check(iterator == 0x4000u, "iterator advance identity");
        calls.emplace_back("advance");
        ++position;
    }
    void DestroyIterator(std::uint32_t iterator) override {
        Check(iterator == 0x4000u, "iterator destroy identity");
        calls.emplace_back("destroy");
        if (fail_destroy) throw std::runtime_error("destruction failed");
    }
    bool IsStateZero(std::uint32_t actor) override {
        calls.emplace_back("zero:" + std::to_string(actor));
        return std::find(zero_candidates.begin(), zero_candidates.end(),
                         actor) != zero_candidates.end();
    }
    bool IsActorReady(std::uint32_t actor) override {
        calls.emplace_back("ready:" + std::to_string(actor));
        return std::find(ready_candidates.begin(), ready_candidates.end(),
                         actor) != ready_candidates.end();
    }
    void InitializeEffects(std::uint32_t actor,
                           std::int32_t selected_slot) override {
        Check(actor == 0x5000u && selected_slot == slot,
              "effect destination and slot");
        calls.emplace_back("initialize");
        if (fail_initialize) throw std::runtime_error("initialization failed");
    }
    std::uint32_t ActorFromFacet(std::uint32_t event) override {
        Check(event == 0x1000u, "facet event address");
        calls.emplace_back("facet");
        return 0x2000u;
    }
    std::uint32_t ActorRotation(std::uint32_t actor,
                                std::uint32_t mode) override {
        Check(actor == 0x2000u && mode == 0u,
              "rotation actor and mode");
        calls.emplace_back("rotation");
        return 0x1234u;
    }
    void ProcessChildSlot4(std::uint32_t actor,
                           std::uint32_t rotation,
                           std::uint32_t mode) override {
        Check(actor == 0x2000u && rotation == 0x1234u && mode == 0u,
              "child slot arguments");
        calls.emplace_back("child");
    }
    std::uint32_t ProcessRotationResult(std::uint32_t actor,
                                        std::uint32_t mode) override {
        Check(actor == 0x2000u && mode == 1u,
              "final rotation processing arguments");
        calls.emplace_back("result");
        return 0x77u;
    }
    bool CheckStateThree(std::uint32_t actor) override {
        Check(actor == 0x2000u, "state check actor");
        calls.emplace_back("state-three");
        return state_three;
    }
    std::uint32_t CombatSystem() override {
        calls.emplace_back("system");
        return 0x3000u;
    }
    std::uint32_t ActorId(std::uint32_t actor) override {
        Check(actor == 0x2000u, "actor id source");
        calls.emplace_back("id");
        return 42u;
    }
    std::uint32_t CreateExtendedIterator(std::uint32_t actor) override {
        Check(actor == 0x2000u, "extended iterator source");
        calls.emplace_back("extended-iterator");
        position = 0u;
        return 0x4000u;
    }
    bool ObjectStateNotThree(std::uint32_t actor) override {
        calls.emplace_back("not-three:" + std::to_string(actor));
        if (fail_extended_check)
            throw std::runtime_error("extended state check failed");
        return std::find(not_three_candidates.begin(),
                         not_three_candidates.end(), actor) !=
               not_three_candidates.end();
    }
    std::uint32_t ActorVisualMode(std::uint32_t actor) override {
        Check(actor == 0x2000u, "visual mode actor");
        calls.emplace_back("visual-mode");
        return visual_mode;
    }
    void SetVisualScalar(std::uint32_t visual,
                         std::uint32_t value_bits) override {
        Check(visual == 0x2000u + 716u &&
                  value_bits == 0x42000000u,
              "visual field and float bits");
        calls.emplace_back("visual:32");
    }
    std::uint32_t CheckActorStateExtended(
        std::uint32_t owner) override {
        Check(owner == 0x6000u, "extended owner");
        calls.emplace_back("extended-state");
        return extended_state;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == 0x6000u + 136u, "owner flag address");
        calls.emplace_back("flag");
        return owner_flag;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        Check(address == 0x6000u + 136u && value == 1u,
              "owner flag write");
        calls.emplace_back("set-flag");
        owner_flag = value;
    }
    bool IsCombatStateActive(std::uint32_t system) override {
        Check(system == 0x3000u, "active system");
        calls.emplace_back("active");
        return combat_active;
    }
    void SetActorState(std::uint32_t actor, std::uint32_t state,
                       std::uint32_t mode) override {
        Check(actor == 0x2000u && state == 6u && mode == 1u,
              "actor state assignment");
        calls.emplace_back("actor-state:6");
    }
    std::uint32_t ScriptEventRouter() override {
        calls.emplace_back("router");
        return 0x7000u;
    }
    std::uint32_t ProcessScriptEvent(std::uint32_t router,
                                     std::uint32_t descriptor,
                                     std::uint32_t payload,
                                     std::uint32_t mode) override {
        Check(router == 0x7000u && descriptor == 0x8b8b8cu && mode == 1u,
              "combat-over event descriptor");
        calls.emplace_back("dispatch:" + std::to_string(payload));
        return script_result;
    }
};

void ActorTransfer() {
    Runtime runtime;
    runtime.slot = 6;
    Check(!pal4::CombatActorEventCallbacks::HandleActorEvent(
              0x2000u, runtime) &&
              runtime.calls == std::vector<std::string>{"slot", "finalize"},
          "slot six finalizes without creating iterator");

    runtime.calls.clear();
    runtime.slot = 2;
    Check(!pal4::CombatActorEventCallbacks::HandleActorEvent(
              0x2000u, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "slot", "iterator", "end", "destroy"},
          "empty candidate range releases iterator");

    runtime.calls.clear();
    runtime.candidates = {0x2000u, 0x4001u, 0x5000u};
    runtime.zero_candidates = {0x2000u, 0x5000u};
    Check(pal4::CombatActorEventCallbacks::HandleActorEvent(
              0x2000u, runtime) && runtime.position == 2u &&
              runtime.calls == std::vector<std::string>{
                  "slot", "iterator", "end", "current", "zero:8192",
                  "advance", "end", "current", "zero:16385",
                  "advance", "end", "current", "zero:20480",
                  "slot", "finalize", "initialize", "destroy"},
          "first state-zero non-self candidate receives current slot");

    runtime.calls.clear();
    runtime.fail_initialize = true;
    bool failed = false;
    try {
        (void)pal4::CombatActorEventCallbacks::HandleActorEvent(
            0x2000u, runtime);
    } catch (const std::runtime_error&) {
        failed = true;
    }
    Check(failed && runtime.calls.back() == "destroy" &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "destroy") == 1,
          "candidate iterator is released when initialization throws");

    runtime.calls.clear();
    runtime.fail_initialize = false;
    runtime.fail_destroy = true;
    failed = false;
    try {
        (void)pal4::CombatActorEventCallbacks::HandleActorEvent(
            0x2000u, runtime);
    } catch (const std::runtime_error&) {
        failed = true;
    }
    Check(failed && std::count(runtime.calls.begin(), runtime.calls.end(),
                               "destroy") == 1,
          "iterator destruction failure does not destroy it twice");
}

void Rotation() {
    Runtime runtime;
    Check(pal4::CombatActorEventCallbacks::GetRotationAndProcess(
              0x1000u, runtime) == 0x77u &&
              runtime.calls == std::vector<std::string>{
                  "facet", "rotation", "facet", "child", "facet", "result"},
          "rotation callback reacquires facet actor three times");
}

void StateChange() {
    Runtime runtime;
    runtime.words[0x1000u + 4u] = 3u;
    pal4::CombatActorEventCallbacks::HandleStateChange(0x1000u, runtime);
    Check(runtime.calls == std::vector<std::string>{"read:4100"},
          "unrelated event does not request actor");

    runtime.calls.clear();
    runtime.words[0x1000u + 4u] = 4u;
    runtime.words[0x2000u + 804u] = 1u;
    pal4::CombatActorEventCallbacks::HandleStateChange(0x1000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "read:4100", "facet", "read:8996"},
          "non-type-two actor stops before state check");

    runtime.calls.clear();
    runtime.words[0x2000u + 804u] = 2u;
    runtime.state_three = false;
    pal4::CombatActorEventCallbacks::HandleStateChange(0x1000u, runtime);
    Check(runtime.calls.back() == "state-three",
          "actor state gate precedes combat system access");

    runtime.calls.clear();
    runtime.state_three = true;
    runtime.words[0x3000u + 444u] = 0u;
    pal4::CombatActorEventCallbacks::HandleStateChange(0x1000u, runtime);
    Check(std::count(runtime.calls.begin(), runtime.calls.end(),
                     "system") == 1 &&
              runtime.calls.back() == "read:12732",
          "empty target stops after first combat singleton read");

    runtime.calls.clear();
    runtime.words[0x3000u + 444u] = 7u;
    pal4::CombatActorEventCallbacks::HandleStateChange(0x1000u, runtime);
    Check(std::count(runtime.calls.begin(), runtime.calls.end(),
                     "system") == 2 &&
              runtime.calls.back() == "read:12732",
          "mismatched actor id leaves combat state unchanged");

    runtime.calls.clear();
    runtime.words[0x3000u + 444u] = 42u;
    pal4::CombatActorEventCallbacks::HandleStateChange(0x1000u, runtime);
    Check(runtime.words[0x3000u + 508u] == 6u &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "system") == 3 &&
              runtime.calls.back() == "state:6",
          "matching target reacquires singleton and sets state six");
}

void ExtendedEvent() {
    Runtime runtime;
    Check(pal4::CombatActorEventCallbacks::HandleExtended(
              0x6000u, 0x1000u, runtime) == 3u &&
              runtime.calls == std::vector<std::string>{
                  "facet", "system", "active", "extended-state"},
          "inactive combat returns state three without iterator or flag");

    runtime.calls.clear();
    runtime.extended_state = 0u;
    Check(pal4::CombatActorEventCallbacks::HandleExtended(
              0x6000u, 0x1000u, runtime) == 0u &&
              runtime.owner_flag == 1u &&
              runtime.calls.back() == "set-flag",
          "first non-three state sets owner flag");

    runtime.calls.clear();
    runtime.extended_state = 4u;
    Check(pal4::CombatActorEventCallbacks::HandleExtended(
              0x6000u, 0x1000u, runtime) == 0x5au &&
              runtime.calls.back() == "dispatch:4",
          "later non-three state dispatches its exact value");

    runtime.calls.clear();
    runtime.combat_active = true;
    runtime.extended_state = 3u;
    runtime.candidates = {0x4001u, 0x5000u};
    runtime.not_three_candidates = {0x5000u};
    Check(pal4::CombatActorEventCallbacks::HandleExtended(
              0x6000u, 0x1000u, runtime) == 3u &&
              runtime.calls == std::vector<std::string>{
                  "facet", "system", "active", "extended-iterator",
                  "end", "current", "not-three:16385", "advance",
                  "end", "current", "not-three:20480", "visual-mode",
                  "visual:32", "destroy", "extended-state"},
          "active scan updates visual for first non-three candidate");

    runtime.calls.clear();
    runtime.visual_mode = 0u;
    Check(pal4::CombatActorEventCallbacks::HandleExtended(
              0x6000u, 0x1000u, runtime) == 3u &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "visual:32") == runtime.calls.end() &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "destroy") == 1,
          "visual mode other than one only releases iterator");

    runtime.calls.clear();
    runtime.fail_extended_check = true;
    bool failed = false;
    try {
        (void)pal4::CombatActorEventCallbacks::HandleExtended(
            0x6000u, 0x1000u, runtime);
    } catch (const std::runtime_error&) {
        failed = true;
    }
    Check(failed && runtime.calls.back() == "destroy",
          "extended iterator is released on state-check failure");
}

void TypedEvent() {
    Runtime runtime;
    runtime.words[0x2000u + 804u] = 1u;
    Check(pal4::CombatActorEventCallbacks::HandleType(
              0x6000u, 0x1000u, runtime) == 0x5au &&
              runtime.calls == std::vector<std::string>{
                  "facet", "read:8996", "router", "dispatch:2"},
          "type one dispatches payload two without actor-state check");

    runtime.calls.clear();
    runtime.words[0x2000u + 804u] = 2u;
    Check(pal4::CombatActorEventCallbacks::HandleType(
              0x6000u, 0x1000u, runtime) == 0x5au &&
              runtime.calls == std::vector<std::string>{
                  "facet", "read:8996", "iterator", "end", "destroy",
                  "actor-state:6", "finalize", "router", "dispatch:1"},
          "eligible actor enters state six and dispatches payload one");

    runtime.calls.clear();
    runtime.candidates = {0x5000u};
    runtime.zero_candidates = {0x5000u};
    Check(pal4::CombatActorEventCallbacks::HandleType(
              0x6000u, 0x1000u, runtime) == 1u &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "finalize") == 1 &&
              runtime.calls.back() == "destroy",
          "failed state check reuses transfer handler and returns one");

    runtime.calls.clear();
    runtime.slot = 6;
    Check(pal4::CombatActorEventCallbacks::HandleType(
              0x6000u, 0x1000u, runtime) == 0x7eu &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "finalize") == 2,
          "failed transfer returns low byte of second finalize call");
}

void ActorStateQuery() {
    Runtime runtime;
    Check(pal4::CombatActorEventCallbacks::CheckActorState(
              0x2000u, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "iterator", "end", "destroy"},
          "empty candidate range reports actor state ready");

    runtime.calls.clear();
    runtime.candidates = {0x2000u, 0x5000u};
    runtime.ready_candidates = {0x2000u};
    Check(pal4::CombatActorEventCallbacks::CheckActorState(
              0x2000u, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "iterator", "end", "current", "advance", "end",
                  "current", "ready:20480", "zero:20480", "advance",
                  "end", "destroy"},
          "self candidate skips readiness checks and unrelated actor advances");

    runtime.calls.clear();
    runtime.ready_candidates = {0x5000u};
    Check(!pal4::CombatActorEventCallbacks::CheckActorState(
              0x2000u, runtime) &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "zero:20480") == runtime.calls.end() &&
              runtime.calls.back() == "destroy",
          "ready candidate short-circuits state-zero check");

    runtime.calls.clear();
    runtime.ready_candidates.clear();
    runtime.zero_candidates = {0x5000u};
    Check(!pal4::CombatActorEventCallbacks::CheckActorState(
              0x2000u, runtime) &&
              runtime.calls.back() == "destroy",
          "state-zero candidate also rejects actor state");
}

}  // namespace

int main() {
    try {
        ActorTransfer();
        Rotation();
        StateChange();
        ExtendedEvent();
        TypedEvent();
        ActorStateQuery();
        std::cout << "combat actor event callbacks tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
