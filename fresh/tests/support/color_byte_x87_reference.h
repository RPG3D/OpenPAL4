#pragma once
#include <bit>
#include <cstdint>
namespace pal4::test {
// Independent IDA61CFF5/61D1A6 FILD -> FMUL float84258C -> FSTP.
// No game RNG or game memory is touched; restore the calling x87 control word.
inline float ColorBytePc24(std::int32_t channel) {
    const float scale=std::bit_cast<float>(0x3b808081u);
#if defined(_MSC_VER) && defined(_M_IX86)
    unsigned short saved{},pc24{};
    __asm fnstcw saved
    pc24=static_cast<unsigned short>(saved & ~0x0300u);
    float result{};
    __asm {
        fldcw pc24
        fild channel
        fmul scale
        fstp result
        fldcw saved
    }
    return result;
#else
    return static_cast<float>(channel)*scale;
#endif
}
}
