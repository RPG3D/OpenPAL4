#include "pal4/game_data_equip_record_copy.h"

namespace pal4 {
namespace {

void CopyWords(std::uint32_t destination, std::uint32_t source,
               std::uint32_t first, std::uint32_t last,
               IGameDataEquipRecordCopyRuntime& runtime) {
    for (std::uint32_t i = first; i < last; ++i)
        runtime.WriteWord(destination + i * 4,
                          runtime.ReadWord(source + i * 4));
}

}  // namespace

std::uint32_t GameDataEquipRecordCopy::Construct(
    std::uint32_t destination, std::uint32_t source,
    IGameDataEquipRecordCopyRuntime& runtime) {
    if (!destination) return 0;
    CopyWords(destination, source, 0, 38, runtime);
    for (std::uint32_t i = 0; i < 13; ++i) {
        const auto offset = 152 + i * 16;
        runtime.WriteByte(destination + offset,
                          runtime.ReadByte(source + offset));
        runtime.CopyConstructText(destination + offset,
                                  source + offset);
    }
    CopyWords(destination, source, 90, 100, runtime);
    return source;
}

std::uint32_t GameDataEquipRecordCopy::Assign(
    std::uint32_t destination, std::uint32_t source,
    IGameDataEquipRecordCopyRuntime& runtime) {
    CopyWords(destination, source, 0, 38, runtime);
    for (std::uint32_t i = 0; i < 13; ++i) {
        const auto offset = 152 + i * 16;
        runtime.AssignText(destination + offset, source + offset);
    }
    CopyWords(destination, source, 90, 100, runtime);
    return destination;
}

}  // namespace pal4
