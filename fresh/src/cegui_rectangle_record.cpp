#include "pal4/cegui_rectangle_queue.h"

namespace pal4 {

std::uint32_t CeguiRectangleRecord::ReadDword(
    const std::uint32_t offset) const noexcept {
    std::uint32_t value = 0u;
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        value |= static_cast<std::uint32_t>(bytes_[offset + index])
                 << (8u * index);
    }
    return value;
}

void CeguiRectangleRecord::WriteDword(
    const std::uint32_t offset, const std::uint32_t value) noexcept {
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        bytes_[offset + index] =
            static_cast<std::uint8_t>(value >> (8u * index));
    }
}

}  // namespace pal4
