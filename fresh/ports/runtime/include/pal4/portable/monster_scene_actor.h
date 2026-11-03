#pragma once
#include "pal4/portable/monster_pursuit_frame.h"
#include "pal4/portable/actor_resource_frame.h"
#include "pal4/portable/object_memory.h"
#include "pal4/portable/monster_encounter_storage.h"
#include "pal4/collision_actor.h"

namespace pal4::portable {
class IMonsterSceneRenderServices:public ICollisionActorCallbackRuntime {
public:
    virtual bool FrustumTestSphere(SceneMonsterHandle,std::uint32_t camera,std::array<float,4> sphere)=0;
    virtual void DrawClump(SceneMonsterHandle,std::uint32_t clump)=0;
};
struct MonsterSceneRenderResult {
    std::uint32_t active_objects{},render_successes{},visible_objects{},animation_calls{},character_updates{};
};
// 所有引用和回调均由场景拥有并须活得更久；不推测初始动作或玩法服务。
// 单线程使用，禁止在帧更新及其回调期间关闭或销毁。
class MonsterSceneActor final {
public:
    MonsterSceneActor(MonsterMovementWorld&, SceneMonsterInstances&, SceneMonsterHandle,
        MonsterOrientation&, MonsterMovementBehavior::Radius, MonsterMovementBehavior::Identifier,
        PackageReader&, std::uint32_t manager, std::uint32_t clump, std::uint32_t mode, std::uint32_t extra,
        CollisionSurface&, const CollisionMesh&, float height, MovementCollisionQuery,
        IMonsterPursuitSceneServices&);
    ~MonsterSceneActor();
    MonsterSceneActor(const MonsterSceneActor&)=delete;
    MonsterSceneActor& operator=(const MonsterSceneActor&)=delete;
    MonsterBehaviorManagerState& State();
    MonsterMovementBehavior& Behavior();
    MonsterBaseMotion& Motion();
    MonsterPursuitFrame& Frame();
    MonsterEncounterStorage& Encounter();
    std::uint32_t ManagerToken() const noexcept;
    SceneMonsterHandle Handle() const noexcept;
    bool UsesInstances(const SceneMonsterInstances&) const noexcept;
    std::string Label() const;
    // Calls shared BaseObjectActivity/Resource/Animation/CollisionActor rules.
    // Required platform services provide real culling, drawing and callbacks.
    MonsterSceneRenderResult Render(std::uint32_t camera,IMonsterSceneRenderServices&);
    // 调用方可显式检查解绑或原成员释放错误；关闭后不可继续使用。
    void Close();
private:
    friend class MonsterSceneActors;
    bool CanAttachScene() const noexcept;
    void AttachScene(const bool* allow_close);
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
