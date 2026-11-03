#pragma once
#include <array>
#include <bit>
#include <cstdint>

#if defined(_MSC_VER) && defined(_M_IX86)
namespace pal4::test {
// Independent execution of the IDA64748C..6474AA arithmetic. Restore the
// caller's CW before returning; these fixtures never call a target game's CRT.
inline float ParticleRangePc24(std::int32_t sample, float center, float radius) {
    unsigned short saved{}, pc24{};
    __asm fnstcw saved
    pc24 = static_cast<unsigned short>(saved & ~0x0300u);
    const float negative = -radius;
    const float scale = std::bit_cast<float>(0x38000100u);
    float result{};
    __asm {
        fldcw pc24
        fild sample
        fld radius
        fsub negative
        fmulp st(1), st
        fmul scale
        fadd center
        fadd negative
        fstp result
        fldcw saved
    }
    return result;
}
struct ParticleSphereCandidate {
    std::array<float,3> position{};
    float squared{};
    bool accepted{};
};
// Independent IDA6478FB..64799C, excluding CRT calls and the retry branch.
// The retained Z stays on the x87 stack through Z^2+Y^2+X^2 comparison.
inline ParticleSphereCandidate ParticleSpherePc24(
    std::int32_t x_sample,std::int32_t y_sample,std::int32_t z_sample,float diameter) {
    unsigned short saved{},pc24{},status{};
    __asm fnstcw saved
    pc24=static_cast<unsigned short>(saved & ~0x0300u);
    const float scale=std::bit_cast<float>(0x38800100u),one=1.f,half=0.5f;
    float x{},y{},px{},py{},pz{},squared{};
    __asm {
        fldcw pc24
        fild x_sample
        fmul scale
        fsub one
        fstp x
        fild y_sample
        fmul scale
        fsub one
        fstp y
        fild z_sample
        fmul scale
        fsub one
        fld st
        fmul st, st(1)
        fld y
        fmul y
        faddp st(1), st
        fld x
        fmul x
        faddp st(1), st
        fst squared
        fcomp one
        fnstsw status
        fld x
        fmul diameter
        fmul half
        fstp px
        fld y
        fmul diameter
        fmul half
        fstp py
        fmul diameter
        fmul half
        fstp pz
        fldcw saved
    }
    const auto comparison=status & 0x4100u;
    return {{px,py,pz},squared,comparison==0x0100u || comparison==0x4000u};
}
}
#endif
