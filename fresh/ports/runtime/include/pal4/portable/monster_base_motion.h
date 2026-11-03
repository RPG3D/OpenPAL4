#pragma once
#include "pal4/portable/scene_monster_instances.h"
#include "pal4/portable/surface_motion.h"
#include "pal4/monster_behavior_manager.h"
namespace pal4::portable {
struct MonsterBaseMotionResult {std::uint32_t resource_updates{},animation_updates{},collision_updates{},position_result{};};
// Actual instance playback and original CollisionActor movement. Resource
// schema/shadow update remains a required scene service, with no empty fallback.
// Requires a loaded single clump at unit scale; unsupported transforms fail.
// State, instances and surface must outlive this owner. Single thread, no reentry.
// constructed_object_storage非空时借用已构造的+564成员；运动退出后原对象
// 才能销毁该成员和存储。未提供存储时拒绝覆盖已有路径哨兵。
class MonsterBaseMotion final {
public:
    using ResourceUpdate=std::function<void(MonsterBehaviorManagerState&,float)>;
    MonsterBaseMotion(SceneMonsterInstances&,SceneMonsterHandle,MonsterBehaviorManagerState& actual_state,
        std::uint32_t manager_token,CollisionSurface&,float movement_height,
        MovementCollisionQuery actual_world_collision,ResourceUpdate actual_resource_update,
        ObjectMemory* constructed_object_storage=nullptr);
    ~MonsterBaseMotion();
    MonsterBaseMotion(const MonsterBaseMotion&)=delete;
    MonsterBaseMotion& operator=(const MonsterBaseMotion&)=delete;
    bool Start(Point3 from,Point3 to,std::uint32_t target_face);
    void SetSpeed(float);
    void Stop();
    // 先于原对象析构调用。返回false表示动画图像被外部覆盖，未恢复；
    // 不可继续按有效动画对象销毁。可重复关闭，关闭后禁止继续运动。
    bool Close();
    MonsterBaseMotionResult Update(float elapsed,std::uint32_t argument);
    bool MovementActive() const;
    bool Failed() const;
    bool Busy() const noexcept;
private:
    friend class MonsterPursuitPaths;
    friend class MonsterMovementBehavior;
    void ValidateComponentOwner(SceneMonsterInstances&,SceneMonsterHandle,MonsterBehaviorManagerState&,std::uint32_t) const;
    void ValidateBinding(SceneMonsterInstances&,SceneMonsterHandle,MonsterBehaviorManagerState&,std::uint32_t,CollisionSurface&) const;
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
