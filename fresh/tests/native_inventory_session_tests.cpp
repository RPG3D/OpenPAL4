#include "pal4/native_inventory_session.h"
#include "pal4/game_data_race_stats.h"
#include "pal4/native_player_inventory_state_runtime.h"
#include "pal4/player_manager_object.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void TestEmptyInventoryLifecycle() {
    pal4::NativeInventorySession inventory;
    const auto object = inventory.Start();
    Check(object != 0u && inventory.Start() == object &&
              inventory.OutstandingAllocations() == 1u &&
              inventory.ReadDword(object) == 0x84346cu &&
              inventory.ReadDword(object + 0x7cu) == 0x84346cu &&
              inventory.ReadDword(object + 0x80u) == 1u,
          "native inventory constructs base and seven-slot state once");
    for (std::uint32_t index = 0u; index < 7u; ++index) {
        const auto slot = object + 0x84u + index * 16u;
        Check(inventory.ReadDword(slot + 4u) == 0u &&
                  inventory.ReadDword(slot + 8u) == 0u,
              "new inventory seven-slot collection starts empty");
    }
    for (std::uint32_t index = 0u; index < 3u; ++index) {
        const auto slot = object + 0x104u + index * 16u;
        Check(inventory.ReadDword(slot + 4u) == 0u &&
                  inventory.ReadDword(slot + 8u) == 0u,
              "new inventory three-slot array starts empty");
    }
    pal4::InventoryItemSlotsState player_slots{};
    pal4::InventoryItemSlots::ProcessAndInsertEach(player_slots, 0u,
                                                    inventory);
    Check(pal4::InventoryItemSlots::ProcessEach(player_slots, 0u, 0u,
                                                 inventory) == 0u,
          "six empty equipped-item queries do not modify player stats");
    for (std::uint32_t index = 0u; index < 6u; ++index)
        Check(inventory.FindInventoryEntry(object + 0x7cu, 0u, 7u) == 0u,
              "zero player item has no inventory entry");
    Check(inventory.FindInventoryEntry(object + 0x7cu, 3500u, 7u) == 0u,
          "unpopulated inventory has no GameDB-backed entry");
    inventory.Reset(); // The original constructor leaves money to Reset/load.
    Check(inventory.UpdateCounter(500)==500 && inventory.UpdateCounter(-600)==0 &&
              inventory.UpdateCounter(10000000)==9999999 &&
              inventory.UpdateCounter(2147483647)==0,
          "native inventory reuses original signed wrap and both counter clamps");
    inventory.Close();
    Check(inventory.Address() == 0u &&
              inventory.OutstandingAllocations() == 0u,
          "native inventory releases its object and nested storage");
    inventory.Close();
}

