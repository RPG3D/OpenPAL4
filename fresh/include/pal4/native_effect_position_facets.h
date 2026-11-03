#pragma once
#include <array>
#include <cstdint>

namespace pal4 {
// Optional capability of an effect factory. PositionTask must not assume that
// a legacy task fixture also implements model movement or facing.
class INativeEffectPositionFacets {
public:
    virtual ~INativeEffectPositionFacets()=default;
    virtual std::array<float,3> ReadPosition(std::uint32_t facet)=0;
    virtual void Move(std::uint32_t facet,const std::array<float,3>&,bool relative)=0;
    virtual void Face(std::uint32_t facet,const std::array<float,3>&)=0;
    virtual void SetFaceRotation(std::uint32_t facet,const std::array<float,3>&)=0;
    virtual void ActivateFace(std::uint32_t facet)=0;
    virtual std::uint8_t ControlEnabled(std::uint32_t facet)=0;
    virtual void SetControlEnabled(std::uint32_t facet,std::uint8_t)=0;
};
}
