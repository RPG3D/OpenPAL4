#pragma once
#include <cstdint>
#include "pal4/monster_behavior_manager.h"
#include "pal4/portable/monster_behavior_definition.h"
#include <functional>
#include <memory>
#include <ostream>
namespace pal4::portable {class ObjectMemory;}
namespace pal4::portable::detail {
// Required adapters to the actual owned queue; internal to movement behavior.
struct MonsterSchedulerBindings {
    std::function<bool()> ready;
    std::function<bool()> dispatch;
    std::function<std::uint32_t(float)> advance;
    std::function<std::uint32_t()> random;
};
class MonsterBehaviorScheduler final {
public:
    MonsterBehaviorScheduler(const MonsterBehaviorDefinition&,MonsterSchedulerBindings,BinaryXmlNode& actual_memory,ObjectMemory* shared_storage=nullptr,bool prepare_behavior=true);
    // 构造组合先建立原空处理器，再在组件配置阶段装载行为。
    void PrepareBehavior();
    bool Prepared() const;
    void PrepareComponent(MonsterBehaviorManagerState&);
    void CopyProcessorTo(MonsterBehaviorManagerState&) const;
    void Close();
    bool Closed() const;
    std::uint32_t MemoryToken() const;
    std::size_t AllocationCount() const;
    BinaryXmlNode& Memory();
    ~MonsterBehaviorScheduler();
    void SetEnabled(bool);
    bool Enabled() const;
    std::uint32_t Advance(float);
    std::uint32_t AdvanceManager(MonsterBehaviorManagerState&,std::uint32_t token,float,std::uint32_t argument,IMonsterBehaviorFrameRuntime& required_services);
    IScriptEventProcessorRuntime& PursuitProcessor(MonsterBehaviorManagerState&,std::uint32_t);
    void RestoreFuzzy(MonsterBehaviorManagerState&,std::uint32_t,bool has_fuzzy);
    void WriteReport(std::ostream&) const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
