#pragma once
#include "pal4/native_crt_random.h"
#include <cmath>
#include <stdexcept>
extern "C" __declspec(dllimport) unsigned long __stdcall timeGetTime(void);
namespace pal4 {
// Original6683C0 is game glue around the public CRT. Keep the reseed/draw even
// for equal bounds. timeGetTime's uint32->float can round up to 2^32.
inline float NativeGobRandomDelay(float minimum,float maximum,float seed){
    if(seed>0.f){
        if(!std::isfinite(seed) || seed>4294967296.f)throw std::out_of_range("GOB random seed");
        NativeCrtSeedRandom(static_cast<unsigned>(static_cast<std::uint64_t>(seed)),__FILE__);
    }
    return static_cast<float>(static_cast<double>(NativeCrtRandom(__FILE__))*
        0.00003051850947599719*static_cast<double>(maximum-minimum)+minimum);
}
}
