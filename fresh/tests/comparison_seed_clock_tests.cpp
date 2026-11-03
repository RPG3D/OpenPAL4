#include "pal4/comparison_seed_clock.h"
#include "pal4/native_crt_random.h"
#include <cassert>
#include <cstdlib>
#include <iostream>
int main() {
    using pal4::ComparisonSeedClock;
    const auto off=ComparisonSeedClock::Parse(nullptr,false,false,0u);
    assert(!off.Enabled() && off.Epoch(123u,999,17)==123u && off.Milliseconds(456u,999,17)==456u);
    const auto clock=ComparisonSeedClock::Parse("1793557546",true,true,17u);
    assert(clock.Epoch(1u,0u,17u)==1793557546u && clock.Milliseconds(2u,0u,17u)==1000u);
    assert(clock.Epoch(1u,59u,17u)==1793557547u && clock.Milliseconds(2u,59u,17u)==2003u);
    assert(clock.Milliseconds(1u,0xffffffffu,17u)==983u); // public DWORD wrapping
    for(const auto* text:{"0","-1","+1"," 1","1x","2147483648","4294967296"}) {
        bool rejected=false;try{(void)ComparisonSeedClock::Parse(text,true,true,17u);}catch(const std::invalid_argument&){rejected=true;}assert(rejected);
    }
    for(unsigned mode=0;mode<3u;++mode) {
        bool rejected=false;try{(void)ComparisonSeedClock::Parse("123",mode!=0,mode!=1,mode==2?0u:17u);}catch(const std::invalid_argument&){rejected=true;}assert(rejected);
    }
    for(const auto offset:{0x1a189eu,0x1a5395u,0x1a5a74u,0x1a6233u})
        assert(ComparisonSeedClock::IsOriginalGobTimeCaller(0x400000u+offset,0x400000u));
    assert(!ComparisonSeedClock::IsOriginalGobTimeCaller(0x4092a8u,0x400000u));
    assert(!ComparisonSeedClock::IsOriginalGobTimeCaller(0x5a6232u,0x400000u));
    std::srand(123u);const auto expected=std::rand();std::srand(123u);
    (void)clock.Epoch(5u,777u,17u);(void)clock.Milliseconds(6u,777u,17u);
    assert(std::rand()==expected);
    std::cout<<"PASS: explicit seed clock inputs, disabled public API values, validation, boundaries and no RNG draws\n";
}
