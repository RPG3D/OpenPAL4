#include "pal4/player_manager_slot_snapshot.h"

#include "pal4/range_no_op_callback.h"

#include <bit>

namespace pal4 {

std::uint32_t PlayerManagerSlotSnapshotRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }
void PlayerManagerSlotSnapshotRuntimeStub::WriteDword(
    std::uint32_t, std::uint32_t) {}
void PlayerManagerSlotSnapshotRuntimeStub::CopyTripleOwner(
    PlayerManagerScriptSnapshot&, std::uint32_t) {}
void PlayerManagerSlotSnapshotRuntimeStub::CopyPairOwner(
    PlayerManagerScriptSnapshot&, std::uint32_t) {}
void PlayerManagerSlotSnapshotRuntimeStub::ProcessInventory(
    std::uint32_t, PlayerManagerScriptSnapshot&) {}
void PlayerManagerSlotSnapshotRuntimeStub::ProcessSkills(
    PlayerManagerScriptSnapshot&, std::uint32_t) {}
std::uint32_t PlayerManagerSlotSnapshotRuntimeStub::GetElementByIndex(
    const PlayerManagerScriptSnapshot& snapshot, std::uint32_t index) {
    return index < 8u
        ? snapshot.words[PlayerManagerScriptSnapshot::kSecondRecord + index + 1u]
        : index;
}
void PlayerManagerSlotSnapshotRuntimeStub::SetProperty(
    PlayerManagerScriptSnapshot&, std::uint32_t,
    std::uint32_t) {}
void PlayerManagerSlotSnapshotRuntimeStub::Free(std::uint32_t) {}

namespace {

constexpr std::uint32_t kBaseVtable = 0x843270u;
constexpr std::uint32_t kScriptVtable = 0x8430e8u;

void ClearOwner(PlayerManagerScriptSnapshot& snapshot,
                std::uint32_t owner,
                IPlayerManagerSlotSnapshotRuntime& runtime) {
    runtime.Free(snapshot.words[owner + 1u]);
    snapshot.words[owner + 1u] = 0u;
    snapshot.words[owner + 2u] = 0u;
    snapshot.words[owner + 3u] = 0u;
}

void Unwind(PlayerManagerScriptSnapshot& snapshot, int state,
            IPlayerManagerSlotSnapshotRuntime& runtime) {
    if (state == 2) {
        snapshot.words[0] = kScriptVtable;
        ClearOwner(snapshot, PlayerManagerScriptSnapshot::kPairOwner,
                   runtime);
        ClearOwner(snapshot, PlayerManagerScriptSnapshot::kTripleOwner,
                   runtime);
    }
    if (state == 1 || state == 4)
        ClearOwner(snapshot, PlayerManagerScriptSnapshot::kTripleOwner,
                   runtime);
    snapshot.words[0] = kBaseVtable;
}

}  // namespace

std::uint32_t PlayerManagerSlotSnapshot::Build(
    std::uint32_t object, std::uint32_t destination,
    std::uint32_t inventory_entries,
    IPlayerManagerSlotSnapshotRuntime& runtime) {
    PlayerManagerScriptSnapshot snapshot;
    const auto script = object == 0u ? 0u : object + 0x654u;
    for (std::uint32_t index = 0u;
         index < PlayerManagerScriptSnapshot::kRecordWords; ++index)
        snapshot.words[PlayerManagerScriptSnapshot::kFirstRecord + index] =
            runtime.ReadDword(script + 4u + index * 4u);
    for (std::uint32_t index = 0u;
         index < PlayerManagerScriptSnapshot::kRecordWords; ++index)
        snapshot.words[PlayerManagerScriptSnapshot::kSecondRecord + index] =
            runtime.ReadDword(script + 0x154u + index * 4u);
    snapshot.words[0] = kBaseVtable;

    int unwind_state = 0;
    try {
        runtime.CopyTripleOwner(snapshot, script + 0x2a4u);
        unwind_state = 1;
        runtime.CopyPairOwner(snapshot, script + 0x2b4u);
        snapshot.words[177] = runtime.ReadDword(script + 0x2c4u);
        snapshot.words[178] = runtime.ReadDword(script + 0x2c8u);
        snapshot.words[0] = kScriptVtable;

        const auto property9 = runtime.ReadDword(object + 0x898u);
        const auto property8 = runtime.ReadDword(object + 0x894u);
        const auto property7 = runtime.ReadDword(object + 0x890u);
        for (std::uint32_t index = 0u;
             index < PlayerManagerScriptSnapshot::kRecordWords; ++index)
            snapshot.words[PlayerManagerScriptSnapshot::kSecondRecord + index] =
                runtime.ReadDword(object + 0x658u + index * 4u);

        unwind_state = 2;
        runtime.ProcessInventory(inventory_entries, snapshot);
        runtime.ProcessSkills(snapshot, 1u);
        auto& current = snapshot.words[
            PlayerManagerScriptSnapshot::kSecondRecord + 58u];
        current = property7;
        const auto first_limit = runtime.GetElementByIndex(snapshot, 0u);
        if (std::bit_cast<std::int32_t>(current) >
            std::bit_cast<std::int32_t>(first_limit))
            current = runtime.GetElementByIndex(snapshot, 0u);
        else if (std::bit_cast<std::int32_t>(current) < 0)
            current = 0u;
        runtime.SetProperty(snapshot, 9u, property9);
        runtime.SetProperty(snapshot, 8u, property8);
        for (std::uint32_t index = 0u;
             index < PlayerManagerScriptSnapshot::kRecordWords; ++index)
            runtime.WriteDword(destination + index * 4u,
                snapshot.words[PlayerManagerScriptSnapshot::kSecondRecord + index]);

        unwind_state = 4;
        RangeNoOpCallback::Invoke(
            snapshot.words[PlayerManagerScriptSnapshot::kPairOwner + 1u],
            snapshot.words[PlayerManagerScriptSnapshot::kPairOwner + 2u]);
        ClearOwner(snapshot, PlayerManagerScriptSnapshot::kPairOwner,
                   runtime);
        unwind_state = 3;
        ClearOwner(snapshot, PlayerManagerScriptSnapshot::kTripleOwner,
                   runtime);
        unwind_state = -1;
        snapshot.words[0] = kBaseVtable;
        return destination;
    } catch (...) {
        Unwind(snapshot, unwind_state, runtime);
        throw;
    }
}

}  // namespace pal4
