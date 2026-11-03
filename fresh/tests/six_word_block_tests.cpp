#include "pal4/six_word_block.h"

#include <array>
#include <cstdlib>
#include <iostream>

int main() {
    pal4::SixWordBlockState state;
    state.words.fill(0xffffffffu);
    if (pal4::SixWordBlock::Initialize(state, 0x1234u) != 0x1234u ||
        state.words != std::array<std::uint32_t, 6>{}) {
        std::cerr << "FAIL: six-word initializer clears all 24 bytes\n";
        return EXIT_FAILURE;
    }
    std::cout << "1 six-word block case passed\n";
}
