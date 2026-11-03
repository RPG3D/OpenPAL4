#pragma once
#include <cstdint>
namespace pal4 {
struct ResourceVec3;
// Borrowed typed actor owner for WorldManager scene events. Player and Monster
// ownership stay separate. A binding must close before either owner retires.
class INativeCombatSceneActors {
public:
    virtual ~INativeCombatSceneActors()=default;
    virtual void BindSceneEvents()=0;
    virtual void UnbindSceneEvents() noexcept=0;
    virtual std::uint32_t SceneChild(std::uint32_t actor)=0;
    virtual void ActivateSceneModel(std::uint32_t actor,std::uint32_t world,std::uint32_t queue)=0;
    virtual void PositionSceneModel(std::uint32_t actor,const ResourceVec3&)=0;
    virtual void RestoreSceneCallbacks()=0;
};
}
