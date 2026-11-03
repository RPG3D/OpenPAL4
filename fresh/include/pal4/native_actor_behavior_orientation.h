#pragma once
#include "pal4/game_object_orientation_updater.h"
#include <array>
#include <functional>
#include "pal4/native_actor_behavior_state_runtime.h"

namespace pal4 {
class NativeArenaNpcSceneLoader;
class NativeBaseObjectMemory;
class NativeSceneMovementAsset;
class NativePlayerTransformRuntime;
// Original per-variant static caches. The game/session owns these across scene
// changes; do not make them per-NPC or recalculate for every actor.
using NativeActorBehaviorOrientationCaches=std::array<GameObjectOrientationGlobals,4>;

class NativeActorBehaviorOrientation final {
public:
    NativeActorBehaviorOrientation(NativeArenaNpcSceneLoader&,NativeBaseObjectMemory&,
        NativeSceneMovementAsset&,NativePlayerTransformRuntime&,NativeActorBehaviorOrientationCaches&);
    using ValidateActor=std::function<void(std::uint32_t)>;
    NativeActorBehaviorOrientation(NativeBehaviorActorFamily,ValidateActor,NativeBaseObjectMemory&,
        NativeSceneMovementAsset&,NativePlayerTransformRuntime&,NativeActorBehaviorOrientationCaches&);
    // Original target Update callback; helper +B9 gates floor/pose application.
    // This consumes movement produced by grid physics, it does not generate it.
    bool Update(std::int32_t kind,std::uint32_t npc,std::uint32_t helper);
private:
    class Runtime;
    NativeBehaviorActorFamily family_;
    ValidateActor validate_;
    NativeBaseObjectMemory& memory_;
    NativeSceneMovementAsset& movement_;
    NativePlayerTransformRuntime& transforms_;
    NativeActorBehaviorOrientationCaches& caches_;
};
}
