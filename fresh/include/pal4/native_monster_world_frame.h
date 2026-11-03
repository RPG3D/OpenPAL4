#pragma once

#include "pal4/monster_behavior_manager.h"
#include "pal4/native_base_object_motion_frame.h"

#include <functional>

namespace pal4 {

class NativeMonsterManagerOwner;
class NativeMonsterPursuitFrame;
class NativeMonsterPursuitPathRuntime;
class NativeArenaCollisionTree;
class NativePlayerTransformRuntime;
class NativeBaseObjectMemory;
class IRwDictSchemaRuntime;

// Scene-owned native services for the existing outer 0x4317F0 update.
// All borrowed scene dependencies must outlive synchronous updates.
class NativeMonsterWorldFrame final : public IMonsterBehaviorFrameRuntime {
public:
    using Scheduler = std::function<IScriptEventSchedulerRuntime&(std::uint32_t)>;
    using AnimationUpdate = std::function<void(std::uint32_t, float)>;
    NativeMonsterWorldFrame(NativeMonsterManagerOwner& manager,
        NativeMonsterPursuitFrame& pursuit, NativeMonsterPursuitPathRuntime& paths,
        NativeArenaCollisionTree& collision, NativePlayerTransformRuntime& transforms,
        NativeBaseObjectMemory& memory, IRwDictSchemaRuntime& schema,
        Scheduler scheduler, AnimationUpdate animation);
    [[nodiscard]] std::uint32_t Update(std::uint32_t object, float elapsed,
                                       std::uint32_t argument = 0u);
    [[nodiscard]] const NativeBaseObjectMotionFrameResult& LastMotion() const noexcept {
        return last_motion_;
    }
    std::uint32_t ProcessManagerState(MonsterBehaviorManagerState& state,
        std::uint32_t object, float elapsed) override;
    void CheckStateAndProcessCollision(MonsterBehaviorManagerState& state,
        std::uint32_t object, float elapsed) override;
    IScriptEventSchedulerRuntime& SchedulerRuntime() override;
    std::uint32_t UpdatePosition(std::uint32_t object, float elapsed,
                                  std::uint32_t argument) override;
private:
    void RequireObject(std::uint32_t object) const;
    NativeMonsterManagerOwner& manager_;
    NativeMonsterPursuitFrame& pursuit_;
    NativeMonsterPursuitPathRuntime& paths_;
    NativeArenaCollisionTree& collision_;
    NativePlayerTransformRuntime& transforms_;
    NativeBaseObjectMemory& memory_;
    IRwDictSchemaRuntime& schema_;
    Scheduler scheduler_;
    AnimationUpdate animation_;
    std::uint32_t current_ = 0u;
    NativeBaseObjectMotionFrameResult last_motion_{};
};

}  // namespace pal4
