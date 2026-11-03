#include "pal4/game_data_bitset_codec.h"
#include "pal4/bitset_bounds_error.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

int main() {
    try {
        std::string text = "old content";
        const std::array<std::uint32_t, 4> zero{};
        if (&pal4::GameDataBitsetCodec::ToString(zero, text) != &text ||
            text != std::string(128u, '0'))
            throw std::runtime_error("zero bitset and destination identity");
        const std::array<std::uint32_t, 4> edge{
            0x80000001u, 0x00000001u, 0x80000001u, 0x80000000u};
        (void)pal4::GameDataBitsetCodec::ToString(edge, text);
        if (text.size() != 128u || text[0] != '1' || text[32] != '1' ||
            text[63] != '1' || text[95] != '1' || text[96] != '1' ||
            text[127] != '1' || text[1] != '0')
            throw std::runtime_error("highest, lowest and word-boundary bits");
        const std::array<std::uint32_t, 4> full{
            0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu};
        (void)pal4::GameDataBitsetCodec::ToString(full, text);
        if (text != std::string(128u, '1'))
            throw std::runtime_error("full bitset");
        std::array<std::uint32_t, 4> words{};
        if (&pal4::GameDataBitsetCodec::SetBit(words, 0u, true) != &words ||
            &pal4::GameDataBitsetCodec::SetBit(words, 127u, true) != &words ||
            words[0] != 1u || words[3] != 0x80000000u)
            throw std::runtime_error("128-bit setter boundary and identity");
        (void)pal4::GameDataBitsetCodec::SetBit(words, 0u, false);
        (void)pal4::GameDataBitsetCodec::SetBit(words, 32u, true);
        if (words[0] != 0u || words[1] != 1u)
            throw std::runtime_error("128-bit setter clears and crosses word");
        std::uint32_t word = 0u;
        if (&pal4::GameDataBitsetCodec::SetBit32(word, 31u, true) != &word ||
            word != 0x80000000u)
            throw std::runtime_error("32-bit setter high bit and identity");
        (void)pal4::GameDataBitsetCodec::SetBit32(word, 31u, false);
        if (word != 0u) throw std::runtime_error("32-bit setter clear");
        for (const auto index : {32u, 128u, 0xffffffffu}) {
            try {
                if (index == 32u)
                    (void)pal4::GameDataBitsetCodec::SetBit32(word, index, true);
                else
                    (void)pal4::GameDataBitsetCodec::SetBit(words, index, true);
                throw std::runtime_error("expected bitset range failure");
            } catch (const std::out_of_range& error) {
                if (std::string(error.what()) != "invalid bitset<N> position")
                    throw;
            }
        }
        try {
            pal4::BitsetBoundsError::ThrowInvalidCharacter();
        } catch (const std::invalid_argument& error) {
            if (std::string(error.what()) != "invalid bitset<N> char")
                throw;
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
