#pragma once
#include <cstdint>
namespace pal4 {
// Optional collection/queue capability used by RumpShahdowTask.
class INativeEffectShadowFacets {
public:
    virtual ~INativeEffectShadowFacets()=default;
    virtual bool PrepareShadowCount(std::uint32_t facet,std::int32_t count)=0;
    virtual std::uint32_t ShadowChild(std::uint32_t facet,std::int32_t index)=0;
    virtual std::uint32_t ShadowFrame(std::uint32_t object)=0;
    virtual void DispatchShadow(std::uint32_t child,bool retiring)=0;
};
}
