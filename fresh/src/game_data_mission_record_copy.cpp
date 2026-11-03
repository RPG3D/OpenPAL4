#include "pal4/game_data_mission_record_copy.h"

#include <initializer_list>

namespace pal4 {

std::uint32_t GameDataMissionRecordCopy::Construct(
    std::uint32_t destination, std::uint32_t source,
    IGameDataMissionRecordCopyRuntime& runtime) {
    if (destination == 0) return 0;
    for (std::uint32_t offset = 0; offset < 20; offset += 4)
        runtime.WriteWord(destination + offset,
                          runtime.ReadWord(source + offset));
    std::uint32_t result = 0;
    for (std::uint32_t offset : {20u, 36u, 52u}) {
        runtime.WriteByte(destination + offset,
                          runtime.ReadByte(source + offset));
        runtime.WriteWord(destination + offset + 4, 0);
        runtime.WriteWord(destination + offset + 8, 0);
        runtime.WriteWord(destination + offset + 12, 0);
        result = runtime.AssignText(destination + offset,
                                    source + offset);
    }
    return result;
}

std::uint32_t GameDataMissionRecordCopy::Assign(
    std::uint32_t destination, std::uint32_t source,
    IGameDataMissionRecordCopyRuntime& runtime) {
    for (std::uint32_t offset = 0; offset < 20; offset += 4)
        runtime.WriteWord(destination + offset,
                          runtime.ReadWord(source + offset));
    for (std::uint32_t offset : {20u, 36u, 52u})
        (void)runtime.AssignText(destination + offset,
                                 source + offset);
    return destination;
}

}  // namespace pal4
