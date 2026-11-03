#pragma once
#include "pal4/native_behavior_tasks.h"
#include "pal4/native_behavior_movement.h"
#include "pal4/native_actor_behavior_orientation.h"
#include <cstddef>

namespace pal4 {
class NativeMonsterManagerOwner;
class NativeEventDocumentRuntime;
class NativeWorldPathPlannerOwner;
struct NativeMonsterBehaviorSessionStats {
    std::uint32_t constructed{},accepted{},rolled_back{},released{},target_updates{},grid_frames{};
    std::array<std::uint32_t,4> accepted_by_kind{},released_by_kind{};
};
// Scene-owned helper composition; cancel tasks before releasing paths, actors
// or manager grid. Identifier/random/orientation caches belong to the game.
class NativeMonsterBehaviorSession final {
public:
    NativeMonsterBehaviorSession(NativeMonsterManagerOwner&,NativeBaseObjectMemory&,
        NativeWorldPathPlannerOwner&,NativeSceneMovementAsset&,NativePlayerTransformRuntime&,
        NativeActorBehaviorOrientationCaches&,std::uint32_t& identifier,NativeBehaviorMovement::Random);
    ~NativeMonsterBehaviorSession();
    std::shared_ptr<INativeBehaviorHelpers> Helpers(std::uint32_t,NativeEventDocumentRuntime&);
    const NativeMonsterBehaviorSessionStats& Stats() const;
    std::size_t LiveHelpers() const;
private:
    class Impl;
    class HelpersForMonster;
    std::shared_ptr<Impl> impl_;
};
}
