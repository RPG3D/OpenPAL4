#include "pal4/smith_window_item_quantity.h"

namespace pal4 {

std::int32_t SmithWindowItemQuantity::FirstPositive(
    const std::uint32_t item,
    ISmithWindowQuantityRuntime& runtime) {
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const auto value = static_cast<std::int32_t>(
            runtime.ReadWord(item + 0x168u + index * 4u));
        if (value > 0) return value;
    }
    return 0;
}

}  // namespace pal4
