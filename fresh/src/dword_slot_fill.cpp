#include "pal4/dword_slot_fill.h"

namespace pal4 {

std::uint32_t DwordSlotFill::Fill(
    const std::uint32_t destination, const std::uint32_t count,
    const std::uint32_t value, ISpatialGridRuntime& runtime) {
    if (count == 0u)
        return 0u;
    if (destination == 0u)
        return 4u * count;
    for (std::uint32_t index = 0u; index < count; ++index)
        runtime.WriteWord(destination + 4u * index, value);
    return destination + 4u * count;
}

}  // namespace pal4
