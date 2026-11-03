#pragma once
#include "pal4/portable/monster_behavior_tasks.h"
#include "pal4/portable/monster_orientation.h"
#include "pal4/monster_behavior_manager.h"

namespace pal4::portable {
class ObjectMemory;
class MonsterBaseMotion;
enum class MonsterBehaviorPreparation {Immediate,Component};
struct MonsterMovementBehaviorStats {
    std::uint64_t constructed{},entered{},saved{},discarded{},updates{},failures{};
    std::size_t live_actions{};
};
// Per-actor path0/1 and random3 factory, original task queue and event scheduler.
// Dependencies, actual memory node and application orientation caches must
// outlive this owner. Cancel explicitly to observe cleanup/save errors.
class MonsterMovementBehavior final {
public:
    using Radius=std::function<float()>;using Identifier=std::function<std::uint32_t()>;
    MonsterMovementBehavior(MonsterMovementWorld&,SceneMonsterInstances&,SceneMonsterHandle,
        BinaryXmlNode& actual_memory,MonsterOrientation&,Radius actual_manager_radius,Identifier shared_identifiers);
    // 原成员组合路径：构造空manager图像、沿用同一实际动作工厂与共享存储。
    // 随后绑定模型/基础运动；必须先关闭基础运动，再Close本拥有者。
    MonsterMovementBehavior(MonsterMovementWorld&,SceneMonsterInstances&,SceneMonsterHandle,
        BinaryXmlNode&,MonsterOrientation&,Radius,Identifier,
        MonsterBehaviorManagerState&,std::uint32_t manager_token,ObjectMemory&,
        MonsterBehaviorPreparation=MonsterBehaviorPreparation::Immediate);
    // Component模式在模型/基础运动绑定后，由原MST记录装载行为与初始状态。
    void InitializeComponent(MonsterBaseMotion&,bool dispatch_birth_transform=false);
    std::uint32_t SourceToken() const;
    std::uint32_t BehaviorChildToken() const;
    ~MonsterMovementBehavior();
    MonsterMovementBehavior(const MonsterMovementBehavior&)=delete;
    MonsterMovementBehavior& operator=(const MonsterMovementBehavior&)=delete;
    std::optional<std::uint32_t> FactoryAddress(std::int32_t class_id) const;
    bool Start();
    bool Start(const MonsterBehaviorDefinition&);
    // Caller first updates the entire shared grid, then each actor's queue.
    // Animation advancement and outer manager state gates are separate phases.
    void AdvanceAfterGrid(float seconds);
    // Original event scheduler repeats the actual fuzzy event when queue is ready.
    // Disabled by default for single-round callers. Disable does not cancel.
    void SetSchedulingEnabled(bool);
    bool SchedulingEnabled() const;
    std::uint32_t AdvanceScheduledAfterGrid(float seconds);
    // Original4317F0 gates with the actual borrowed manager image. This owner
    // exclusively controls its queue and event processor. The manager-image
    // constructor also owns original members; pursuit/collision/base-motion
    // services must still come from the scene owner.
    // Image and token must stay stable and the image must outlive this owner.
    std::uint32_t AdvanceManagerAfterGrid(MonsterBehaviorManagerState&,std::uint32_t manager_token,
        float seconds,std::uint32_t argument,IMonsterBehaviorFrameRuntime& required_services);
    void Cancel();
    // 终结队列及原处理器；原成员组合路径还销毁基础成员。
    // 保留调用方拥有的持久记忆节点，必须先关闭基础运动。
    void Close();
    bool Closed() const;
    std::uint32_t ProcessorMemoryToken() const;
    std::size_t ProcessorAllocationCount() const;
    BinaryXmlNode& ProcessorMemory();
    bool Ready() const;
    bool Failed() const;
    bool IsBusy() const noexcept;
    bool UsesWorld(const MonsterMovementWorld&) const noexcept;
    std::size_t TaskCount() const;
    MonsterMovementBehaviorStats Stats() const;
    void WriteReport(std::ostream&) const;
private:
    friend class MonsterPursuitFrame;
    IScriptEventProcessorRuntime& PursuitProcessor(MonsterBehaviorManagerState&,std::uint32_t);
    void ClearPursuitTasks(MonsterBehaviorManagerState&,std::uint32_t);
    void RestorePursuitBehavior(MonsterBehaviorManagerState&,std::uint32_t,std::uint32_t child);
    void ValidateActor(SceneMonsterInstances&,SceneMonsterHandle) const;
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
