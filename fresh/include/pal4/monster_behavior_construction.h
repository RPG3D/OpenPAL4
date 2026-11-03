#pragma once
#include "pal4/monster_behavior_manager.h"
namespace pal4 {
// 原4312A0的成员生命周期边界。工厂表可由现代容器拥有，
// 不要求宿主实现原STL节点或整套追逐/战斗服务。
class IMonsterBehaviorConstructionRuntime:public IMonsterBehaviorFactoryRegistrationRuntime {
public:
    virtual ~IMonsterBehaviorConstructionRuntime()=default;
    virtual void ConstructBase(MonsterBehaviorManagerState&,std::uint32_t)=0;
    virtual void DestructBase(MonsterBehaviorManagerState&,std::uint32_t)=0;
    virtual void ConstructProcessor(MonsterBehaviorManagerState&,std::uint32_t)=0;
    virtual void DestructProcessor(MonsterBehaviorManagerState&,std::uint32_t)=0;
    virtual void ConstructScript(MonsterBehaviorManagerState&,std::uint32_t)=0;
    virtual void DestructScript(MonsterBehaviorManagerState&,std::uint32_t)=0;
    virtual MonsterBehaviorTreeBytes TreeInputBytes()=0;
    virtual void InitializeTree(MonsterBehaviorManagerState&,std::uint32_t)=0;
    virtual void CleanupTree(MonsterBehaviorManagerState&,std::uint32_t)=0;
    virtual void DestroyAuxiliary(MonsterBehaviorManagerState&,std::uint32_t)=0;
    virtual void CleanupWordArray(MonsterBehaviorManagerState&,std::uint32_t)=0;
    virtual void CleanupHistory(MonsterBehaviorManagerState&,std::uint32_t)=0;
};
}
