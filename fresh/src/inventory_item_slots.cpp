#include "pal4/inventory_item_slots.h"

#include <cstddef>

namespace pal4 {

std::uint32_t InventoryItemSlotsRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }
std::uint32_t InventoryItemSlotsRuntimeStub::GetGameDatabase() {
    return 0u;
}
std::uint32_t InventoryItemSlotsRuntimeStub::ProcessGameDatabaseArray(
    std::uint32_t, std::uint32_t) { return 5u; }
std::uint32_t InventoryItemSlotsRuntimeStub::GetInventory() {
    return 0u;
}
std::uint32_t InventoryItemSlotsRuntimeStub::FindInventoryEntry(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0u; }
std::uint32_t InventoryItemSlotsRuntimeStub::ProcessEntry(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0u; }
void InventoryItemSlotsRuntimeStub::InsertEntry(
    std::uint32_t, std::uint32_t) {}

std::uint32_t InventoryItemSlots::Initialize(
    InventoryItemSlotsState& state) {
    state.ids.fill(0u);
    return 0u;
}

std::uint32_t InventoryItemSlots::Classify(
    const InventoryItemSlotsState& state, std::uint32_t record_address,
    IInventoryItemSlotsRuntime& runtime) {
    const auto value = runtime.ReadDword(record_address + 0x94u);
    const auto database = runtime.GetGameDatabase();
    const auto category = runtime.ProcessGameDatabaseArray(
        database + 0x58u, value);
    if (category < 4u)
        return category;
    if (category == 4u) {
        if (state.ids[4] == 0u)
            return 4u;
        return state.ids[5] == 0u ? 5u : 4u;
    }
    return record_address;
}

std::uint32_t InventoryItemSlots::FindSlotOrClassify(
    const InventoryItemSlotsState& state, std::uint32_t record_address,
    IInventoryItemSlotsRuntime& runtime) {
    const auto id = runtime.ReadDword(record_address + 0x190u);
    for (std::uint32_t i = 0u; i < state.ids.size(); ++i) {
        if (state.ids[i] == id)
            return i;
    }
    return Classify(state, record_address, runtime);
}

bool InventoryItemSlots::Contains(
    const InventoryItemSlotsState& state, std::uint32_t id) {
    for (const auto candidate : state.ids) {
        if (candidate == id)
            return true;
    }
    return false;
}

std::uint32_t InventoryItemSlots::GetIndexed(
    const InventoryItemSlotsState& state, std::int32_t index) {
    if (index < 0 || index >= 6)
        return static_cast<std::uint32_t>(index);
    return state.ids[static_cast<std::size_t>(index)];
}

std::uint32_t InventoryItemSlots::SetIndexed(
    InventoryItemSlotsState& state, std::int32_t index,
    std::uint32_t value) {
    if (index < 0 || index >= 6)
        return static_cast<std::uint32_t>(index);
    state.ids[static_cast<std::size_t>(index)] = value;
    return value;
}

std::uint32_t InventoryItemSlots::ProcessEach(
    const InventoryItemSlotsState& state, std::uint32_t first_argument,
    std::uint32_t second_argument,
    IInventoryItemSlotsRuntime& runtime) {
    std::uint32_t result = 0u;
    for (const auto id : state.ids) {
        const auto inventory = runtime.GetInventory();
        const auto entry = runtime.FindInventoryEntry(
            inventory + 0x7cu, id, 7u);
        result = runtime.ProcessEntry(
            first_argument, second_argument, entry);
    }
    return result;
}

void InventoryItemSlots::ProcessAndInsertEach(
    const InventoryItemSlotsState& state, std::uint32_t destination,
    IInventoryItemSlotsRuntime& runtime) {
    for (const auto id : state.ids) {
        const auto inventory = runtime.GetInventory();
        const auto entry = runtime.FindInventoryEntry(
            inventory + 0x7cu, id, 7u);
        runtime.InsertEntry(destination, entry);
    }
}

}  // namespace pal4
