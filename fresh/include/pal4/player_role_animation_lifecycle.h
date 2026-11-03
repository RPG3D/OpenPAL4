#pragma once
#include <cstdint>

namespace pal4 {
// Role loading owns the timing; the native animation owner owns the records.
class IPlayerRoleAnimationLifecycle {
public:
    virtual ~IPlayerRoleAnimationLifecycle()=default;
    virtual void BindLoadedActor(std::uint32_t slot)=0;
    virtual void ReleaseActor(std::uint32_t slot)=0;
};
}
