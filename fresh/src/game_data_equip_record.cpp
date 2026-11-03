#include "pal4/game_data_equip_record.h"

namespace pal4 {

void GameDataEquipRecordLifecycle::Destroy(
    std::uint32_t record, IGameDataEquipRecordRuntime& runtime) {
    for (std::uint32_t i = 13; i != 0; --i)
        runtime.DestroyTextRecord(record + 152 + (i - 1) * 16);
}

void GameDataEquipRecordLifecycle::DestroyRange(
    std::uint32_t first, std::uint32_t last,
    IGameDataEquipRecordRuntime& runtime) {
    for (std::uint32_t record = first; record != last; record += 400)
        Destroy(record, runtime);
}

}  // namespace pal4
