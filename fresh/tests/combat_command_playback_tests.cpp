#include "pal4/combat_command_playback.h"

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

struct Runtime final : pal4::ICombatCommandPlaybackRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t found = 0x6000u;
    std::uint32_t mode = 0u;
    std::uint32_t candidate = 0x7000u;
    std::int32_t count = 1;
    bool throw_on_current = false;
    std::uint32_t attack_mode = 1u;
    int destroyed = 0;
    int actor_reads = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
        words[address] = value;
    }
    std::uint32_t ScriptEventRouter() override {
        calls.emplace_back("router");
        return 0x8000u;
    }
    void DispatchEvent(std::uint32_t router, std::uint32_t descriptor,
                       std::uint32_t event, std::uint32_t mode_arg,
                       bool pointer_payload) override {
        Check(router == 0x8000u && event == 0x1000u && mode_arg == 1u,
              "event routing arguments");
        if (descriptor == 0x8b62d8u) {
            Check(pointer_payload, "close UI uses pointer payload");
            calls.emplace_back("close-pointer");
        } else {
            Check(!pointer_payload, "target routing uses event record");
            calls.emplace_back(descriptor == 0x8b62ccu ? "select" : "choose");
            Check(descriptor == 0x8b62ccu || descriptor == 0x8b8b74u,
                  "target descriptor");
        }
    }
    void DispatchLocalByte(std::uint32_t router,
                           std::uint32_t descriptor,
                           std::uint32_t mode_arg) override {
        Check(router == 0x8000u && descriptor == 0x8b62d8u &&
                  mode_arg == 1u, "flee close UI local-byte payload");
        calls.emplace_back("close-local-byte");
    }
    std::uint32_t GameDatabase() override {
        calls.emplace_back("database");
        return 0x5000u;
    }
    std::uint32_t FindMagic(std::uint32_t table,
                            std::uint32_t id) override {
        Check(table == 0x500cu && id == 0x99u, "magic lookup");
        calls.emplace_back("magic");
        return found;
    }
    std::uint32_t FindStunt(std::uint32_t table,
                            std::uint32_t id) override {
        Check(table == 0x500cu && id == 0x99u, "stunt lookup");
        calls.emplace_back("stunt");
        return found;
    }
    std::uint32_t FindProperty(std::uint32_t database,
                               std::uint32_t id) override {
        Check(database == 0x5000u && id == 0x99u, "property lookup");
        calls.emplace_back("property");
        return found;
    }
    std::uint32_t MagicTargetMode(std::uint32_t magic) override {
        Check(magic == found, "magic mode resource");
        calls.emplace_back("magic-mode");
        return mode;
    }
    std::uint32_t StuntTargetMode(std::uint32_t stunt) override {
        Check(stunt == found, "stunt mode resource");
        calls.emplace_back("stunt-mode");
        return mode;
    }
    std::uint32_t PropertyTargetMode(std::uint32_t property) override {
        Check(property == found, "property mode resource");
        calls.emplace_back("property-mode");
        return mode;
    }
    std::uint32_t ActorFromFacet(std::uint32_t event) override {
        Check(event == 0x1000u, "actor facet argument");
        calls.emplace_back("facet");
        ++actor_reads;
        return 0x3000u;
    }
    std::uint32_t ActorAttackMode(std::uint32_t actor) override {
        Check(actor == 0x3000u, "flee mode actor");
        calls.emplace_back("flee-mode");
        return attack_mode;
    }
    std::uint32_t SelectCandidates(std::uint32_t actor,
                                    std::uint32_t event) override {
        Check(actor == 0x3000u && event == 0x1000u,
              "candidate selection arguments");
        calls.emplace_back("candidates");
        return 0x9000u;
    }
    std::int32_t CandidateCount(std::uint32_t candidates) override {
        Check(candidates == 0x9000u, "candidate count handle");
        calls.emplace_back("count");
        return count;
    }
    std::uint32_t CurrentCandidate(std::uint32_t candidates) override {
        Check(candidates == 0x9000u, "current candidate handle");
        calls.emplace_back("current");
        if (throw_on_current) throw std::runtime_error("current failed");
        return candidate;
    }
    void DestroyCandidates(std::uint32_t candidates) override {
        Check(candidates == 0x9000u, "candidate destroy handle");
        calls.emplace_back("destroy");
        ++destroyed;
    }
    void ReportInvalidTargetMode(std::uint32_t value) override {
        Check(value == mode, "invalid mode argument");
        calls.emplace_back("invalid");
    }
};

