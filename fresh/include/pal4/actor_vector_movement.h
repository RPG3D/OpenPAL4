#pragma once
#include "pal4/actor_control_movement_types.h"
#include <cstdint>
#include <string_view>

namespace pal4 {
// Source game vector-control flow, with the x86 object/address boundary moved
// into its legacy adapter. Native ports implement these services with owners.
class IActorVectorMovementRuntime {
public:
    virtual ~IActorVectorMovementRuntime()=default;
    virtual void Begin()=0;
    virtual ActorControlVec3 Position()=0;
    virtual float Speed()=0;
    virtual ActorControlVec3 Forward()=0;
    virtual ActorMovementRadii PrepareBounds(ActorControlVec3 position)=0;
    virtual std::uint32_t Mode()=0;
    virtual void SelectMoveAnimation(std::string_view name)=0;
    virtual void Preprocess(ActorControlVec3& displacement)=0;
    virtual void UpdateGround(ActorControlVec3& position,ActorMovementRadii radii)=0;
    virtual bool CanMove(ActorControlVec3& position,ActorMovementRadii radii)=0;
    virtual void SetPosition(ActorControlVec3 position)=0;
    virtual void FaceInput()=0;
    virtual void UpdateFloor()=0;
};

// Preserves the original ordering and float/double narrowing. Rejection keeps
// animation selection and solver calls, but never writes position/yaw/floor.
inline bool DispatchActorVectorMovement(float elapsed,IActorVectorMovementRuntime& runtime) {
    runtime.Begin();
    auto position=runtime.Position();
    const auto distance=static_cast<float>(static_cast<double>(elapsed)*static_cast<double>(runtime.Speed()));
    const auto direction=runtime.Forward();
    const auto radii=runtime.PrepareBounds(position);
    std::string_view animation;
    switch (runtime.Mode()) {
    case 0: animation="C02"; break;
    case 1: case 2: animation="C03"; break;
    default: break;
    }
    runtime.SelectMoveAnimation(animation);
    ActorControlVec3 displacement{direction.x*distance,direction.y*distance,direction.z*distance};
    runtime.Preprocess(displacement);
    position.x+=displacement.x; position.y+=displacement.y; position.z+=displacement.z;
    runtime.UpdateGround(position,radii);
    if (!runtime.CanMove(position,radii)) return false;
    runtime.SetPosition(position); runtime.FaceInput(); runtime.UpdateFloor();
    return true;
}
}  // namespace pal4
