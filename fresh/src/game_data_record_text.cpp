#include "pal4/game_data_record_text.h"

#include <array>

namespace pal4 {
namespace {

std::uint32_t ReleaseField(std::uint32_t field,
                           IGameDataRecordTextRuntime& runtime) {
    const auto data = runtime.ReadWord(field);
    auto result = data;
    if (data) {
        const auto header = data - 1;
        const auto references = runtime.ReadByte(header);
        result = header;
        if (references == 0 || references == 0xff)
            result = runtime.Free(header);
        else
            runtime.WriteByte(header,
                              static_cast<std::uint8_t>(references - 1));
    }
    runtime.WriteWord(field, 0);
    runtime.WriteWord(field + 4, 0);
    runtime.WriteWord(field + 8, 0);
    return result;
}

template <std::size_t N>
std::uint32_t ReleaseFields(
    std::uint32_t record, const std::array<std::uint32_t, N>& offsets,
    IGameDataRecordTextRuntime& runtime) {
    std::uint32_t result = 0;
    for (const auto offset : offsets)
        result = ReleaseField(record + offset, runtime);
    return result;
}

}  // namespace

std::uint32_t SkillDataRecordText::Release(
    std::uint32_t record, IGameDataRecordTextRuntime& runtime) {
    return ReleaseFields(record, std::array<std::uint32_t, 4>{
        120, 104, 88, 72}, runtime);
}

std::uint32_t StuntDataRecordText::Release(
    std::uint32_t record, IGameDataRecordTextRuntime& runtime) {
    runtime.ResetNested(record + 192);
    return ReleaseFields(record, std::array<std::uint32_t, 6>{
        180, 164, 148, 44, 28, 12}, runtime);
}

std::uint32_t MagicDataRecordText::Release(
    std::uint32_t record, IGameDataRecordTextRuntime& runtime) {
    runtime.ResetNested(record + 136);
    return ReleaseFields(record, std::array<std::uint32_t, 6>{
        124, 108, 92, 48, 32, 16}, runtime);
}

}  // namespace pal4
