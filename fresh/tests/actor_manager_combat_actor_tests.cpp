#include "pal4/actor_manager_combat_actor.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t object = 0x1000u;
constexpr std::uint32_t monster_record = 0xb000u;
constexpr std::uint32_t name_text = 0xd000u;

struct Runtime final : pal4::IActorManagerCombatActorRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> trace;
    std::vector<std::uint32_t> copied_destinations;
    std::vector<std::uint32_t> shared_records;
    Runtime() {
        // Independent record image: property prefix plus monster extension.
        for (std::uint32_t offset = 0; offset < 596u; offset += 4u)
            words[monster_record + offset] = 1000u + offset;
        for (std::uint32_t i = 0; i < 11u; ++i) {
            const auto record = monster_record + 408u + i * 16u;
            words[record + 4u] = 0u;
            words[record + 8u] = 0u;
            words[record + 12u] = 0u;
        }
        words[monster_record + 412u] = name_text;
        words[monster_record + 416u] = 5u;
        words[monster_record + 420u] = 5u;
        bytes[name_text - 1u] = 1u;
    }
    bool damage_allocation_fails = false;
    bool facet_allocation_fails = false;

    std::uint32_t Allocate(std::uint32_t size) override {
        trace.emplace_back("allocate:" + std::to_string(size));
        if (size == 12u) return damage_allocation_fails ? 0u : 0x8000u;
        Check(size == 8u, "facet allocation size");
        return facet_allocation_fails ? 0u : 0x9000u;
    }
    void Free(std::uint32_t address) override {
        trace.emplace_back("free:" + std::to_string(address));
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        if (address < object + 896u || address >= object + 1156u)
            trace.emplace_back("write:" + std::to_string(address));
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        if (address != name_text - 1u)
            trace.emplace_back("byte:" + std::to_string(address));
    }
    std::uint32_t ConstructDamage(std::uint32_t storage) override {
        Check(storage == 0x8000u, "damage constructor storage");
        trace.emplace_back("damage");
        return 0x8100u;
    }
    void ConstructActorData(std::uint32_t address, std::uint32_t mode,
                            std::uint32_t damage) override {
        Check(address == object && mode == 2u &&
                  damage == (damage_allocation_fails ? 0u : 0x8100u),
              "actor data constructor inputs");
        trace.emplace_back("base-construct");
    }
    void InitializeArrays(std::uint32_t address) override {
        Check(address == object + 896u, "embedded array offset");
        trace.emplace_back("arrays");
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void EraseText(std::uint32_t, std::uint32_t, std::uint32_t) override {
        throw std::logic_error("constructor source must not alias actor text");
    }
    void SetErrorFlag(std::uint32_t record, std::uint8_t value) override {
        Check(value == 1u, "copy text release mode");
        shared_records.push_back(record);
    }
    bool AllocateText(std::uint32_t, std::uint32_t length,
                      std::uint8_t mode) override {
        Check(length == 0u && mode == 1u, "empty text copy path");
        return false;
    }
    void SetTextSize(std::uint32_t, std::uint32_t) override {
        throw std::logic_error("no deep text allocation in shared-name fixture");
    }
    void CopyBytes(std::uint32_t destination, std::uint32_t source,
                   std::uint32_t count) override {
        Check(source == monster_record && count == 336u,
              "both actor property snapshots use the complete database prefix");
        copied_destinations.push_back(destination);
        trace.emplace_back("snapshot:" + std::to_string(destination));
        for (std::uint32_t offset = 0; offset < count; offset += 4u)
            words[destination + offset] = words.at(source + offset);
    }
    std::uint32_t ConstructFacet(std::uint32_t storage,
                                 std::uint32_t address) override {
        Check(storage == 0x9000u && address == object,
              "facet constructor inputs");
        trace.emplace_back("facet");
        return 0x9100u;
    }
    void CleanupProperty(std::uint32_t address,
                         std::uint32_t context) override {
        Check(address == object && context == 0xa000u,
              "property cleanup context");
        trace.emplace_back("property-cleanup");
    }
    void DestroyFacet(std::uint32_t facet) override {
        Check(facet == 0x9100u, "facet destructor address");
        trace.emplace_back("facet-destroy");
    }
    void DestroyTextArray(std::uint32_t address,
                          std::uint32_t count,
                          std::uint32_t stride) override {
        Check(stride == 16u, "embedded text stride");
        if (address == object + 1064u) {
            Check(count == 5u, "last text array count");
            trace.emplace_back("texts-5");
        } else {
            Check(address == object + 968u && count == 6u,
                  "first text array layout");
            trace.emplace_back("texts-6");
        }
    }
    void DestroyActorData(std::uint32_t address,
                          std::uint32_t context) override {
        Check(address == object && context == 0xa000u,
              "base destructor context");
        trace.emplace_back("base-destroy");
    }
};

