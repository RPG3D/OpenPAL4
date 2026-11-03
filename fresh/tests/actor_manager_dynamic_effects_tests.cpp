#include "pal4/actor_manager_dynamic_effects.h"

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

constexpr std::uint32_t actor = 0x1000u;
constexpr std::uint32_t record = 0x2000u;
constexpr std::uint32_t tree = actor + 836u;
constexpr std::uint32_t head = 0x4000u;
constexpr std::uint32_t root = 0x5000u;
constexpr std::uint32_t nil = 0x6000u;

struct Runtime final : pal4::IActorManagerDynamicEffectsRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> trace;
    std::uint32_t lower_bound = head;
    std::uint32_t first_node = head;
    std::uint32_t predecessor = head;
    bool valid = true;
    bool inserted = false;
    std::uint32_t result_node = 0u;

    Runtime() {
        words[record] = 7u;
        words[actor + 840u] = head;
        words[head + 4u] = nil;
        bytes[actor + 844u] = 0u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    std::uint32_t NilNode() override {
        return nil;
    }
    std::uint32_t LowerBound(std::uint32_t address,
                             std::int32_t key) override {
        Check(address == tree && key == static_cast<std::int32_t>(words[record]),
              "lower bound arguments");
        trace.emplace_back("lower");
        return lower_bound;
    }
    void NotifyEffectEnd(std::uint32_t address,
                         std::uint32_t context) override {
        Check(address == lower_bound + 16u && context == 0x3000u,
              "old effect notification arguments");
        trace.emplace_back("notify");
    }
    void EraseNode(std::uint32_t address, std::uint32_t node) override {
        Check(address == tree && node == lower_bound, "erase arguments");
        trace.emplace_back("erase");
    }
    pal4::ActorEffectHandle CreateEffect(
        std::uint32_t subject, std::uint32_t first,
        std::uint32_t second) override {
        Check(subject == actor && first == record + 68u &&
              second == record + 84u, "actor virtual slot 384 arguments");
        trace.emplace_back("create");
        return {11u, 22u};
    }
    bool IsValid(pal4::ActorEffectHandle effect) override {
        Check(effect.id == 11u, "validity handle");
        trace.emplace_back("valid");
        return valid;
    }
    pal4::ActorEffectHandle CopyEffect(
        pal4::ActorEffectHandle effect) override {
        Check(effect.id == 11u && effect.owner == 22u, "copy handle");
        trace.emplace_back("copy");
        return {11u, 23u};
    }
    std::uint32_t FirstNode(std::uint32_t address) override {
        Check(address == tree, "first node tree");
        trace.emplace_back("first");
        return first_node;
    }
    std::uint32_t Predecessor(std::uint32_t node) override {
        Check(node == 0x5200u, "predecessor node");
        trace.emplace_back("predecessor");
        return predecessor;
    }
    std::uint32_t InsertNode(
        std::uint32_t address, std::uint32_t cursor,
        std::uint32_t parent, std::int32_t key,
        pal4::ActorEffectHandle handle) override {
        Check(address == tree && cursor == nil &&
              key == static_cast<std::int32_t>(words[record]) &&
              handle.id == 11u && handle.owner == 23u,
              "tree insertion arguments");
        Check(parent == head || parent == root || parent == 0x5200u,
              "tree insertion parent");
        trace.emplace_back("insert");
        return 0x7000u;
    }
    void RecordResult(std::uint32_t node, bool was_inserted) override {
        result_node = node;
        inserted = was_inserted;
        trace.emplace_back(was_inserted ? "result-insert" : "result-existing");
    }
    void ResetTemporary(pal4::ActorEffectHandle effect) override {
        Check(effect.id == 11u && (effect.owner == 22u ||
                                    effect.owner == 23u),
              "temporary handle reset");
        trace.emplace_back("reset");
    }
};

