#pragma once
#include "pal4/portable/monster_base_motion.h"
#include "pal4/monster_pursuit_collision_speed.h"
namespace pal4::portable {
// 原追逐/返程路径阶段；场景、实际运动及完整重置服务均由调用方拥有。
class MonsterPursuitPaths final {
public:
    using TargetResolver=std::function<MonsterPursuitTarget(std::uint32_t)>;
    using Reset=std::function<std::uint32_t()>;
    MonsterPursuitPaths(SceneMonsterInstances&,SceneMonsterHandle,MonsterBehaviorManagerState&,
        std::uint32_t manager_token,std::uint32_t source_token,MonsterBaseMotion&,CollisionSurface&,
        TargetResolver actual_target,Reset actual_reset);
    ~MonsterPursuitPaths();
    MonsterPursuitPaths(const MonsterPursuitPaths&)=delete;
    MonsterPursuitPaths& operator=(const MonsterPursuitPaths&)=delete;
    bool Process(float elapsed,std::uint32_t target);
    std::uint32_t EnterReturn(float elapsed);
    bool Failed() const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
