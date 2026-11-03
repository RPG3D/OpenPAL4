#pragma once
#include <bit>
#include <cstdint>

namespace pal4::test {
// Independent execution of61D23C..61D25A and61D84A..61D86C.
// Supplied samples only; restore the caller's x87 control word.
inline float EffectDurationPc24(std::int32_t sample,float initial,float bias) {
    const float negative=-bias,scale=std::bit_cast<float>(0x38000100u);
#if defined(_MSC_VER) && defined(_M_IX86)
    unsigned short saved{},pc24{};
    __asm fnstcw saved
    pc24=static_cast<unsigned short>(saved & ~0x0300u);
    float result{};
    __asm {
        fldcw pc24
        fld bias
        fsub negative
        fild sample
        fmulp st(1), st
        fmul scale
        fadd negative
        fadd initial
        fstp result
        fldcw saved
    }
    return result;
#else
    const float width=bias-negative;
    const float product=static_cast<float>(sample)*width;
    const float fraction=product*scale;
    const float adjustment=fraction+negative;
    return adjustment+initial;
#endif
}
}
