#pragma once
#include "pal4/comparison_frame_gate.h"
#include <charconv>
#include <cstdlib>
#include <stdexcept>
#include <string_view>
namespace pal4 {
// Explicit external time input experiment at verified game time boundaries.
// It never seeds/draws RNG or owns gameplay clock state. Disabled uses public APIs.
class ComparisonSeedClock final {
public:
    static ComparisonSeedClock Parse(const char* text,bool scripted,bool checkpoint,std::uint32_t step) {
        if(!text || !*text)return {};
        if(!scripted || !checkpoint || step<17u || step>100u)
            throw std::invalid_argument("seed clock input requires scripted checkpoint comparison and fixed frame step");
        std::uint32_t epoch{};const std::string_view value(text);
        const auto result=std::from_chars(value.data(),value.data()+value.size(),epoch);
        if(result.ec!=std::errc{} || result.ptr!=value.data()+value.size() || !epoch || epoch>0x7fffffffu)
            throw std::invalid_argument("seed clock epoch must be a positive signed32 second timestamp");
        return ComparisonSeedClock(epoch);
    }
    static constexpr bool IsOriginalGobTimeCaller(std::uintptr_t caller,std::uintptr_t base) noexcept {
        return caller==base+0x1a189eu || caller==base+0x1a5395u ||
               caller==base+0x1a5a74u || caller==base+0x1a6233u;
    }
    bool Enabled()const noexcept{return epoch_!=0u;}
    std::uint32_t Epoch(std::uint32_t real,std::uint64_t frames,std::uint32_t step)const noexcept {
        return Enabled()?epoch_+static_cast<std::uint32_t>((frames*step)/1000u):real;
    }
    std::uint32_t Milliseconds(std::uint32_t real,std::uint64_t frames,std::uint32_t step)const noexcept {
        return Enabled()?1000u+static_cast<std::uint32_t>(frames*step):real;
    }
    static ComparisonSeedClock FromEnvironment() {
        const auto* text=std::getenv("PAL4_COMPARE_SEED_EPOCH");
        if(!text || !*text)return {};
        return Parse(text,std::getenv("PAL4_COMPARE_SCRIPTED_INPUT")!=nullptr,
            GetComparisonFrameGate().Observe().checkpoint_allowed,
            GetComparisonFrameClock().StepMilliseconds());
    }
private:
    ComparisonSeedClock()=default;
    explicit ComparisonSeedClock(std::uint32_t epoch):epoch_(epoch){}
    std::uint32_t epoch_{};
};
inline const ComparisonSeedClock& GetComparisonSeedClock() {
    static const auto clock=ComparisonSeedClock::FromEnvironment();return clock;
}
inline std::uint32_t ComparisonSeedEpoch(std::uint32_t real) {
    const auto& clock=GetComparisonSeedClock();if(!clock.Enabled())return real;
    return clock.Epoch(real,GetComparisonFrameGate().Observe().completed,GetComparisonFrameClock().StepMilliseconds());
}
inline std::uint32_t ComparisonSeedMilliseconds(std::uint32_t real) {
    const auto& clock=GetComparisonSeedClock();if(!clock.Enabled())return real;
    return clock.Milliseconds(real,GetComparisonFrameGate().Observe().completed,GetComparisonFrameClock().StepMilliseconds());
}
}
