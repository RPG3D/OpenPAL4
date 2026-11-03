#pragma once

#include <cstdint>

namespace pal4 {
// Optional playback controls. Missing owner capabilities must fail explicitly.
class INativeEffectPlaybackFacets {
public:
    virtual ~INativeEffectPlaybackFacets() = default;
    virtual void PauseEffect(std::uint32_t facet) = 0;
    // Native RewindEffectTask dispatches resume, preserving playback time.
    virtual void RewindEffect(std::uint32_t facet) = 0;
    virtual float RemainingEffectTime(std::uint32_t facet) = 0;
    virtual void StopEffect(std::uint32_t facet) = 0;
};
}  // namespace pal4
