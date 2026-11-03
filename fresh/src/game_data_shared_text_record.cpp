#include "pal4/game_data_shared_text_record.h"

namespace pal4 {
namespace {

std::uint32_t ReleaseAt(std::uint32_t object, std::uint32_t offset,
                        IGameDataSharedTextRuntime& runtime) {
    const std::uint32_t data = runtime.ReadWord(object + offset);
    std::uint32_t result = 0;
    if (data != 0) {
        const std::uint32_t header = data - 1;
        const std::uint8_t references = runtime.ReadByte(header);
        result = header;
        if (references == 0 || references == 0xff) {
            result = runtime.FreeMemory(header);
        } else {
            runtime.WriteByte(header, static_cast<std::uint8_t>(references - 1));
        }
    }
    runtime.WriteWord(object + offset, 0);
    runtime.WriteWord(object + offset + 4, 0);
    runtime.WriteWord(object + offset + 8, 0);
    runtime.WriteWord(object, 0x842bd4);
    return result;
}

}  // namespace

std::uint32_t GameDataSharedTextRecord::ReleaseFirst(
    std::uint32_t object, IGameDataSharedTextRuntime& runtime) {
    return ReleaseAt(object, 12, runtime);
}

std::uint32_t GameDataSharedTextRecord::ReleaseSecond(
    std::uint32_t object, IGameDataSharedTextRuntime& runtime) {
    return ReleaseAt(object, 20, runtime);
}

}  // namespace pal4
