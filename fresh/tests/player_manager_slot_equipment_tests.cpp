#include "pal4/player_manager_slot_equipment.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Action {
    bool weapon{};
    pal4::PlayerEquipmentVisuals visuals;
};

struct Runtime final : pal4::PlayerManagerSlotEquipmentRuntimeStub {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint32_t> indexed;
    std::vector<std::string> events;
    std::vector<Action> actions;
    std::string name;
    bool item_found{true};
    bool throw_in_render{};
    std::uint32_t cleanup_result{0x99u};

    Runtime() {
        words[0x5920u] = 0x77u;
        words[0x5004u] = 0x8000u;
        words[0x90fcu] = 0x21u;
        words[0x90ccu] = 0x22u;
        words[0x90ecu] = 0x23u;
        words[0x90dcu] = 0x24u;
        words[0x911cu] = 0x25u;
        indexed = {{9u, 0x29u}, {6u, 0x26u}, {3u, 0x23u},
                   {5u, 0x25u}, {4u, 0x24u}};
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return words[address];
    }
    std::uint32_t InventoryManager() override {
        events.emplace_back("inventory");
        return 0x7000u;
    }
    std::uint32_t FindItem(std::uint32_t collection,
                           std::uint32_t key,
                           std::uint32_t category) override {
        Check(collection == 0x707cu && key == 0x77u &&
                  category == 7u, "inventory lookup operands");
        events.emplace_back("find-item");
        return item_found ? 0x9000u : 0u;
    }
    std::uint32_t CreateName(std::uint32_t source) override {
        Check(source == 0x8000u, "character name source");
        events.emplace_back("create-name");
        return 0x8100u;
    }
    bool NameEquals(std::uint32_t handle,
                    const char* expected) override {
        Check(handle == 0x8100u, "name comparison handle");
        events.push_back(std::string("compare:") + expected);
        return name == expected;
    }
    std::uint32_t CleanupName(std::uint32_t handle) override {
        Check(handle == 0x8100u, "name cleanup handle");
        events.emplace_back("cleanup-name");
        return cleanup_result;
    }
    std::uint32_t EmptyText() override { return 0xe000u; }
    std::uint32_t SharedEffect() override {
        events.emplace_back("shared-effect");
        return 0xe100u;
    }
    std::uint32_t IndexedText(std::uint32_t item,
                              std::uint32_t index) override {
        Check(item == 0x9000u, "indexed item address");
        events.push_back("indexed:" + std::to_string(index));
        return indexed[index];
    }
    void ApplyVisuals(std::uint32_t object,
                      const pal4::PlayerEquipmentVisuals& visuals) override {
        Check(object == 0x5000u, "visual object");
        events.emplace_back("visuals");
        actions.push_back({false, visuals});
        if (throw_in_render) throw std::runtime_error("visuals");
    }
    void ApplyWeapon(std::uint32_t object,
                     const pal4::PlayerEquipmentVisuals& visuals) override {
        Check(object == 0x5000u, "weapon object");
        events.emplace_back("weapon");
        actions.push_back({true, visuals});
    }
};

void CheckVisuals(const pal4::PlayerEquipmentVisuals& actual,
                  std::uint32_t primary, std::uint32_t secondary,
                  std::uint32_t extra, std::uint32_t trailing) {
    Check(actual.primary == primary && actual.secondary == secondary &&
              actual.shared_effect == 0xe100u &&
              actual.extra == extra && actual.trailing == trailing,
          "equipment visual parameters and ordering");
}

void MissingItemAndUnknownName() {
    Runtime missing;
    missing.item_found = false;
    Check(pal4::PlayerManagerSlotEquipment::Apply(
              0x5000u, missing) == 0u &&
              missing.events == std::vector<std::string>{
                  "read:22816", "inventory", "find-item"},
          "missing inventory item returns before name construction");

    Runtime unknown;
    unknown.name = "Unknown";
    Check(pal4::PlayerManagerSlotEquipment::Apply(
              0x5000u, unknown) == 0x99u &&
              unknown.actions.empty() &&
              unknown.events.back() == "cleanup-name" &&
              unknown.events.size() == 10u,
          "unknown name checks all four roles and cleans the copied name");
}

void TianheAndLinsha() {
    Runtime tianhe;
    tianhe.name = "Player-TIANHE";
    Check(pal4::PlayerManagerSlotEquipment::Apply(
              0x5000u, tianhe) == 0x99u &&
              tianhe.actions.size() == 1u &&
              !tianhe.actions[0].weapon,
          "Tianhe applies only common visuals");
    CheckVisuals(tianhe.actions[0].visuals, 0x24u, 0x23u,
                 0x22u, 0x21u);

    for (const auto alternate : {0u, 1u}) {
        for (const auto indexed_weapon : {0u, 1u}) {
            Runtime linsha;
            linsha.name = "Player-LINSHA";
            linsha.words[0x9120u] = alternate;
            linsha.words[0x9130u] = indexed_weapon;
            Check(pal4::PlayerManagerSlotEquipment::Apply(
                      0x5000u, linsha) == 0x99u &&
                      linsha.actions.size() == 2u &&
                      !linsha.actions[0].weapon &&
                      linsha.actions[1].weapon &&
                      linsha.events.back() == "cleanup-name",
                  "Linsha applies visuals then weapon and cleans name");
            CheckVisuals(linsha.actions[0].visuals,
                         alternate == 0u ? 0x24u : 0x25u,
                         0x23u, 0x22u, 0x21u);
            CheckVisuals(linsha.actions[1].visuals,
                         indexed_weapon == 0u ? 0x24u : 0x29u,
                         0x23u, 0x22u, 0x21u);
        }
    }
}

void MengliAndZiyingFallbacks() {
    for (const auto& name : {"Player-MENGLI", "Player-ZIYING"}) {
        Runtime runtime;
        runtime.name = name;
        runtime.indexed[6u] = 0u;
        runtime.indexed[5u] = 0u;
        Check(pal4::PlayerManagerSlotEquipment::Apply(
                  0x5000u, runtime) == 0x99u &&
                  runtime.actions.size() == 1u &&
                  runtime.actions[0].weapon ==
                      (runtime.name == "Player-ZIYING"),
              "Mengli and Ziying select distinct rendering calls");
        CheckVisuals(runtime.actions[0].visuals,
                     0x24u, 0xe000u, 0x23u, 0xe000u);
        std::vector<std::string> lookups;
        for (const auto& event : runtime.events)
            if (event.starts_with("indexed:")) lookups.push_back(event);
        Check(lookups == std::vector<std::string>{
                  "indexed:6", "indexed:3", "indexed:5", "indexed:4"},
              "indexed fields are requested in IDA order");
    }
}

void ExceptionCleansName() {
    Runtime runtime;
    runtime.name = "Player-TIANHE";
    runtime.throw_in_render = true;
    try {
        static_cast<void>(pal4::PlayerManagerSlotEquipment::Apply(
            0x5000u, runtime));
        Check(false, "render exception must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.events.back() == "cleanup-name",
              "copied name is cleaned on rendering exception");
    }
}

}  // namespace

int main() {
    MissingItemAndUnknownName();
    TianheAndLinsha();
    MengliAndZiyingFallbacks();
    ExceptionCleansName();
    std::cout << "4 player-manager equipment groups passed\n";
}