std::size_t Index(const Runtime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected call absent");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

void Prepare(Runtime& runtime, std::uint32_t action) {
    runtime.words[0x1000u + 8u] = 0x99u;
    runtime.words[0x1000u + 12u] = action;
}

void EntryPoints() {
    using pal4::CombatCommandPlayback;
    for (std::uint32_t action = 0u; action < 3u; ++action) {
        Runtime runtime;
        Prepare(runtime, 9u);
        runtime.mode = 1u;
        if (action == 0u) CombatCommandPlayback::PlayMagic(0x1000u, runtime);
        if (action == 1u) CombatCommandPlayback::PlayStunt(0x1000u, runtime);
        if (action == 2u) CombatCommandPlayback::UseProperty(0x1000u, runtime);
        Check(runtime.words[0x1000u + 12u] == action &&
                  Index(runtime, "write:4108:" + std::to_string(action)) <
                      Index(runtime, "close-pointer") &&
                  Index(runtime, "close-pointer") < Index(runtime, "database") &&
                  Index(runtime, "database") < Index(runtime, "choose"),
              "action update, close, lookup and target order");
        Check(runtime.actor_reads == 1, "postprocess reads actor once");

        Runtime missing;
        Prepare(missing, action);
        missing.found = 0u;
        if (action == 0u) CombatCommandPlayback::PlayMagic(0x1000u, missing);
        if (action == 1u) CombatCommandPlayback::PlayStunt(0x1000u, missing);
        if (action == 2u) CombatCommandPlayback::UseProperty(0x1000u, missing);
        Check(missing.actor_reads == 0 && missing.calls.back() ==
                  (action == 0u ? "magic" : action == 1u ? "stunt" : "property"),
              "missing resource exits after lookup");
    }

    Runtime unchanged;
    Prepare(unchanged, 0u);
    CombatCommandPlayback::PlayMagic(0x1000u, unchanged);
    Check(std::none_of(unchanged.calls.begin(), unchanged.calls.end(),
                       [](const auto& call) {
                           return call.rfind("write:4108", 0) == 0;
                       }),
          "existing action type is not rewritten");
}

void TargetModes() {
    using pal4::CombatCommandPlayback;
    for (const auto mode : {0u, 3u}) {
        Runtime runtime;
        CombatCommandPlayback::PostProcess(0x1000u, mode, runtime);
        Check(runtime.words[0x1000u + 4u] == 0x3000u &&
                  Index(runtime, "facet") < Index(runtime, "write:4100:12288") &&
                  Index(runtime, "write:4100:12288") < Index(runtime, "select"),
              "self-target mode writes actor before select event");
    }
    for (const auto mode : {1u, 2u}) {
        Runtime runtime;
        CombatCommandPlayback::PostProcess(0x1000u, mode, runtime);
        Check(runtime.actor_reads == 1 &&
                  runtime.words.find(0x1000u + 4u) == runtime.words.end() &&
                  runtime.calls.back() == "choose",
              "manual target mode emits choice without target write");
    }
    for (const auto count : {0, 1}) {
        Runtime runtime;
        runtime.count = count;
        CombatCommandPlayback::PostProcess(0x1000u, 4u, runtime);
        Check(runtime.actor_reads == 2 && runtime.destroyed == 1 &&
                  (count == 0 ? runtime.words.find(0x1000u + 4u) == runtime.words.end()
                              : runtime.words[0x1000u + 4u] == 0x7000u),
              "candidate mode count gate and cleanup");
        if (count != 0)
            Check(Index(runtime, "current") < Index(runtime, "select") &&
                      Index(runtime, "select") < Index(runtime, "destroy"),
                  "candidate selection event precedes cleanup");
    }
    Runtime null_candidate;
    null_candidate.candidate = 0u;
    CombatCommandPlayback::PostProcess(0x1000u, 4u, null_candidate);
    Check(null_candidate.destroyed == 1 &&
              std::find(null_candidate.calls.begin(), null_candidate.calls.end(),
                        "select") == null_candidate.calls.end(),
          "null candidate does not dispatch select event");

    Runtime throwing;
    throwing.throw_on_current = true;
    try {
        CombatCommandPlayback::PostProcess(0x1000u, 4u, throwing);
        Check(false, "candidate exception expected");
    } catch (const std::runtime_error&) {
        Check(throwing.destroyed == 1, "exception releases candidates");
    }

    Runtime invalid;
    invalid.mode = 9u;
    CombatCommandPlayback::PostProcess(0x1000u, 9u, invalid);
    Check(invalid.actor_reads == 1 && invalid.calls.back() == "invalid",
          "invalid target mode reaches native report boundary");
}

void FleeCommand() {
    Runtime candidate_mode;
    pal4::CombatCommandPlayback::SelectAttack(0x1000u, candidate_mode);
    Check(candidate_mode.actor_reads == 3 &&
              candidate_mode.destroyed == 1 &&
              candidate_mode.words[0x1000u + 4u] == 0x7000u &&
              Index(candidate_mode, "close-local-byte") <
                  Index(candidate_mode, "flee-mode") &&
              Index(candidate_mode, "flee-mode") <
                  Index(candidate_mode, "candidates") &&
              Index(candidate_mode, "candidates") <
                  Index(candidate_mode, "select"),
          "flee candidate branch follows local-byte close event");

    Runtime manual_mode;
    manual_mode.attack_mode = 2u;
    pal4::CombatCommandPlayback::SelectAttack(0x1000u, manual_mode);
    Check(manual_mode.actor_reads == 1 &&
              manual_mode.destroyed == 0 &&
              manual_mode.calls.back() == "choose" &&
              Index(manual_mode, "close-local-byte") <
                  Index(manual_mode, "choose"),
          "other flee mode requests target choice");
}

}  // namespace

int main() {
    try {
        EntryPoints();
        TargetModes();
        FleeCommand();
        std::cout << "combat command playback tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
