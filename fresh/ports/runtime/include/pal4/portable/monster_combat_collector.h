#pragma once
#include "pal4/portable/monster_scene_actors.h"
#include "pal4/portable/monster_player_view.h"
#include "pal4/portable/world.h"

namespace pal4::portable {
// 顺序须由场景管理器提供；此组件不假设资源文件顺序等于原名称树顺序。
// 所有依赖必须比本对象活得更久。失败可能保留部分记录，不可重入。
class MonsterCombatCollector final {
public:
    MonsterCombatCollector(SceneMonsterInstances&,MonsterSceneActors&,MonsterPlayerView&,
                           const CollisionMesh&);
    MonsterCombatCollector(SceneMonsterInstances&,MonsterSceneActors&,MonsterPlayerView&,
                           const CollisionMesh&,std::vector<std::size_t> traversal_order);
    ~MonsterCombatCollector();
    MonsterCombatCollector(const MonsterCombatCollector&)=delete;
    MonsterCombatCollector& operator=(const MonsterCombatCollector&)=delete;
    void Collect(std::size_t initiator);
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
