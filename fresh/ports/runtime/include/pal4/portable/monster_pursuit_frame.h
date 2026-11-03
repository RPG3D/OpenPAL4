#pragma once
#include "pal4/portable/monster_movement_behavior.h"
#include "pal4/portable/monster_proximity.h"
#include "pal4/portable/monster_pursuit_paths.h"
namespace pal4::portable {
// 场景必须提供真实交互、组动作、历史清理、距离触发和战斗；无空成功后备。
class IMonsterPursuitSceneServices {
public:
    virtual ~IMonsterPursuitSceneServices()=default;
    virtual MonsterProximityActor ControlledActor()=0;
    virtual MonsterPursuitTarget Target(std::uint32_t)=0;
    virtual bool PlayerControlBusy()=0;
    virtual bool ProcessObjectInteraction(MonsterBehaviorManagerState&,std::uint32_t manager,std::uint32_t target,float)=0;
    virtual void DispatchCombatPositions(std::uint32_t manager,float)=0;
    virtual void InitializeCombat(std::uint32_t manager,std::int32_t)=0;
    virtual void ClearHistoryRecord(std::uint32_t record)=0;
    virtual void ProcessActionWithScript(std::uint32_t manager,float)=0;
    virtual void VisitArenaObjects(std::uint32_t manager)=0;
};
// 同一实际管理器上的原外层帧、追逐状态机、事件队列、路径和基础运动组合。
// 必须先更新完整空间网格，每帧仅调用此入口；依赖均须活得更久。
class MonsterPursuitFrame final {
public:
    MonsterPursuitFrame(SceneMonsterInstances&,SceneMonsterHandle,MonsterBehaviorManagerState&,
        std::uint32_t manager_token,std::uint32_t source_token,std::uint32_t behavior_child_token,
        MonsterMovementBehavior&,MonsterBaseMotion&,CollisionSurface&,const CollisionMesh&,IMonsterPursuitSceneServices&);
    ~MonsterPursuitFrame();
    MonsterPursuitFrame(const MonsterPursuitFrame&)=delete;
    MonsterPursuitFrame& operator=(const MonsterPursuitFrame&)=delete;
    std::uint32_t AdvanceAfterGrid(float elapsed,std::uint32_t argument);
    bool Failed() const;
    bool Busy() const noexcept;
    bool UsesWorld(const MonsterMovementWorld&) const noexcept;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
