#include "pal4/inventory_entry_processor.h"

#include <bit>

namespace pal4 {

std::uint32_t InventoryEntryRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }
std::uint32_t InventoryEntryRuntimeStub::UpdateStructMembers(
    std::uint32_t, std::uint32_t) { return 0u; }
std::uint32_t InventoryEntryInsertRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }
std::uint32_t InventoryEntryInsertRuntimeStub::GetGameDatabase() {
    return 0u;
}
bool InventoryEntryInsertRuntimeStub::ContainsDatabaseKey(
    std::uint32_t, std::uint32_t) { return false; }
std::uint32_t InventoryEntryInsertRuntimeStub::LookupDatabaseKey(
    std::uint32_t, std::uint32_t) { return 0u; }
std::uint32_t InventoryEntryInsertRuntimeStub::Allocate(
    std::uint32_t) { return 0u; }
void InventoryEntryInsertRuntimeStub::CopyRange(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
void InventoryEntryInsertRuntimeStub::FillOne(
    std::uint32_t, std::uint32_t) {}
void InventoryEntryInsertRuntimeStub::Free(std::uint32_t) {}

namespace {

std::int32_t DwordCount(std::uint32_t begin, std::uint32_t end) {
    return std::bit_cast<std::int32_t>(end - begin) >> 2;
}

}  // namespace

std::uint32_t InventoryEntryProcessor::ProcessConditional(
    std::uint32_t context, std::uint32_t,
    std::uint32_t entry, IInventoryEntryRuntime& runtime) {
    if (entry == 0u)
        return 0u;
    return runtime.UpdateStructMembers(context, entry);
}

std::uint32_t InventoryEntryProcessor::ProcessArray(
    InventoryEntryArrayState& state, std::uint32_t context,
    std::uint32_t unused_argument, IInventoryEntryRuntime& runtime) {
    auto cursor = state.begin;
    while (cursor != state.end) {
        const auto entry = runtime.ReadDword(cursor);
        (void)ProcessConditional(context, unused_argument, entry, runtime);
        cursor += 4u;
    }
    return state.end;
}

void InventoryEntryProcessor::ProcessAndInsert(
    InventoryEntryArrayState& state, std::uint32_t inventory_entry,
    IInventoryEntryInsertRuntime& runtime) {
    if (inventory_entry == 0u)
        return;
    const auto first_key = runtime.ReadDword(inventory_entry + 0x180u);
    const auto first_database = runtime.GetGameDatabase();
    if (!runtime.ContainsDatabaseKey(first_database + 0x0cu, first_key))
        return;
    const auto second_key = runtime.ReadDword(inventory_entry + 0x180u);
    const auto second_database = runtime.GetGameDatabase();
    const auto match = runtime.LookupDatabaseKey(
        second_database + 0x0cu, second_key);
    if (runtime.ReadDword(match + 12u) != 0xffffffffu)
        return;
    const auto value = runtime.ReadDword(match);

    const auto old_end = state.end;
    if (static_cast<std::uint32_t>(DwordCount(old_end, state.capacity)) >= 1u) {
        runtime.CopyRange(old_end, old_end, old_end + 4u);
        runtime.FillOne(old_end, value);
        state.end = old_end + 4u;
        return;
    }

    const auto old_begin = state.begin;
    const auto old_count = old_begin == 0u ? 0 : DwordCount(old_begin, old_end);
    const auto growth = old_begin != 0u &&
        static_cast<std::uint32_t>(old_count) > 1u ? old_count : 1;
    const auto desired_words = std::bit_cast<std::int32_t>(
        static_cast<std::uint32_t>(old_count) +
        static_cast<std::uint32_t>(growth));
    const auto allocation_words = desired_words < 0 ? 0u
        : static_cast<std::uint32_t>(desired_words);
    const auto allocation = runtime.Allocate(allocation_words * 4u);
    runtime.CopyRange(old_begin, old_end, allocation);
    const auto insertion = allocation + (old_end - old_begin);
    runtime.FillOne(insertion, value);
    runtime.CopyRange(old_end, old_end, insertion + 4u);
    runtime.Free(old_begin);
    state.capacity = allocation +
        static_cast<std::uint32_t>(desired_words) * 4u;
    state.begin = allocation;
    state.end = allocation +
        (static_cast<std::uint32_t>(old_count) + 1u) * 4u;
}

}  // namespace pal4
