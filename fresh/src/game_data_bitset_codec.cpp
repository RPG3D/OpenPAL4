#include "pal4/game_data_bitset_codec.h"
#include "pal4/bitset_bounds_error.h"

namespace pal4 {

std::string& GameDataBitsetCodec::ToString(
    const std::array<std::uint32_t, 4>& words,
    std::string& destination) {
    destination.resize(128u);
    for (std::uint32_t bit = 0u; bit < 128u; ++bit)
        destination[127u - bit] =
            (words[bit / 32u] & (1u << (bit % 32u))) != 0u ? '1' : '0';
    return destination;
}

std::array<std::uint32_t, 4>& GameDataBitsetCodec::SetBit(
    std::array<std::uint32_t, 4>& words,
    const std::uint32_t position, const bool value) {
    if (position >= 128u) BitsetBoundsError::ThrowInvalidPosition();
    const auto mask = 1u << (position & 31u);
    auto& word = words[position >> 5u];
    word = value ? word | mask : word & ~mask;
    return words;
}

std::uint32_t& GameDataBitsetCodec::SetBit32(
    std::uint32_t& word, const std::uint32_t position, const bool value) {
    if (position >= 32u) BitsetBoundsError::ThrowInvalidPosition();
    const auto mask = 1u << (position & 31u);
    word = value ? word | mask : word & ~mask;
    return word;
}

}  // namespace pal4
