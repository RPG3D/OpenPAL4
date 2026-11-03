#pragma once
#include "pal4/monster_interaction_processor.h"
#include "pal4/portable/monster_player_view.h"
#include "pal4/portable/scene_monster_instances.h"

namespace pal4::portable {
// 控制器状态、距离和脚本操作由调用方提供；不提供空成功实现。
// 借用对象必须活得更久，单线程且不可重入。
class MonsterInteraction final {
public:
    MonsterInteraction(SceneMonsterInstances&,SceneMonsterHandle,MonsterBehaviorManagerState&,
                       std::uint32_t manager,std::uint32_t source,MonsterPlayerView&,
                       IMonsterInteractionRuntime& scene_services);
    ~MonsterInteraction();
    MonsterInteraction(const MonsterInteraction&)=delete;
    MonsterInteraction& operator=(const MonsterInteraction&)=delete;
    bool Process(std::uint32_t target);
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
