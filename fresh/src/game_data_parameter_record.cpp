#include "pal4/game_data_parameter_record.h"

namespace pal4 {

void GameDataParameterRecordLifecycle::Destroy(
    std::uint32_t record, IGameDataParameterRecordDestroyRuntime& runtime) {
    for (std::uint32_t i = 6; i > 0; --i)
        runtime.DestroyTextRecord(record + 4 + (i - 1) * 16);
}

}  // namespace pal4
