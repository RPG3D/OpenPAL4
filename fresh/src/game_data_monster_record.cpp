#include "pal4/game_data_monster_record.h"

namespace pal4 {

std::uint32_t GameDataMonsterRecord::Initialize(
    std::uint32_t base, IGameDataMonsterRecordRuntime& runtime) {
    for (std::uint32_t i = 0; i < 6; ++i)
        runtime.ConstructText(base + 72 + i * 16);
    for (std::uint32_t i = 0; i < 5; ++i)
        runtime.ConstructText(base + 168 + i * 16);
    ResetScalarAndArrayFields(base, runtime);
    return base;
}

void GameDataMonsterRecord::ResetScalarAndArrayFields(
    std::uint32_t base, IGameDataMonsterRecordRuntime& runtime) {
    runtime.WriteWord(base + 64, 0);
    runtime.WriteWord(base + 248, 0);
    runtime.WriteWord(base + 20, 0);
    runtime.WriteWord(base + 24, 0);
    runtime.WriteWord(base + 28, 0);
}

void GameDataMonsterRecord::DestroySubrecord(
    std::uint32_t base, IGameDataMonsterRecordDestroyRuntime& runtime) {
    for (std::uint32_t i = 5; i != 0; --i)
        runtime.DestroyText(base + 168 + (i - 1) * 16);
    for (std::uint32_t i = 6; i != 0; --i)
        runtime.DestroyText(base + 72 + (i - 1) * 16);
}

void GameDataMonsterRecord::DestroyFullRecord(
    std::uint32_t record, IGameDataMonsterRecordDestroyRuntime& runtime) {
    DestroySubrecord(record ? record + 336 : 0, runtime);
}

}  // namespace pal4
