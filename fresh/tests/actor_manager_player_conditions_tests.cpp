#include "pal4/actor_manager_player_conditions.h"

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
constexpr std::uint32_t requirement = 0x2000u;
constexpr std::uint32_t inventory = 0x3000u;

struct Runtime final : pal4::IActorManagerPlayerConditionRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint32_t> pairs;
    std::map<std::uint32_t, std::uint32_t> elements;
    std::vector<std::string> trace;
    bool used = false;
    bool fixed = true;
    bool indexed = true;
    bool script_exists = true;
    std::int32_t slot8 = 10;
    std::int32_t slot9 = 20;

    Runtime() {
        words[requirement] = 42u;
        words[inventory + 308u] = 100u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        trace.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    std::int32_t ReadSignedWord(std::uint32_t address) override {
        trace.emplace_back("signed:" + std::to_string(address));
        return static_cast<std::int32_t>(words[address]);
    }
    bool AlreadyUsed(std::uint32_t values,
                     std::uint32_t key) override {
        Check(values == actor + 920u && key == 42u,
              "already-used collection and key");
        trace.emplace_back("used");
        return used;
    }
    bool FixedContains(std::uint32_t values,
                       std::uint32_t key) override {
        Check(values == actor + 920u && key == 42u,
              "fixed collection and key");
        trace.emplace_back("fixed");
        return fixed;
    }
    bool IndexedContains(std::uint32_t values,
                         std::uint32_t group,
                         std::uint32_t key) override {
        Check(values == actor + 920u && key == 42u &&
                  group == words[requirement + 4u],
              "indexed collection, group and key");
        trace.emplace_back("indexed");
        return indexed;
    }
    std::int32_t ActorSlot52(std::uint32_t actual_actor,
                             std::uint32_t mode) override {
        Check(actual_actor == actor && (mode == 8u || mode == 9u),
              "actor virtual slot 52 mode");
        trace.emplace_back("slot:" + std::to_string(mode));
        return mode == 8u ? slot8 : slot9;
    }
    bool ScriptElementExists(std::uint32_t actual_actor,
                             std::uint32_t key) override {
        Check(actual_actor == actor &&
                  key == words[requirement + 128u],
              "script existence argument");
        trace.emplace_back("script");
        return script_exists;
    }
    std::uint32_t Inventory() override {
        trace.emplace_back("inventory");
        return inventory;
    }
    std::uint32_t GameDbPair(std::uint32_t key) override {
        trace.emplace_back("pair:" + std::to_string(key));
        return pairs[key];
    }
    std::uint32_t FindInventoryElement(std::uint32_t actual_inventory,
                                       std::uint32_t id,
                                       std::uint32_t variant) override {
        Check(actual_inventory == inventory,
              "inventory element lookup receiver");
        trace.emplace_back("element:" + std::to_string(id) + ":" +
                           std::to_string(variant));
        return elements[id];
    }

    void AddPair(std::uint32_t key, std::uint32_t pair,
                 std::uint32_t id, std::uint32_t variant,
                 std::uint32_t element, std::int32_t count) {
        pairs[key] = pair;
        words[pair] = id;
        words[pair + 4u] = variant;
        elements[id] = element;
        if (element)
            words[element + 8u] = static_cast<std::uint32_t>(count);
    }
};

