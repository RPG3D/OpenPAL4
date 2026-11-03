#include "pal4/native_legacy_crt_math.h"
#include <bit>
#include <cstdint>
#include <float.h>
#include <iostream>
#include <stdexcept>

int main(){const auto saved=_controlfp(0,0);try{
    (void)_controlfp(_PC_24,_MCW_PC);
    pal4::NativeLegacyCrtMath::Initialize();
    const auto control=_controlfp(0,0);
    // Actual original pair98 Run direction BF599E3C -> Face yaw43143828.
    const auto radians=pal4::NativeLegacyCrtMath::Acos(
        static_cast<double>(std::bit_cast<float>(0xbf599e3cu)));
    const auto degrees=static_cast<float>(radians*57.29577951308232);
    if(std::bit_cast<std::uint32_t>(degrees)!=0x43143828u)
        throw std::runtime_error("legacy CRT acos retains original Face store bits");
    const auto formation=static_cast<float>(pal4::NativeLegacyCrtMath::Acos(
        static_cast<double>(std::bit_cast<float>(0x3f7ff4f7u)))*57.29577951308232);
    if(std::bit_cast<std::uint32_t>(formation)!=0x3f869642u)
        throw std::runtime_error("legacy CRT acos retains original M13 formation yaw bits");
    if((_controlfp(0,0)&(_MCW_PC|_MCW_RC))!=(control&(_MCW_PC|_MCW_RC)))
        throw std::runtime_error("public legacy CRT must restore caller precision/rounding");
    (void)_controlfp(saved,_MCW_PC|_MCW_RC);
    std::cout<<"Original Face acos and caller precision pass\n";return 0;
}catch(const std::exception& e){(void)_controlfp(saved,_MCW_PC|_MCW_RC);std::cerr<<e.what()<<'\n';return 1;}}
