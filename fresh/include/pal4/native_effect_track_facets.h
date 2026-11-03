#pragma once
#include <array>
#include <cstdint>
namespace pal4 {
// TrackTask's facet offsets differ from PositionTask and MoveToTargetTask.
class INativeEffectTrackFacets {
public:
    virtual ~INativeEffectTrackFacets()=default;
    virtual void SetTrackPosition(std::uint32_t,const std::array<float,3>&)=0;
    virtual void SetTrackDirection(std::uint32_t,const std::array<float,3>&)=0;
};
}
