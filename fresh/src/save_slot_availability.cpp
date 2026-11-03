#include "pal4/save_slot_availability.h"

#include <bit>

namespace pal4 {

bool SaveSlotAvailability::IsAvailable(
    const std::uint32_t record, ISaveSlotAvailabilityRuntime& runtime) {
    return std::bit_cast<std::int32_t>(runtime.ReadWord(record + 404u)) > 0;
}

std::uint32_t SaveSlotAvailability::CountAvailable(
    ISaveSlotAvailabilityRuntime& runtime) {
    const auto collection = runtime.ReadWord(runtime.GameDatabase() + 168u);
    std::uint32_t count = 0u;
    std::uint32_t index = 0u;
    while (true) {
        const auto begin = runtime.ReadWord(collection + 96u);
        if (begin == 0u) {
            break;
        }
        const auto end = runtime.ReadWord(collection + 100u);
        if (end < begin || index >= (end - begin) / 596u) {
            break;
        }
        if (IsAvailable(begin + index * 596u, runtime)) {
            ++count;
        }
        ++index;
    }
    return count;
}

}  // namespace pal4
