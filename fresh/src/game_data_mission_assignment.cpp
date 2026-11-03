#include "pal4/game_data_mission_assignment.h"

#include <algorithm>
#include <new>

namespace pal4 {

int GameDataMissionAssignment::Refresh(
    std::uint32_t manager, GameDataMissionPointerArray& output,
    IGameDataMissionAssignmentRuntime& runtime) {
    GameDataMissionPointerArray candidates;
    try {
        (void)GameDataMissionSelection::CollectAndSort(
            manager, candidates, runtime);

        const auto game_state = runtime.GameState();
        const auto slot_begin = runtime.ReadWord(game_state + 0x158);
        const auto slot_count = slot_begin
            ? (runtime.ReadWord(game_state + 0x15c) - slot_begin) / 4
            : 0;

        if (output.records.size() + slot_count > output.capacity) {
            const auto old_count = output.records.size();
            const auto new_capacity = old_count +
                                      std::max(old_count,
                                               static_cast<std::size_t>(slot_count));
            const auto address = runtime.AllocatePointerStorage(new_capacity);
            if (!address && new_capacity) throw std::bad_alloc();
            if (output.begin_address)
                runtime.FreePointerStorage(output.begin_address);
            output.begin_address = address;
            output.capacity = new_capacity;
        }
        output.records.insert(output.records.begin(), slot_count, 0);

        for (const auto record : candidates.records) {
            const auto current_game = runtime.GameState();
            const auto category = runtime.ReadWord(record + 8);
            const auto current_level = runtime.CurrentLevel(current_game,
                                                             category);
            const auto required = static_cast<std::int32_t>(
                runtime.ReadWord(record + 4));
            if (required > current_level || category >= slot_count) continue;
            auto& selected = output.records[category];
            if (!selected || required > static_cast<std::int32_t>(
                                            runtime.ReadWord(selected + 4)))
                selected = record;
        }
    } catch (...) {
        (void)runtime.ReleaseTemporaryStorage(candidates.begin_address);
        throw;
    }
    return runtime.ReleaseTemporaryStorage(candidates.begin_address);
}

}  // namespace pal4
