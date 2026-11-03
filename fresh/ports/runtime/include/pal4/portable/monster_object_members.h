#pragma once
#include "pal4/portable/monster_behavior_tasks.h"
#include "pal4/portable/object_memory.h"
#include "pal4/monster_behavior_manager.h"
namespace pal4::portable {
struct SceneMonster;
namespace detail {class MonsterBehaviorScheduler;}
// 原构造的跨平台组合：基础成员、实际处理器、任务队列及现代工厂注册表。
// state/storage/factory/持久记忆必须存活更久。关闭运动并交还动画后显式Close。
// 不创建场景服务，不自动装载模型、不推进剧情或战斗。
class MonsterObjectMembers final {
public:
    MonsterObjectMembers(MonsterBehaviorManagerState&,std::uint32_t logical_address,ObjectMemory&,
        IMonsterBehaviorActionFactory&,const MonsterBehaviorDefinition&,BinaryXmlNode& actual_memory,
        std::function<std::uint32_t()> actual_random);
    ~MonsterObjectMembers();
    MonsterObjectMembers(const MonsterObjectMembers&)=delete;
    MonsterObjectMembers& operator=(const MonsterObjectMembers&)=delete;
    void Close();
    bool Closed() const;
    void PrepareBehavior();
    using ComponentTransform=std::function<void(std::uint32_t slot,const std::array<float,3>&)>;
    void InitializeComponent(const SceneMonster&,ComponentTransform dispatch_transform={});
    std::uint32_t SourceToken() const;
    std::uint32_t BehaviorChildToken() const;
    MonsterBehaviorTasks& Tasks();
    detail::MonsterBehaviorScheduler& Scheduler();
    void ValidateManager(const MonsterBehaviorManagerState&,std::uint32_t logical_address) const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
