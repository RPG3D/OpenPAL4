#pragma once
#include "pal4/behavior_path_endpoint_helper.h"

#if defined(_MSC_VER) && defined(_M_IX86)
namespace pal4::test {
inline float MovementVectorDotPc24(const BehaviorVector3& first,
                                  const BehaviorVector3& second) {
    unsigned short saved{},pc24{};
    __asm fnstcw saved
    pc24=static_cast<unsigned short>(saved & ~0x0300u);
    const auto* a=first.data();const auto* b=second.data();float result{};
    __asm {
        fldcw pc24
        mov eax,a
        mov edx,b
        fld dword ptr[eax+8]
        fmul dword ptr[edx+8]
        fld dword ptr[eax+4]
        fmul dword ptr[edx+4]
        faddp st(1),st
        fld dword ptr[eax]
        fmul dword ptr[edx]
        faddp st(1),st
        fstp result
        fldcw saved
    }
    return result;
}
// CalculateDotProduct40BF24..40BF36 followed by original FSQRT.
inline float MovementVectorLengthPc24(const BehaviorVector3& input) {
    unsigned short saved{},pc24{};
    __asm fnstcw saved
    pc24=static_cast<unsigned short>(saved & ~0x0300u);
    const auto* p=input.data();float result{};
    __asm {
        fldcw pc24
        mov eax,p
        fld dword ptr[eax+8]
        fmul dword ptr[eax+8]
        fld dword ptr[eax+4]
        fmul dword ptr[eax+4]
        faddp st(1),st
        fld dword ptr[eax]
        fmul dword ptr[eax]
        faddp st(1),st
        fsqrt
        fstp result
        fldcw saved
    }
    return result;
}
// Original42D002..42D032. Keep subtraction, Z^2+X^2+Y^2, and FSQRT
// under PC24; these are game vector instructions, not a RenderWare call.
inline float RandomMovementDistancePc24(const BehaviorVector3& position,
                                      const BehaviorVector3& target) {
    unsigned short saved{},pc24{};
    __asm fnstcw saved
    pc24=static_cast<unsigned short>(saved & ~0x0300u);
    const auto* p=position.data();const auto* t=target.data();float result{};
    __asm {
        fldcw pc24
        mov eax,p
        mov edx,t
        fld dword ptr[eax]
        fsub dword ptr[edx]
        fld dword ptr[eax+4]
        fsub dword ptr[edx+4]
        fld dword ptr[eax+8]
        fsub dword ptr[edx+8]
        fld st
        fmul st,st(1)
        fld st(3)
        fmul st,st(4)
        faddp st(1),st
        fld st(2)
        fmul st,st(3)
        faddp st(1),st
        fsqrt
        fstp st(3)
        fstp st
        fstp st
        fstp result
        fldcw saved
    }
    return result;
}

// Original40BF24..40BF36 then42CEA9/42CEBA..42CED1. Finite nonzero
// direction reference, with exact original dot-product association Z,Y,X.
inline BehaviorVector3 RandomMovementNormalizePc24(const BehaviorVector3& input) {
    unsigned short saved{},pc24{};
    __asm fnstcw saved
    pc24=static_cast<unsigned short>(saved & ~0x0300u);
    const auto* p=input.data();BehaviorVector3 output{};auto* result_data=output.data();
    const float one=1.f;
    __asm {
        fldcw pc24
        mov eax,p
        mov edx,result_data
        fld dword ptr[eax+8]
        fmul dword ptr[eax+8]
        fld dword ptr[eax+4]
        fmul dword ptr[eax+4]
        faddp st(1),st
        fld dword ptr[eax]
        fmul dword ptr[eax]
        faddp st(1),st
        fsqrt
        fdivr one
        fld st
        fmul dword ptr[eax]
        fstp dword ptr[edx]
        fld st
        fmul dword ptr[eax+4]
        fstp dword ptr[edx+4]
        fmul dword ptr[eax+8]
        fstp dword ptr[edx+8]
        fldcw saved
    }
    return output;
}
}
#endif
