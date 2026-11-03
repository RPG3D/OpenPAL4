#include "pal4/player_manager_slot_equipment.h"

namespace pal4 {

std::uint32_t PlayerManagerSlotEquipmentRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }
std::uint32_t PlayerManagerSlotEquipmentRuntimeStub::InventoryManager() {
    return 0u;
}
std::uint32_t PlayerManagerSlotEquipmentRuntimeStub::FindItem(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0u; }
std::uint32_t PlayerManagerSlotEquipmentRuntimeStub::CreateName(
    std::uint32_t) { return 0u; }
bool PlayerManagerSlotEquipmentRuntimeStub::NameEquals(
    std::uint32_t, const char*) { return false; }
std::uint32_t PlayerManagerSlotEquipmentRuntimeStub::CleanupName(
    std::uint32_t) { return 0u; }
std::uint32_t PlayerManagerSlotEquipmentRuntimeStub::EmptyText() {
    return 0u;
}
std::uint32_t PlayerManagerSlotEquipmentRuntimeStub::SharedEffect() {
    return 0u;
}
std::uint32_t PlayerManagerSlotEquipmentRuntimeStub::IndexedText(
    std::uint32_t, std::uint32_t) { return 0u; }
void PlayerManagerSlotEquipmentRuntimeStub::ApplyVisuals(
    std::uint32_t, const PlayerEquipmentVisuals&) {}
void PlayerManagerSlotEquipmentRuntimeStub::ApplyWeapon(
    std::uint32_t, const PlayerEquipmentVisuals&) {}

namespace {

std::uint32_t TextOrEmpty(std::uint32_t text,
                          IPlayerManagerSlotEquipmentRuntime& runtime) {
    return text == 0u ? runtime.EmptyText() : text;
}

PlayerEquipmentVisuals CommonVisuals(
    std::uint32_t item, std::uint32_t primary_offset,
    bool indexed_primary, IPlayerManagerSlotEquipmentRuntime& runtime) {
    const auto trailing = TextOrEmpty(runtime.ReadDword(item + 0xfcu), runtime);
    const auto extra = TextOrEmpty(runtime.ReadDword(item + 0xccu), runtime);
    const auto secondary = TextOrEmpty(
        runtime.ReadDword(item + 0xecu), runtime);
    const auto primary = TextOrEmpty(indexed_primary
        ? runtime.IndexedText(item, 9u)
        : runtime.ReadDword(item + primary_offset), runtime);
    return {primary, secondary, runtime.SharedEffect(), extra, trailing};
}

PlayerEquipmentVisuals IndexedVisuals(
    std::uint32_t item, IPlayerManagerSlotEquipmentRuntime& runtime) {
    // The four helper calls occur in the order 6, 3, 5, 4 in IDA.
    const auto trailing = TextOrEmpty(runtime.IndexedText(item, 6u), runtime);
    const auto extra = TextOrEmpty(runtime.IndexedText(item, 3u), runtime);
    const auto secondary = TextOrEmpty(
        runtime.IndexedText(item, 5u), runtime);
    const auto primary = TextOrEmpty(runtime.IndexedText(item, 4u), runtime);
    return {primary, secondary, runtime.SharedEffect(), extra, trailing};
}

}  // namespace

std::uint32_t PlayerManagerSlotEquipment::Apply(
    std::uint32_t object, IPlayerManagerSlotEquipmentRuntime& runtime) {
    const auto key = runtime.ReadDword(object + 0x920u);
    const auto inventory = runtime.InventoryManager();
    const auto item = runtime.FindItem(inventory + 0x7cu, key, 7u);
    if (item == 0u) return 0u;

    const auto name = runtime.CreateName(runtime.ReadDword(object + 4u));
    try {
        if (runtime.NameEquals(name, "Player-TIANHE")) {
            runtime.ApplyVisuals(object,
                CommonVisuals(item, 0xdcu, false, runtime));
        }
        if (runtime.NameEquals(name, "Player-LINSHA")) {
            const auto alternate = runtime.ReadDword(item + 0x120u);
            runtime.ApplyVisuals(object,
                CommonVisuals(item,
                              alternate == 0u ? 0xdcu : 0x11cu,
                              false, runtime));
            const auto weapon_index = runtime.ReadDword(item + 0x130u);
            runtime.ApplyWeapon(object,
                CommonVisuals(item, 0xdcu, weapon_index != 0u, runtime));
        }
        if (runtime.NameEquals(name, "Player-MENGLI"))
            runtime.ApplyVisuals(object, IndexedVisuals(item, runtime));
        if (runtime.NameEquals(name, "Player-ZIYING"))
            runtime.ApplyWeapon(object, IndexedVisuals(item, runtime));
    } catch (...) {
        static_cast<void>(runtime.CleanupName(name));
        throw;
    }
    return runtime.CleanupName(name);
}

}  // namespace pal4
