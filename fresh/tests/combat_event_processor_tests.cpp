#include "pal4/combat_event_processor.h"

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

struct Runtime final : pal4::ICombatEventProcessRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> actors;
    std::size_t position = 0u;
    std::uint32_t actor_id = 42u;
    std::uint8_t owner_flag = 0u;
    std::uint8_t object_status = 0u;
    std::uint8_t actor_state = 1u;
    std::uint8_t actors_ready = 0u;
    std::uint8_t combat_active = 0u;
    std::uint8_t script_result = 0x5au;
    bool handled = true;
    bool condition = false;
    bool first_ready = false;
    bool fail_advance = false;

    std::uint32_t ActorFromFacet(std::uint32_t event) override {
        Check(event == 0x1000u, "event address");
        calls.emplace_back("actor");
        return 0x2000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == 0x5000u + 136u, "owner flag address");
        calls.emplace_back("flag");
        return owner_flag;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        Check(address == 0x3000u + 508u && value == 3u,
              "combat state write");
        calls.emplace_back("state:3");
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == 0x5000u + 136u && value == 1u,
              "ready flag write");
        calls.emplace_back("set-flag");
        owner_flag = value;
    }
    std::uint32_t CombatSystem() override {
        calls.emplace_back("system");
        return 0x3000u;
    }
    std::uint32_t ActorId(std::uint32_t actor) override {
        Check(actor == 0x2000u, "actor id source");
        calls.emplace_back("id");
        return actor_id;
    }
    bool HandleActorEvent(std::uint32_t actor) override {
        calls.emplace_back("handle:" + std::to_string(actor));
        return handled;
    }
    void FinalizeActor(std::uint32_t actor) override {
        Check(actor == 0x2000u, "finalize actor");
        calls.emplace_back("finalize");
    }
    void GameInstance() override { calls.emplace_back("game"); }
    void HandleActorProgress(std::uint32_t id) override {
        Check(id == actor_id, "progress actor id");
        calls.emplace_back("progress");
    }
    std::uint32_t CreateActorIterator(std::uint32_t actor) override {
        Check(actor == 0x2000u, "iterator source");
        calls.emplace_back("iterator");
        position = 0u;
        return 0x6000u;
    }
    bool IteratorAtEnd(std::uint32_t iterator) override {
        Check(iterator == 0x6000u, "iterator end identity");
        calls.emplace_back("end");
        return position >= actors.size();
    }
    std::uint32_t IteratorCurrent(std::uint32_t iterator) override {
        Check(iterator == 0x6000u && position < actors.size(),
              "iterator current identity");
        calls.emplace_back("current");
        return actors[position];
    }
    void IteratorAdvance(std::uint32_t iterator) override {
        Check(iterator == 0x6000u, "iterator advance identity");
        calls.emplace_back("advance");
        if (fail_advance) throw std::runtime_error("advance failed");
        ++position;
    }
    void DestroyIterator(std::uint32_t iterator) override {
        Check(iterator == 0x6000u, "iterator destruction identity");
        calls.emplace_back("destroy");
    }
    bool IsActorReady(std::uint32_t actor) override {
        Check(position < actors.size() && actor == actors[position],
              "actor ready identity");
        calls.emplace_back("actor-ready");
        return first_ready && position == 0u;
    }
    bool ActorCondition(std::uint32_t actor) override {
        Check(position < actors.size() && actor == actors[position],
              "actor condition identity");
        calls.emplace_back("condition");
        return condition;
    }
    std::uint8_t ObjectStatus(std::uint32_t actor) override {
        Check(actor == 0x2000u, "status actor");
        calls.emplace_back("status");
        return object_status;
    }
    std::uint8_t CheckActorState(std::uint32_t actor) override {
        Check(actor == 0x2000u, "state actor");
        calls.emplace_back("actor-state");
        return actor_state;
    }
    std::uint8_t CheckActorsReady() override {
        calls.emplace_back("all-ready");
        return actors_ready;
    }
    std::uint8_t IsCombatStateActive(std::uint32_t system) override {
        Check(system == 0x3000u, "active system");
        calls.emplace_back("active");
        return combat_active;
    }
    std::uint32_t ScriptEventRouter() override {
        calls.emplace_back("router");
        return 0x7000u;
    }
    std::uint8_t ProcessScriptEvent(std::uint32_t router,
                                    std::uint32_t descriptor,
                                    std::uint32_t payload,
                                    std::uint32_t mode) override {
        Check(router == 0x7000u && descriptor == 0x8b8b8cu && mode == 1u,
              "script event descriptor");
        calls.emplace_back("dispatch:" + std::to_string(payload));
        return script_result;
    }
};

