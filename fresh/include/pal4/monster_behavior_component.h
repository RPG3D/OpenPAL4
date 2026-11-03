#pragma once
#include "pal4/monster_behavior_manager.h"
namespace pal4 {
// 原4318A0/431A10的组件装载边界；文档和记录可由现代对象拥有。
class IMonsterBehaviorComponentRuntime {
public:
    virtual ~IMonsterBehaviorComponentRuntime()=default;
    virtual std::uint32_t SourceDocument(std::uint32_t source)=0;
    virtual void ClearEventsAndTasks(MonsterBehaviorManagerState&,std::uint32_t manager)=0;
    virtual std::uint32_t XmlChildByIndex(std::uint32_t document,std::uint32_t index)=0;
    virtual void LoadFuzzyBehaviorConfig(std::uint32_t manager,std::uint32_t child)=0;
    virtual void DispatchComponentField(std::uint32_t manager,std::uint32_t slot,std::uint32_t field)=0;
    virtual void ClearHistoryRecord(std::uint32_t record)=0;
};
}
