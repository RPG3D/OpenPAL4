#include "pal4/dialog_bitset256.h"

#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

int main() {
    try {
        pal4::AtlNilStringManagerSlot slot;
        if (slot.Manager() != 0 || slot.SetManager(0x12345678) != 0x12345678 ||
            slot.Manager() != 0x12345678 || slot.SetManager(0) != 0)
            throw std::runtime_error("ATL manager slot write and return");

        pal4::DialogBitset256::Words words{};
        if (&pal4::DialogBitset256::SetBit(words, 0, true) != &words ||
            &pal4::DialogBitset256::SetBit(words, 255, true) != &words ||
            words[0] != 1 || words[7] != 0x80000000)
            throw std::runtime_error("edge bits and destination identity");
        static_cast<void>(pal4::DialogBitset256::SetBit(words, 32, true));
        static_cast<void>(pal4::DialogBitset256::SetBit(words, 255, false));
        if (words[1] != 1 || words[7] != 0)
            throw std::runtime_error("word boundary and clearing");
        for (const auto position : {256u, 0xffffffffu}) {
            try {
                static_cast<void>(pal4::DialogBitset256::SetBit(
                    words, position, true));
                throw std::runtime_error("missing position error");
            } catch (const std::out_of_range& error) {
                if (std::string(error.what()) != "invalid bitset<N> position")
                    throw;
            }
        }
        if (pal4::DialogBitset256::Fill(words, 0xa5a5a5a5) != -1 ||
            words != pal4::DialogBitset256::Words{
                         0xa5a5a5a5, 0xa5a5a5a5, 0xa5a5a5a5, 0xa5a5a5a5,
                         0xa5a5a5a5, 0xa5a5a5a5, 0xa5a5a5a5, 0xa5a5a5a5})
            throw std::runtime_error("reverse eight-word fill and return");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
