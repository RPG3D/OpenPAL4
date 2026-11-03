#pragma once
#include <cstdint>
namespace pal4 {
class INativeEffectCameraFacets {
public:
    virtual ~INativeEffectCameraFacets()=default;
    virtual void StartCamera(std::uint32_t facet)=0;
    virtual void StopCamera(std::uint32_t facet)=0;
    virtual std::uint32_t CameraEffectFrame(std::uint32_t owner)=0;
};
}
