#pragma once
#include "pal4/portable/scene_monster_instances.h"
#include "pal4/portable/package.h"
#include "pal4/monster_behavior_manager.h"
namespace pal4::portable {
struct ActorResourceFrameStats {std::uint64_t calls{},updated{},skipped{};};
// 原包确认没有UVA/独立阴影时的真实667620资源更新分支。
// 存在动态资源则明确拒绝，不能把缓存未保留元数据当成原资源不存在。
class ActorResourceFrame final {
public:
    ActorResourceFrame(SceneMonsterInstances&,SceneMonsterHandle,MonsterBehaviorManagerState&,PackageReader& actual_actor_package);
    ~ActorResourceFrame();
    ActorResourceFrame(const ActorResourceFrame&)=delete;
    ActorResourceFrame& operator=(const ActorResourceFrame&)=delete;
    bool Update(MonsterBehaviorManagerState&,float elapsed);
    ActorResourceFrameStats Stats() const;
    bool Failed() const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
