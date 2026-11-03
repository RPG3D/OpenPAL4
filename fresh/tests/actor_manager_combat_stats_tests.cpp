#include "pal4/actor_manager_combat_actor.h"

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
constexpr std::uint32_t temporary = 0xf000u;

struct Runtime final : pal4::IActorManagerCombatStatsRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> trace;
    std::vector<std::string> property_calls;
    std::int32_t maximum_health = 100;
    std::int32_t maximum_value9 = 50;
    std::uint32_t damage = 0x9100u;
    std::uint32_t storage = 0x9000u;
    bool constructor_throws = false;
    bool force_health_above_maximum = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == actor + 580u || address == actor + 576u ||
            address == actor + 572u)
            trace.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        if (address == actor + 572u)
            trace.emplace_back("health:" + std::to_string(value));
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    void DeleteActor(std::uint32_t address,
                     std::uint32_t flags) override {
        Check(flags == 1u, "old owned property deleting destructor flag");
        trace.emplace_back("delete:" + std::to_string(address));
    }
    void Free(std::uint32_t address) override {
        trace.emplace_back("free:" + std::to_string(address));
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == 12u, "damage allocation size");
        trace.emplace_back("allocate");
        return storage;
    }
    std::uint32_t ConstructDamage(std::uint32_t address) override {
        Check(address == storage, "damage constructor storage");
        trace.emplace_back("construct");
        if (constructor_throws) throw std::runtime_error("constructor failed");
        return damage;
    }
    std::uint32_t TemporarySlotAddress() override {
        trace.emplace_back("temporary");
        return temporary;
    }
    void CopyBytes(std::uint32_t destination,
                   std::uint32_t source,
                   std::uint32_t size) override {
        Check(destination == actor + 340u && source == actor + 4u &&
                  size == 336u,
              "baseline copied into derived data");
        trace.emplace_back("copy-baseline");
        for (std::uint32_t word = 0u; word < 84u; ++word)
            words[destination + word * 4u] = words[source + word * 4u];
    }
    void ProcessSkills(std::uint32_t address,
                       std::uint32_t mode) override {
        Check(address == actor && mode == 1u,
              "skill batch receives actor and mode one");
        trace.emplace_back("skills");
    }
    std::int32_t DataElement(std::uint32_t data,
                             std::uint32_t index) override {
        Check(data == actor + 340u && (index == 0u || index == 2u),
              "data element lookup arguments");
        trace.emplace_back("element:" + std::to_string(index));
        return index == 0u ? maximum_health : maximum_value9;
    }
    void SetValidatedProperty(std::uint32_t data,
                              std::uint32_t index,
                              std::uint32_t value) override {
        Check(data == actor + 340u &&
                  (index == 7u || index == 8u || index == 9u),
              "validated property arguments");
        property_calls.emplace_back(std::to_string(index) + ":" +
                                    std::to_string(value));
        trace.emplace_back("set:" + std::to_string(index));
        if (index == 7u) words[actor + 572u] = value;
        if (index == 8u) {
            words[actor + 576u] = value;
            if (force_health_above_maximum)
                words[actor + 572u] = maximum_health + 1u;
        }
        if (index == 9u) words[actor + 580u] = value;
    }
    std::int32_t UpdateHealthStatus(std::uint32_t address) override {
        Check(address == actor, "health update receives actor");
        trace.emplace_back("update-health");
        return 77;
    }
};

void InitializeValues(Runtime& runtime, std::int32_t health,
                      std::uint32_t value8, std::uint32_t value9) {
    runtime.words[actor + 572u] = static_cast<std::uint32_t>(health);
    runtime.words[actor + 576u] = value8;
    runtime.words[actor + 580u] = value9;
    runtime.words[actor + 8u] = 33u;
}

void ReplacesOwnedDamageAndClamps() {
    Runtime runtime;
    runtime.bytes[actor + 796u] = 1u;
    runtime.words[actor + 800u] = 0x8000u;
    InitializeValues(runtime, 150, 20u, 80u);
    Check(pal4::ActorManagerCombatActor::RebuildStats(actor, runtime) == 77 &&
              runtime.bytes[actor + 796u] == 1u &&
              runtime.words[actor + 800u] == 0x9100u &&
              runtime.bytes[temporary] == 0u &&
              runtime.words[actor + 572u] == 100u &&
              runtime.words[actor + 576u] == 20u &&
              runtime.words[actor + 580u] == 33u &&
              runtime.property_calls == std::vector<std::string>{
                  "9:80", "8:20", "9:33"},
          "new damage takes ownership and values follow both clamps");
    const std::vector<std::string> expected{
              "allocate", "construct", "temporary", "delete:32768",
              "read:4676", "read:4672", "read:4668",
              "copy-baseline", "skills", "health:150", "element:0",
              "element:0", "health:100", "set:9", "set:8",
              "read:4668", "element:0", "read:4676", "element:2",
              "set:9", "update-health"};
    Check(runtime.trace == expected,
          "damage replacement, saved reads and property update order");
}

void NegativeAndPostSetterHealth() {
    Runtime negative;
    InitializeValues(negative, -5, 8u, 9u);
    Check(pal4::ActorManagerCombatActor::RebuildStats(actor, negative) == 77 &&
              negative.words[actor + 572u] == 0u &&
              negative.property_calls == std::vector<std::string>{
                  "9:9", "8:8"},
          "negative old health is reset to zero");

    Runtime changed;
    InitializeValues(changed, 40, 8u, 9u);
    changed.force_health_above_maximum = true;
    Check(pal4::ActorManagerCombatActor::RebuildStats(actor, changed) == 77 &&
              changed.words[actor + 572u] == 100u &&
              changed.property_calls == std::vector<std::string>{
                  "9:9", "8:8", "7:100"},
          "health changed by validated setters is capped again");
}

void MissingAllocationAndSamePointer() {
    Runtime missing;
    missing.storage = 0u;
    missing.bytes[actor + 796u] = 1u;
    missing.words[actor + 800u] = 0x8000u;
    InitializeValues(missing, 5, 6u, 7u);
    Check(pal4::ActorManagerCombatActor::RebuildStats(actor, missing) == 77 &&
              missing.words[actor + 800u] == 0u &&
              missing.bytes[actor + 796u] == 0u &&
              missing.trace[1] == "temporary" &&
              missing.trace[2] == "delete:32768",
          "missing damage allocation still replaces old owned slot");

    Runtime same;
    same.words[actor + 800u] = 0x9100u;
    same.bytes[actor + 796u] = 0u;
    InitializeValues(same, 5, 6u, 7u);
    Check(pal4::ActorManagerCombatActor::RebuildStats(actor, same) == 77 &&
              same.words[actor + 800u] == 0x9100u &&
              same.bytes[actor + 796u] == 1u &&
              same.trace[3] == "read:4676",
          "same pointer merges ownership without deleting it");
}

void ConstructorFailure() {
    Runtime runtime;
    runtime.constructor_throws = true;
    bool failed = false;
    try {
        (void)pal4::ActorManagerCombatActor::RebuildStats(actor, runtime);
    } catch (const std::runtime_error&) {
        failed = true;
    }
    Check(failed && runtime.trace == std::vector<std::string>{
              "allocate", "construct", "free:36864"},
          "constructor exception frees unowned allocation");
}

}  // namespace

int main() {
    try {
        ReplacesOwnedDamageAndClamps();
        NegativeAndPostSetterHealth();
        MissingAllocationAndSamePointer();
        ConstructorFailure();
        std::cout << "actor_manager_combat_stats_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
