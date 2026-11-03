#include "pal4/dialog_bitset256.h"
#include "pal4/bitset_bounds_error.h"

namespace pal4 {

DialogBitset256::Words& DialogBitset256::SetBit(
    Words& words, const std::uint32_t position, const bool value) {
    if (position >= 256) BitsetBoundsError::ThrowInvalidPosition();
    const auto mask = 1u << (position & 31u);
    auto& word = words[position >> 5u];
    word = value ? word | mask : word & ~mask;
    return words;
}

int DialogBitset256::Fill(Words& words, const std::uint32_t value) noexcept {
    for (int index = 7; index >= 0; --index)
        words[static_cast<std::size_t>(index)] = value;
    return -1;
}

}  // namespace pal4