void NullAndRemoval() {
    Runtime runtime;
    pal4::ActorManagerDynamicEffects::Add(actor, 0x3000u, 0u, runtime);
    pal4::ActorManagerDynamicEffects::RemoveMatching(
        actor, 0x3000u, 0u, runtime);
    Check(runtime.trace.empty(), "null record bypasses both operations");

    pal4::ActorManagerDynamicEffects::RemoveMatching(
        actor, 0x3000u, record, runtime);
    Check(runtime.trace == std::vector<std::string>{"lower"},
          "missing key does not notify or erase");

    runtime.lower_bound = root;
    runtime.words[root + 12u] = 8u;
    runtime.trace.clear();
    pal4::ActorManagerDynamicEffects::RemoveMatching(
        actor, 0x3000u, record, runtime);
    Check(runtime.trace == std::vector<std::string>{"lower"},
          "lower bound above key does not remove");

    runtime.words[root + 12u] = 7u;
    runtime.trace.clear();
    pal4::ActorManagerDynamicEffects::RemoveMatching(
        actor, 0x3000u, record, runtime);
    Check(runtime.trace ==
              std::vector<std::string>{"lower", "notify", "erase"},
          "same key notifies effect before erasing node");

    runtime.words[record] = static_cast<std::uint32_t>(-5);
    runtime.words[root + 12u] = static_cast<std::uint32_t>(-5);
    runtime.trace.clear();
    pal4::ActorManagerDynamicEffects::RemoveMatching(
        actor, 0x3000u, record, runtime);
    Check(runtime.trace ==
              std::vector<std::string>{"lower", "notify", "erase"},
          "negative keys use signed ordering");
}

void InvalidAndEmptyTree() {
    Runtime runtime;
    runtime.valid = false;
    pal4::ActorManagerDynamicEffects::Add(actor, 0x3000u, record, runtime);
    Check(runtime.trace == std::vector<std::string>{
              "lower", "create", "valid", "reset"},
          "invalid handle stops after old-effect lookup and resets");

    runtime.valid = true;
    runtime.trace.clear();
    pal4::ActorManagerDynamicEffects::Add(actor, 0x3000u, record, runtime);
    Check(runtime.inserted && runtime.result_node == 0x7000u &&
          runtime.trace == std::vector<std::string>{
              "lower", "create", "valid", "copy", "first",
              "insert", "result-insert", "reset", "reset"},
          "empty tree inserts and releases both temporary handles");
}

void ExistingAndForcedInsertion() {
    Runtime runtime;
    runtime.words[head + 4u] = root;
    runtime.words[root] = nil;
    runtime.words[root + 8u] = nil;
    runtime.words[root + 12u] = 7u;
    runtime.first_node = root;
    pal4::ActorManagerDynamicEffects::Add(actor, 0x3000u, record, runtime);
    Check(!runtime.inserted && runtime.result_node == root &&
          runtime.trace == std::vector<std::string>{
              "lower", "create", "valid", "copy", "result-existing",
              "reset", "reset"},
          "equal key reuses existing node when unique flag is clear");

    runtime.bytes[actor + 844u] = 1u;
    runtime.trace.clear();
    pal4::ActorManagerDynamicEffects::Add(actor, 0x3000u, record, runtime);
    Check(runtime.inserted && runtime.result_node == 0x7000u &&
          runtime.trace == std::vector<std::string>{
              "lower", "create", "valid", "copy", "insert",
              "result-insert", "reset", "reset"},
          "set flag forces insertion even for an equal key");
}

void PredecessorDecision() {
    Runtime runtime;
    runtime.words[head + 4u] = root;
    runtime.words[root + 12u] = 10u;
    runtime.words[root] = 0x5100u;
    runtime.words[0x5100u + 12u] = 5u;
    runtime.words[0x5100u + 8u] = 0x5200u;
    runtime.words[0x5200u + 12u] = 8u;
    runtime.words[0x5200u] = nil;
    runtime.first_node = 0x5100u;
    runtime.predecessor = 0x5100u;
    pal4::ActorManagerDynamicEffects::Add(actor, 0x3000u, record, runtime);
    Check(runtime.inserted && runtime.trace ==
              std::vector<std::string>{
                  "lower", "create", "valid", "copy", "first",
                  "predecessor", "insert", "result-insert",
                  "reset", "reset"},
          "key above predecessor inserts before candidate");

    runtime.words[0x5100u + 12u] = 7u;
    runtime.trace.clear();
    pal4::ActorManagerDynamicEffects::Add(actor, 0x3000u, record, runtime);
    Check(!runtime.inserted && runtime.result_node == 0x5100u &&
          runtime.trace == std::vector<std::string>{
              "lower", "create", "valid", "copy", "first",
              "predecessor", "result-existing", "reset", "reset"},
          "matching predecessor reuses node");
}

}  // namespace

int main() {
    try {
        NullAndRemoval();
        InvalidAndEmptyTree();
        ExistingAndForcedInsertion();
        PredecessorDecision();
        std::cout << "actor_manager_dynamic_effects_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
