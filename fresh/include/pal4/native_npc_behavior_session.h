#pragma once
#include "pal4/native_behavior_tasks.h"
#include "pal4/native_behavior_movement.h"
#include "pal4/native_actor_behavior_orientation.h"
namespace pal4 {
class NativeArenaNpcSceneLoader;
class NativeBaseObjectMemory;
class NativeWorldPathPlannerOwner;
class NativeSceneMovementAsset;
class NativePlayerTransformRuntime;
class NativeEventDocumentRuntime;
class INativeNpcGridElements;
struct NativeNpcBehaviorSessionStats {
    std::uint32_t constructed{},accepted{},rolled_back{},released{},target_updates{};
    std::uint32_t grid_frames{},stationary_frames{},curve_frames{};
    std::array<std::uint32_t,5> accepted_by_kind{},released_by_kind{};
};
// Scene-owned composition. Retire tasks before destroying this scene's grid,
// paths or actors. Identifier/random/orientation caches belong to the game session.
class NativeNpcBehaviorSession final {
public:
    NativeNpcBehaviorSession(NativeArenaNpcSceneLoader&,NativeBaseObjectMemory&,
        NativeWorldPathPlannerOwner&,NativeSceneMovementAsset&,NativePlayerTransformRuntime&,
        NativeActorBehaviorOrientationCaches&,std::uint32_t& identifier,NativeBehaviorMovement::Random);
    ~NativeNpcBehaviorSession();
    std::shared_ptr<INativeBehaviorHelpers> Helpers(std::uint32_t npc,NativeEventDocumentRuntime&);
    std::shared_ptr<INativeNpcGridElements> GridServices();
    const NativeNpcBehaviorSessionStats& Stats() const;
    std::size_t LiveHelpers(std::uint32_t npc=0u) const;
private:
    class Impl;
    class HelpersForNpc;
    std::shared_ptr<Impl> impl_;
};
}
