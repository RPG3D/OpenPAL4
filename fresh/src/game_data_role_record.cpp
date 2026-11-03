#include "pal4/game_data_role_record.h"

#include <initializer_list>

namespace pal4 {

std::uint32_t GameDataRoleRecord::Initialize(
    std::uint32_t record, IGameDataRoleRecordRuntime& runtime) {
    for (std::uint32_t i = 0; i < 4; ++i)
        runtime.InitializeText(record + i * 16);
    for (std::uint32_t i = 0; i < 5; ++i)
        runtime.InitializeText(record + 96 + i * 16);
    for (std::uint32_t offset : {64u, 68u, 72u, 76u, 88u, 92u})
        runtime.WriteWord(record + offset, 0);
    runtime.WriteWord(record + 80, 4);
    return record;
}

bool GameDataRoleRecord::NormalizeAndValidate(
    std::uint32_t record, IGameDataRoleRecordRuntime& runtime) {
    const auto index = runtime.ReadWord(record + 228) - 1001u;
    runtime.WriteWord(record + 224, index);
    return index < 4u && runtime.ReadWord(record) <= 6u;
}

bool GameDataRoleRecord::HasValidLinkedRoleId(
    std::uint32_t record, IGameDataRoleRecordRuntime& runtime) {
    return runtime.ReadWord(record + 148) - 1001u < 4u;
}

}  // namespace pal4