void Construction() {
    Runtime runtime;
    Check(pal4::ActorManagerCombatActor::Construct(
              object, monster_record, runtime) == object &&
              runtime.words[object] == 0x844d08u &&
              runtime.words[object + 716u] == 0x844cf0u &&
              runtime.words[object + 724u] == 0x844ce8u &&
              runtime.words[object + 1156u] == 0x9100u &&
              runtime.bytes[object + 1160u] == 0u,
          "constructor writes derived vtables, facet and flag");
    Check(runtime.trace == std::vector<std::string>{
              "allocate:12", "damage", "base-construct", "arrays",
              "write:4096", "write:4812", "write:4820",
              "snapshot:4100", "snapshot:4436",
              "allocate:8", "facet", "write:5252", "byte:5256"},
          "constructor copies the database record before facet construction");
    Check(runtime.copied_destinations == std::vector<std::uint32_t>{
              object + 4u, object + 340u} &&
              runtime.shared_records == std::vector<std::uint32_t>{object + 968u} &&
              runtime.words.at(object + 972u) == name_text &&
              runtime.words.at(object + 976u) == 5u &&
              runtime.bytes.at(name_text - 1u) == 2u,
          "constructor reuses CopyFrom and retains source record text");
    for (std::uint32_t offset = 0; offset < 336u; offset += 4u)
        Check(runtime.words.at(object + 4u + offset) == 1000u + offset &&
                  runtime.words.at(object + 340u + offset) == 1000u + offset,
              "initial and current combat properties preserve database values");
    for (std::uint32_t offset = 0; offset < 72u; offset += 4u)
        Check(runtime.words.at(object + 896u + offset) == 1336u + offset,
              "monster extension scalar values copied");
    for (std::uint32_t offset = 248u; offset < 260u; offset += 4u)
        Check(runtime.words.at(object + 896u + offset) == 1336u + offset,
              "monster extension trailing values copied");
}

void MissingOptionalAllocations() {
    Runtime runtime;
    runtime.damage_allocation_fails = true;
    runtime.facet_allocation_fails = true;
    (void)pal4::ActorManagerCombatActor::Construct(
        object, monster_record, runtime);
    Check(runtime.words[object + 1156u] == 0u &&
              runtime.trace == std::vector<std::string>{
                  "allocate:12", "base-construct", "arrays",
                  "write:4096", "write:4812", "write:4820",
                  "snapshot:4100", "snapshot:4436", "allocate:8", "write:5252", "byte:5256"},
          "null optional allocations skip only their constructors");
}

void Accessors() {
    Runtime runtime;
    runtime.words[object + 1148u] = 17u;
    runtime.words[object + 1152u] = 29u;
    runtime.words[object + 960u] = 41u;
    Check(pal4::ActorManagerCombatActor::Value1148(object, runtime) == 17u &&
              pal4::ActorManagerCombatActor::Value1152(object, runtime) ==
                  29u &&
              pal4::ActorManagerCombatActor::Value960(object, runtime) == 41u,
          "three accessors read their exact word offsets");
    for (const auto value : {0u, static_cast<std::uint32_t>(-1), 1u}) {
        runtime.words[object + 1144u] = value;
        Check(pal4::ActorManagerCombatActor::HasPositive1144(
                  object, runtime) == (value == 1u),
              "state predicate uses signed positive comparison");
    }
}

void Destruction() {
    for (const auto flags : {0u, 2u, 3u}) {
        Runtime runtime;
        runtime.words[object + 1156u] = 0x9100u;
        Check(pal4::ActorManagerCombatActor::Delete(
                  object, static_cast<std::uint8_t>(flags), 0xa000u,
                  runtime) == object,
              "scalar deleting destructor returns original object");
        const std::vector<std::string> expected{
            "write:4096", "write:4812", "write:4820",
            "property-cleanup", "facet-destroy", "free:37120",
            "texts-5", "texts-6", "base-destroy"};
        Check(runtime.trace.size() == expected.size() +
                  ((flags & 1u) != 0u ? 1u : 0u) &&
                  std::equal(expected.begin(), expected.end(),
                             runtime.trace.begin()) &&
                  (((flags & 1u) == 0u) ||
                   runtime.trace.back() == "free:4096"),
              "destructor order and optional object free");
    }
    Runtime no_facet;
    no_facet.words[object + 1156u] = 0u;
    pal4::ActorManagerCombatActor::Destroy(object, 0xa000u, no_facet);
    Check(no_facet.trace.size() == 7u &&
              no_facet.trace[3] == "property-cleanup" &&
              no_facet.trace[4] == "texts-5",
          "null facet skips only facet destruction");
}

}  // namespace

int main() {
    try {
        Construction();
        MissingOptionalAllocations();
        Accessors();
        Destruction();
        std::cout << "actor_manager_combat_actor_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
