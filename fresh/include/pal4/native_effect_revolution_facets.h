#pragma once
#include <array>
#include <cstdint>
namespace pal4 {
// Optional capability used by the shared RevoluteTask lifecycle.
class INativeEffectRevolutionFacets {
public:
    virtual ~INativeEffectRevolutionFacets()=default;
    virtual void ApplyRevolution(std::uint32_t facet,
        const std::array<float,3>& origin,const std::array<float,6>& motion)=0;
    virtual bool CameraIsActive(std::uint32_t facet)=0;
    virtual bool CameraHasTarget(std::uint32_t facet)=0;
    virtual void ReadCameraTarget(std::uint32_t facet,std::array<float,3>&)=0;
};
}
