#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace pal4 {

class GameDataBitsetCodec final {
public:
    // IDA 0x5c3c60: word zero contains bits 0..31; text begins at bit 127.
    [[nodiscard]] static std::string& ToString(
        const std::array<std::uint32_t, 4>& words,
        std::string& destination);
    // IDA 0x5c3e90 and 0x5c4280, respectively.
    [[nodiscard]] static std::array<std::uint32_t, 4>& SetBit(
        std::array<std::uint32_t, 4>& words,
        std::uint32_t position, bool value);
    [[nodiscard]] static std::uint32_t& SetBit32(
        std::uint32_t& word, std::uint32_t position, bool value);
};

}  // namespace pal4