void PrimaryGates() {
    using Conditions = pal4::ActorManagerPlayerConditions;
    Runtime runtime;
    Check(Conditions::CheckPrimary(actor, requirement, runtime),
          "empty primary requirements succeed");
    Check(runtime.trace.front() == "read:8192" &&
              runtime.trace[1] == "used" &&
              runtime.trace[2] == "fixed" &&
              runtime.trace[3] == "slot:8",
          "primary initial call order");

    runtime = Runtime{};
    runtime.used = true;
    Check(!Conditions::CheckPrimary(actor, requirement, runtime) &&
              runtime.trace == std::vector<std::string>{
                  "read:8192", "used"},
          "already-used value short circuits membership");
    runtime = Runtime{};
    runtime.fixed = false;
    Check(!Conditions::CheckPrimary(actor, requirement, runtime) &&
              runtime.trace == std::vector<std::string>{
                  "read:8192", "used", "fixed"},
          "missing fixed value short circuits level");
    runtime = Runtime{};
    runtime.words[requirement + 116u] = 11u;
    Check(!Conditions::CheckPrimary(actor, requirement, runtime) &&
              runtime.trace.back() == "signed:8308",
          "primary level above actor level fails");
    runtime.words[requirement + 116u] = 10u;
    Check(Conditions::CheckPrimary(actor, requirement, runtime),
          "equal primary level succeeds");
    runtime = Runtime{};
    runtime.words[requirement + 128u] = 77u;
    runtime.script_exists = false;
    Check(!Conditions::CheckPrimary(actor, requirement, runtime) &&
              runtime.trace.back() == "script",
          "required script element fails before inventory access");
    runtime.script_exists = true;
    Check(Conditions::CheckPrimary(actor, requirement, runtime),
          "existing script element passes");
    runtime = Runtime{};
    runtime.words[requirement + 120u] = 101u;
    const auto resource_sufficient = Conditions::CheckPrimary(
        actor, requirement, runtime);
    Check(!resource_sufficient,
          "insufficient signed inventory resource fails");
    if (runtime.trace.back() != "signed:12596")
        throw std::runtime_error("inventory resource trace: " +
                                 runtime.trace.back());
    runtime.words[requirement + 120u] = 100u;
    Check(Conditions::CheckPrimary(actor, requirement, runtime),
          "equal inventory resource passes");
}

void PrimaryItems() {
    using Conditions = pal4::ActorManagerPlayerConditions;
    Runtime runtime;
    runtime.words[requirement + 124u] = 70u;
    Check(Conditions::CheckPrimary(actor, requirement, runtime),
          "missing database pair for optional item is skipped");
    runtime.AddPair(70u, 0x4000u, 1u, 2u, 0u, 0);
    Check(!Conditions::CheckPrimary(actor, requirement, runtime) &&
              runtime.trace.back() == "element:1:2",
          "known required item absent from inventory fails");
    runtime.AddPair(70u, 0x4000u, 1u, 2u, 0x5000u, 1);
    Check(Conditions::CheckPrimary(actor, requirement, runtime),
          "known required item present passes");

    for (std::uint32_t i = 0u; i < 5u; ++i) {
        Runtime item;
        item.words[requirement + 72u + 4u * i] = 80u + i;
        item.words[requirement + 92u + 4u * i] = 3u;
        item.AddPair(80u + i, 0x4100u, 10u, 2u, 0x5100u, 2);
        Check(!Conditions::CheckPrimary(actor, requirement, item),
              "each of five item quantities is enforced");
        item.words[0x5108u] = 3u;
        Check(Conditions::CheckPrimary(actor, requirement, item),
              "equal item quantity passes");
        item.elements[10u] = 0u;
        Check(!Conditions::CheckPrimary(actor, requirement, item),
              "missing inventory element has quantity zero");
        item.pairs[80u + i] = 0u;
        Check(Conditions::CheckPrimary(actor, requirement, item),
              "missing database pair skips quantity check");
    }
}

void SecondaryGates() {
    using Conditions = pal4::ActorManagerPlayerConditions;
    Runtime runtime;
    runtime.words[requirement + 4u] = 5u;
    runtime.words[requirement + 76u] = 20u;
    Check(Conditions::CheckSecondary(actor, requirement, runtime) &&
              runtime.trace == std::vector<std::string>{
                  "read:8192", "used", "read:8196", "indexed",
                  "slot:9", "signed:8268"},
          "secondary exact level and call order");
    runtime.words[requirement + 76u] = 21u;
    Check(!Conditions::CheckSecondary(actor, requirement, runtime),
          "secondary level above actor level fails");
    runtime = Runtime{};
    runtime.used = true;
    Check(!Conditions::CheckSecondary(actor, requirement, runtime) &&
              runtime.trace.size() == 2u,
          "secondary already-used gate short circuits");
    runtime = Runtime{};
    runtime.indexed = false;
    Check(!Conditions::CheckSecondary(actor, requirement, runtime) &&
              runtime.trace.back() == "indexed",
          "secondary missing indexed key skips actor level");
    runtime = Runtime{};
    runtime.words[requirement + 76u] = 0xffffffffu;
    Check(Conditions::CheckSecondary(actor, requirement, runtime),
          "secondary threshold comparison is signed");
}

}  // namespace

int main() {
    try {
        PrimaryGates();
        PrimaryItems();
        SecondaryGates();
        std::cout << "actor_manager_player_conditions_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
