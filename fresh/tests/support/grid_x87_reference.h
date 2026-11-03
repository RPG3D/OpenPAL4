#pragma once
#include <array>
#include <bit>
#include <cstdint>

#if defined(_MSC_VER) && defined(_M_IX86)
namespace pal4::test {
struct GridSphereCandidate {
    std::array<float,3> coordinates{};
    float length{};
    bool accepted{};
};
// IDA65ECDC..65ED56, with three supplied draws instead of target CRT calls.
// Execute actual PC24 x87 instructions and restore the caller's control word.
inline GridSphereCandidate GridSpherePc24(
    std::int32_t y_sample, std::int32_t x_sample, std::int32_t z_sample) {
    unsigned short saved{},pc24{},status{};
    __asm fnstcw saved
    pc24=static_cast<unsigned short>(saved & ~0x0300u);
    const float scale=std::bit_cast<float>(0x38000100u),one=1.f;
    float y_fraction{},x{},y{},z{},distance_value{};
    __asm {
        fldcw pc24
        fild y_sample
        fmul scale
        fstp y_fraction
        fild x_sample
        fmul scale
        fadd st, st
        fsub one
        fstp x
        fld y_fraction
        fadd st, st
        fsub one
        fstp y
        fild z_sample
        fmul scale
        fadd st, st
        fsub one
        fstp z
        fld y
        fmul y
        fld z
        fmul z
        faddp st(1), st
        fld x
        fmul x
        faddp st(1), st
        fsqrt
        fst distance_value
        fcomp one
        fnstsw status
        fldcw saved
    }
    return {{x,y,z},distance_value,(status & 0x0100u)!=0u};
}
}
#endif
