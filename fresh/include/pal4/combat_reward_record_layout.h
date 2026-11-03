#pragma once
#include <cstdint>
namespace pal4 {
// Original 40-byte player record in the 212-byte battle report. The first
// 16 bytes are the learned-record vector; scalar experience starts at16.
struct CombatRewardRecordLayout final {
    static constexpr std::uint32_t kBytes = 40u;
    static constexpr std::uint32_t kGainedExperience = 16u;
    static constexpr std::uint32_t kPreviousLevel = 20u;
    static constexpr std::uint32_t kTargetLevel = 24u;
    static constexpr std::uint32_t kPreviousExperience = 28u;
    static constexpr std::uint32_t kUpdatedExperience = 32u;
    static constexpr std::uint32_t kActive = 36u;
};
}
