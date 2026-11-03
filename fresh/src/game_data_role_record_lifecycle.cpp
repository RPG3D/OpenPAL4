#include "pal4/game_data_role_record_lifecycle.h"

namespace pal4 {

void GameDataRoleRecordLifecycle::Destroy(
    std::uint32_t record, IGameDataRoleRecordTextRuntime& runtime) {
    // EH vector destruction visits each array from its last element.
    for (std::uint32_t index = 5; index != 0; --index)
        runtime.DestroyText(record + 240 + (index - 1) * 16);
    for (std::uint32_t index = 4; index != 0; --index)
        runtime.DestroyText(record + 144 + (index - 1) * 16);
}

}  // namespace pal4
