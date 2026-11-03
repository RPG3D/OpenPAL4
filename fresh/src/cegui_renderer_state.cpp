#include "pal4/cegui_renderer.h"

#include <bit>

namespace pal4 {

std::uint8_t CeguiRendererState::ReadByte(
    const std::uint32_t offset) const noexcept {
    return bytes_[offset];
}

std::uint32_t CeguiRendererState::ReadDword(
    const std::uint32_t offset) const noexcept {
    std::uint32_t value = 0u;
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        value |= static_cast<std::uint32_t>(bytes_[offset + index])
                 << (8u * index);
    }
    return value;
}

float CeguiRendererState::ReadFloat(
    const std::uint32_t offset) const noexcept {
    return std::bit_cast<float>(ReadDword(offset));
}

void CeguiRendererState::WriteByte(
    const std::uint32_t offset, const std::uint8_t value) noexcept {
    bytes_[offset] = value;
}

void CeguiRendererState::WriteDword(
    const std::uint32_t offset, const std::uint32_t value) noexcept {
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        bytes_[offset + index] =
            static_cast<std::uint8_t>(value >> (8u * index));
    }
}

void CeguiRendererState::WriteFloat(
    const std::uint32_t offset, const float value) noexcept {
    WriteDword(offset, std::bit_cast<std::uint32_t>(value));
}

}  // namespace pal4