void TestOriginalEquipment(const char* game_root) {
    pal4::NativeGameDatabase database;
    Check(database.Load(game_root), "load original equipment database");
    pal4::NativeInventorySession inventory;
    inventory.BindDatabase(database);
    std::vector<std::uint32_t> observed;
    inventory.BindItemObserver([&](std::uint32_t id) { observed.push_back(id); });
    const auto object = inventory.Start();
    const auto bow = inventory.AddItem(3103u, 1);
    Check(bow != 0u && inventory.ItemCount(3103u) == 1u &&
              inventory.ReadDword(bow) == inventory.PropertyAddress(3103u) &&
              observed == std::vector<std::uint32_t>{3103u},
          "original bow adds a property-backed inventory record and discovery observation");
    const auto sequence = inventory.ReadDword(bow + 4u);
    (void)inventory.AddItem(3103u, 150);
    Check(inventory.ItemCount(3103u) == 99u &&
              inventory.ReadDword(object + 116u) - inventory.ReadDword(object + 112u) == 4u &&
              inventory.ReadDword(inventory.ReadDword(object + 112u)) == sequence,
          "repeat acquisition caps quantity and deduplicates the recent sequence");
    (void)inventory.AddItem(3103u, -98);
    Check(inventory.ItemCount(3103u) == 1u && observed.size() == 2u,
          "negative add removes quantity without recording a new discovery");
    (void)inventory.RemoveItem(3103u, -1);
    Check(inventory.ItemCount(3103u) == 2u && observed.size() == 3u,
          "negative remove uses the same acquisition path");
    (void)inventory.RemoveItem(3103u, 20);
    Check(inventory.ItemCount(3103u) == 0u &&
              inventory.ReadDword(object + 116u) == inventory.ReadDword(object + 112u),
          "over-removal erases both the inventory record and recent sequence");
    (void)inventory.AddItem(3241u, 1);
    Check(inventory.ItemCount(3241u) == 1u && inventory.AddItem(0xffffffffu, 1) == 0u,
          "second opening property is valid and missing property does not insert");
    inventory.Reset();
    Check(inventory.ItemCount(3241u) == 0u &&
              inventory.ReadDword(object + 4u) == 1u,
          "NewGame reset clears ordinary items and resets their sequence");
    const auto sword = inventory.ProcessEquipment(object + 0x7cu, 3501u);
    const auto armor = inventory.ProcessEquipment(object + 0x7cu, 3586u);
    Check(sword != 0u && armor != 0u &&
              inventory.ReadDword(sword + 144u) == 3501u &&
              inventory.ReadDword(sword + 148u) == 0u &&
              inventory.ReadDword(armor + 144u) == 3586u &&
              inventory.ReadDword(armor + 148u) == 4u,
          "inventory copies original equipment records");
    const auto sword_sequence = inventory.ReadDword(sword + 400u);
    const auto armor_sequence = inventory.ReadDword(armor + 400u);
    Check(sword_sequence != armor_sequence &&
              inventory.FindInventoryEntry(object + 0x7cu,
                                           sword_sequence, 7u) == sword &&
              inventory.FindInventoryEntry(object + 0x7cu,
                                           armor_sequence, 7u) == armor,
          "inventory indexes distinct snapshot sequences");
    pal4::InventoryItemSlotsState slots{};
    Check(pal4::InventoryItemSlots::FindSlotOrClassify(
              slots, sword, inventory) == 0u &&
              pal4::InventoryItemSlots::FindSlotOrClassify(
              slots, armor, inventory) == 2u,
          "original sword and body armor map to player slots zero and two");
    slots.ids[0u] = sword_sequence;
    slots.ids[2u] = armor_sequence;
    pal4::GameDataRaceStats stats;
    (void)stats.Reset();
    const auto stats_address = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(stats.Data().data()));
    (void)pal4::InventoryItemSlots::ProcessEach(
        slots, stats_address, 0u, inventory);
    Check(stats.Data()[4u] == 10u && stats.Data()[5u] == 5u,
          "equipped original records contribute physical and toughness stats");
    pal4::PlayerManagerObjectState players{};
    pal4::NativeBaseObjectMemory memory;
    pal4::NativePlayerInventoryStateRuntime activation(players, memory, inventory);
    const auto manager = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&players));
    const auto first = manager;
    const auto second = manager + pal4::PlayerManagerObject::kElementBytes;
    const auto last = manager + 3u * pal4::PlayerManagerObject::kElementBytes;
    activation.WriteDword(first + 0x920u, sword_sequence);
    activation.WriteDword(first + 0xb08u, 1u);
    activation.WriteDword(second + 0x920u, sword_sequence);
    activation.WriteDword(second + 0x928u, armor_sequence);
    activation.WriteDword(second + 0x934u, 0xffffffffu);
    Check(activation.CheckPlayerCondition(sword) == 1u &&
        activation.CheckPlayerCondition(armor) == 0u,
        "equipment condition ignores inactive roles and identifies active instance owner");
    (void)pal4::PlayerManagerSlotInventoryState::Update(second, 1u, activation);
    Check(activation.ReadDword(second + 0x920u) == 0u &&
        activation.ReadDword(second + 0x928u) == armor_sequence &&
        activation.ReadDword(second + 0x934u) == 0u &&
        activation.ReadDword(second + 0xb08u) == 1u,
        "activation clears occupied and missing instances while retaining available original armor");
    (void)pal4::PlayerManagerSlotInventoryState::Update(first, 0u, activation);
    activation.WriteDword(last + 0x934u, sword_sequence);
    (void)pal4::PlayerManagerSlotInventoryState::Update(last, 1u, activation);
    Check(activation.ReadDword(first + 0x920u) == sword_sequence &&
        activation.ReadDword(last + 0x934u) == sword_sequence &&
        activation.CheckPlayerCondition(sword) == 1u,
        "disable retains gear and last role can activate an instance held only by inactive role");
    // The original checks every active role, including the activating role itself.
    (void)pal4::PlayerManagerSlotInventoryState::Update(last, 1u, activation);
    Check(activation.ReadDword(last + 0x934u) == 0u,
        "repeat activation preserves original self-ownership exclusion behavior");
    inventory.Reset();
    Check(inventory.Address() == object &&
              !inventory.FindInventoryEntry(object + 0x7cu, sword_sequence, 7u) &&
              !inventory.FindInventoryEntry(object + 0x7cu, armor_sequence, 7u) &&
              inventory.ReadDword(object + 0x134u) == 0u &&
              inventory.ReadDword(object + 0x80u) == 1u,
          "NewGame reset removes populated equipment while retaining inventory identity");
    inventory.Reset();
    Check(inventory.ProcessEquipment(object + 0x7cu, 3501u) != 0u,
          "reset inventory supports original equipment insertion again");
    inventory.Close();
    (void)pal4::PlayerManagerSlotInventoryState::Update(second, 0u, activation);
    Check(activation.ReadDword(second + 0xb08u) == 0u &&
        activation.ReadDword(second + 0x928u) == armor_sequence,
        "disable needs no inventory lookup even after inventory shutdown");
    Check(inventory.OutstandingAllocations() == 0u,
          "inventory releases equipment snapshots and copied text");
}

}  // namespace

int main(int argc, char** argv) {
    TestEmptyInventoryLifecycle();
    if (argc == 2) TestOriginalEquipment(argv[1]);
    return EXIT_SUCCESS;
}
