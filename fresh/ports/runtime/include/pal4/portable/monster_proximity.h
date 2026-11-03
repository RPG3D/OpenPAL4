#pragma once
#include "pal4/portable/scene_monster_instances.h"
#include "pal4/portable/world.h"
#include "pal4/monster_proximity_checker.h"
namespace pal4::portable {
// 借用当前场景的实际管理器/实例/墙体；重载场景时重建，依赖必须活得更久。
// 受控角色快照须提供原逻辑token、活动位、位置、球心与半高，不能用屏幕坐标替代。
class MonsterProximity final {
public:
    using ControlledActor=std::function<MonsterProximityActor()>;
    MonsterProximity(SceneMonsterInstances&,SceneMonsterHandle,MonsterBehaviorManagerState&,
        std::uint32_t manager_token,std::uint32_t source_token,const CollisionMesh& actual_movement,
        ControlledActor actual_controlled_actor);
    ~MonsterProximity();
    MonsterProximity(const MonsterProximity&)=delete;
    MonsterProximity& operator=(const MonsterProximity&)=delete;
    bool Check();
    bool BeyondReturnThreshold();
    bool CheckReturnTargetReached();
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
