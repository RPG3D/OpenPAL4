#pragma once
#include <array>
#include <cstdint>
namespace pal4 {
class INativeEffectRotationFacets {
public:
    virtual ~INativeEffectRotationFacets()=default;
    virtual void ReadRotation(std::uint32_t,std::array<float,3>&)=0;
    virtual void ApplyRotation(std::uint32_t,const std::array<float,3>&,bool relative)=0;
    virtual void SetCameraLockedLookAt(std::uint32_t,bool)=0;
};
}
