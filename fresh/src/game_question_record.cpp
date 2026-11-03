#include "pal4/game_question_record.h"

namespace pal4 {
namespace {

std::uint32_t ReleaseText(std::uint32_t slot,
                          IGameQuestionRecordRuntime& runtime) {
    const auto data = runtime.ReadWord(slot);
    std::uint32_t result = 0;
    if (data) {
        const auto header = data - 1;
        const auto references = runtime.ReadByte(header);
        if (references == 0 || references == 0xff)
            result = runtime.Free(header);
        else {
            runtime.WriteByte(header,
                              static_cast<std::uint8_t>(references - 1));
            result = header;
        }
    }
    runtime.WriteWord(slot, 0);
    runtime.WriteWord(slot + 4, 0);
    runtime.WriteWord(slot + 8, 0);
    return result;
}

}  // namespace

std::uint32_t GameQuestionRecord::Destroy(
    std::uint32_t record, IGameQuestionRecordRuntime& runtime) {
    static_cast<void>(ReleaseText(record + 56, runtime));
    static_cast<void>(ReleaseText(record + 40, runtime));
    static_cast<void>(ReleaseText(record + 24, runtime));
    return ReleaseText(record + 8, runtime);
}

}  // namespace pal4
