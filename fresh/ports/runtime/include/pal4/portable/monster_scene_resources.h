#pragma once
#include "pal4/portable/arena_source.h"
#include "pal4/portable/scene_monster_instances.h"
#include "pal4/portable/scene_pathways.h"
#include <memory>

namespace pal4::portable {
// 场景资源及延迟动作读取共享同一个原包源；玩法对象须先于本对象销毁。
class MonsterSceneResources {
public:
    MonsterSceneResources(std::uint64_t generation,std::shared_ptr<ArenaSource> source,
                          std::span<const SceneMonster> monsters,std::span<const ScenePath> paths);
    SceneMonsterInstances& Monsters() { return monsters_; }
    const SceneMonsterInstances& Monsters() const { return monsters_; }
    const std::shared_ptr<ScenePathways>& Paths() const { return paths_; }
private:
    std::shared_ptr<ArenaSource> source_;
    std::shared_ptr<ScenePathways> paths_;
    SceneMonsterInstances monsters_;
};
}
