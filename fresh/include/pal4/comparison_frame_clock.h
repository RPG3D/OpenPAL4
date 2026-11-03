#pragma once
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <string_view>

namespace pal4 {
// Diagnostic input for the existing frame updater, not a replacement timer.
// Zero disables it. The real-time comparison remains a separate acceptance gate.
class ComparisonFrameClock final {
public:
    static std::uint32_t ParseStep(const char* text,bool scripted) {
        if(!text || !*text)return 0u;
        if(!scripted)throw std::invalid_argument("comparison frame clock requires scripted input");
        const std::string_view value(text);std::uint32_t step{};
        const auto result=std::from_chars(value.data(),value.data()+value.size(),step);
        if(result.ec!=std::errc{} || result.ptr!=value.data()+value.size() || step<17u || step>100u)
            throw std::invalid_argument("comparison frame step must be an integer in [17,100] ms");
        return step;
    }
    static std::uint32_t EnvironmentStep() {
        return ParseStep(std::getenv("PAL4_COMPARE_FRAME_STEP_MS"),
                         std::getenv("PAL4_COMPARE_SCRIPTED_INPUT")!=nullptr);
    }
    explicit ComparisonFrameClock(std::uint32_t step):step_(step){}
    bool Enabled()const noexcept{return step_!=0u;}
    std::uint32_t StepMilliseconds()const noexcept{return step_;}
    std::uint32_t Next()noexcept{return current_+=step_;}
    std::uint32_t Current()const noexcept{return current_;}
    std::uint32_t ResetTimestamp(std::uint32_t real_time)const noexcept{return Enabled()?current_:real_time;}
    static bool IsOriginalFrameCaller(std::uintptr_t caller,std::uintptr_t base)noexcept {
        return caller==base+0x92a8u || caller==base+0x92ccu;
    }
    static bool IsOriginalResetCaller(std::uintptr_t caller,std::uintptr_t base)noexcept {
        // IDA5D3841,5D32B0,58F1FF,5B6767 call the same WINMM import.
        return caller==base+0x1d3846u || caller==base+0x1d32b5u ||
               caller==base+0x18f204u || caller==base+0x1b676cu;
    }
private:
    std::uint32_t step_{},current_{};
};
inline ComparisonFrameClock& GetComparisonFrameClock(){
    static ComparisonFrameClock clock(ComparisonFrameClock::EnvironmentStep());return clock;
}
}
