#pragma once
#include <array>
#include <cstdint>
namespace pal4 {
// Game host owns animation, RenderWare transforms and current-role borrowers.
class INativeScriptPlayerLeaderServices {
public:
    virtual ~INativeScriptPlayerLeaderServices()=default;
    virtual void SetLeaderAction(std::uint32_t actor,std::uint32_t name)=0;
    virtual void CopyLeaderTransform(std::uint32_t actor,std::uint32_t slot,
        const std::array<std::uint32_t,3>& bits,std::uint32_t option)=0;
    virtual std::uint32_t LeaderActorControl()=0;
    virtual void BindLeaderActor(std::uint32_t actor)=0;
};
}
