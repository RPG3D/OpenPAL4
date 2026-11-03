#pragma once
namespace pal4 {
struct ActorControlVec3 { float x{},y{},z{}; };
static_assert(sizeof(ActorControlVec3)==12);
struct ActorMovementRadii { float lower{},upper{}; };
}  // namespace pal4
