#include "pal4/pal_game_object_text_record.h"

namespace pal4 {

void PalGameObjectTextRecord::InitializeAndCopy(
    const std::uint32_t destination, const std::uint32_t source,
    IUiTextBufferRecordRuntime& runtime) {
    UiTextBufferRecord::InitializeAndCopy(destination, source, runtime);
}

std::uint32_t PalGameObjectTextRecord::Delete(
    const std::uint32_t record, const std::uint8_t flags,
    IPalGameObjectTextRecordRuntime& runtime) {
    runtime.ReleaseText(record);
    if ((flags & 1u) != 0u) runtime.FreeMemory(record);
    return record;
}

}  // namespace pal4
