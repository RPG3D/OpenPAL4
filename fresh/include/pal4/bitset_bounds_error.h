#pragma once

#include <stdexcept>

namespace pal4 {

class BitsetBoundsError final {
public:
    // IDA 0x5c3ef0 and equivalent 0x5cec30.
    [[noreturn]] static void ThrowInvalidCharacter() {
        throw std::invalid_argument("invalid bitset<N> char");
    }
    // IDA 0x5c41a0 (also reached from 0x5c3e90).
    [[noreturn]] static void ThrowInvalidPosition() {
        throw std::out_of_range("invalid bitset<N> position");
    }
};

}  // namespace pal4
