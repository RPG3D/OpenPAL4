#pragma once
#include "pal4/native_actor_behavior_state_runtime.h"

namespace pal4 {
class NativeArenaNpcSceneLoader;
class NativeWorldPathPlannerOwner;
class NativeActorAnimationStateBinding;
// Borrowed live scene bindings; retire before either scene owner is destroyed.
class NativeActorBehaviorResources final : public INativeActorBehaviorResources {
public:
    NativeActorBehaviorResources(NativeArenaNpcSceneLoader&,NativeWorldPathPlannerOwner&);
    using ValidateActor=std::function<void(std::uint32_t)>;
    using AnimationLookup=std::function<NativeActorAnimationStateBinding&(std::uint32_t)>;
    NativeActorBehaviorResources(NativeWorldPathPlannerOwner&,ValidateActor,AnimationLookup);
    std::uint32_t FindPath(std::string_view) override;
    MonsterTransformRows Transform(std::uint32_t,std::uint32_t) override;
    // Actor animation virtual wrappers443FF0/444080: an equal name preserves
    // the active blend, interpolator clock and completion latch.
    bool Select(std::uint32_t,std::string_view,bool,float) override;
    std::uint32_t SetRestart(std::uint32_t,std::uint32_t) override;
    float Time(std::uint32_t) override;
    void SetTime(std::uint32_t,float) override;
    std::uint32_t Completed(std::uint32_t) override;
private:
    ValidateActor validate_;
    AnimationLookup animation_;
    NativeWorldPathPlannerOwner& paths_;
};
}