void TargetActorEarlyDispatch() {
    Runtime runtime;
    runtime.words[0x2000u + 804u] = 2u;
    runtime.words[0x3000u + 444u] = 42u;
    Check(pal4::CombatEventProcessor::Process(
              0x5000u, 0x1000u, runtime) == 0x5au &&
              runtime.words[0x3000u + 508u] == 3u &&
              runtime.calls == std::vector<std::string>{
                  "actor", "read:8996", "system", "read:12732",
                  "system", "id", "read:12732", "system",
                  "state:3", "router", "dispatch:1"},
          "target actor changes combat state and dispatches immediately");
}

void OrdinaryActorBranches() {
    Runtime runtime;
    runtime.words[0x2000u + 804u] = 2u;
    runtime.words[0x3000u + 444u] = 7u;
    runtime.words[0x2000u + 964u] = 1u;
    runtime.handled = false;
    runtime.object_status = 9u;
    Check(pal4::CombatEventProcessor::Process(
              0x5000u, 0x1000u, runtime) == 9u &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "finalize") != runtime.calls.end() &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "progress") != runtime.calls.end() &&
              runtime.calls.back() == "status",
          "non-target actor finalizes, progresses and stops at object status");

    runtime.calls.clear();
    runtime.words[0x3000u + 444u] = 0u;
    runtime.words[0x2000u + 964u] = 0xffffffffu;
    runtime.handled = true;
    runtime.object_status = 0u;
    runtime.actor_state = 0u;
    Check(pal4::CombatEventProcessor::Process(
              0x5000u, 0x1000u, runtime) == 0u &&
              runtime.calls.back() == "actor-state" &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "progress") == runtime.calls.end(),
          "signed negative progress and failed actor state skip later work");

    runtime.calls.clear();
    runtime.actor_state = 1u;
    runtime.actors_ready = 1u;
    Check(pal4::CombatEventProcessor::Process(
              0x5000u, 0x1000u, runtime) == 1u &&
              runtime.owner_flag == 1u && runtime.calls.back() == "set-flag",
          "ready actors set owner flag and return before combat dispatch");

    runtime.calls.clear();
    runtime.combat_active = 1u;
    Check(pal4::CombatEventProcessor::Process(
              0x5000u, 0x1000u, runtime) == 0x5au &&
              runtime.calls.back() == "dispatch:1" &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "all-ready") == runtime.calls.end(),
          "existing owner flag skips readiness and dispatches type-two payload");
}

void CollectionBranches() {
    Runtime runtime;
    runtime.words[0x2000u + 804u] = 3u;
    runtime.actors = {0x4001u, 0x4002u};
    runtime.combat_active = 1u;
    Check(pal4::CombatEventProcessor::Process(
              0x5000u, 0x1000u, runtime) == 0x5au &&
              runtime.position == 2u &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "handle:16386") != runtime.calls.end() &&
              runtime.calls.back() == "dispatch:0",
          "collection remembers last matching actor and dispatches zero");
    const auto handle = std::find(runtime.calls.begin(), runtime.calls.end(),
                                  "handle:16386");
    const auto destroy = std::find(runtime.calls.begin(), runtime.calls.end(),
                                   "destroy");
    Check(handle < destroy, "pending actor handled before iterator release");

    runtime.calls.clear();
    runtime.first_ready = true;
    runtime.object_status = 4u;
    Check(pal4::CombatEventProcessor::Process(
              0x5000u, 0x1000u, runtime) == 4u &&
              runtime.position == 0u &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "advance") == runtime.calls.end() &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "destroy") != runtime.calls.end(),
          "ready actor stops scan and still releases iterator");

    runtime.calls.clear();
    runtime.first_ready = false;
    runtime.fail_advance = true;
    bool failed = false;
    try {
        (void)pal4::CombatEventProcessor::Process(
            0x5000u, 0x1000u, runtime);
    } catch (const std::runtime_error&) {
        failed = true;
    }
    Check(failed && runtime.calls.back() == "destroy",
          "iterator is destroyed when scanning throws");
}

}  // namespace

int main() {
    try {
        TargetActorEarlyDispatch();
        OrdinaryActorBranches();
        CollectionBranches();
        std::cout << "combat event processor tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
