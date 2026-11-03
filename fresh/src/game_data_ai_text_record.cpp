#include "pal4/game_data_ai_text_record.h"

namespace pal4 {
namespace {

std::uint32_t ReleaseText(std::uint32_t text,
                          IGameDataAiTextRecordRuntime& runtime) {
    const auto data = runtime.ReadWord(text + 4);
    std::uint32_t result = 0;
    if (data) {
        const auto header = data - 1;
        const auto references = runtime.ReadByte(header);
        if (references == 0 || references == 0xff) {
            result = runtime.Free(header);
        } else {
            runtime.WriteByte(header,
                              static_cast<std::uint8_t>(references - 1));
            result = header;
        }
    }
    runtime.WriteWord(text + 4, 0);
    runtime.WriteWord(text + 8, 0);
    runtime.WriteWord(text + 12, 0);
    return result;
}

}  // namespace

std::uint32_t GameDataAiTextRecord::ResetPointers(
    std::uint32_t record, IGameDataAiTextRecordRuntime& runtime) {
    (void)ReleaseText(record + 36, runtime);
    (void)ReleaseText(record + 20, runtime);
    return ReleaseText(record, runtime);
}

std::uint32_t GameDataAiTextRecord::Destroy(
    std::uint32_t record, std::uint8_t flags,
    IGameDataAiTextRecordRuntime& runtime) {
    (void)ResetPointers(record, runtime);
    if (flags & 1) (void)runtime.Free(record);
    return record;
}

}  // namespace pal4
