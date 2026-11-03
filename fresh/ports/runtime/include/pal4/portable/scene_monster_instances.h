#pragma once
#include "pal4/portable/actor_playback.h"
#include "pal4/portable/scene_data.h"
#include "pal4/portable/monster_behavior_definition.h"
#include <functional>
#include <memory>

namespace pal4::portable {
struct SceneMonsterHandle {
    std::uint64_t generation{};
    std::uint32_t index{};
    bool operator==(const SceneMonsterHandle&) const=default;
};
struct SceneMonsterTransform {std::array<float,3> position{},rotation{};};
enum class MonsterTransformChange {Position,Rotation};

// 拥有模型、原始记录快照和每实例播放状态。世界注册/行为由场景层接入。
class SceneMonsterInstances {
public:
    using ModelLoader=std::function<Actor(std::string_view)>;
    SceneMonsterInstances(std::uint64_t generation,std::span<const SceneMonster> records,
        const ModelLoader& load,ActorMemberReader read);
    ~SceneMonsterInstances();
    SceneMonsterInstances(const SceneMonsterInstances&)=delete;
    SceneMonsterInstances& operator=(const SceneMonsterInstances&)=delete;
    std::size_t Size() const;
    std::size_t ModelCount() const;
    // Mirrors MonsterInfoManager::ForEach: ascending unique MST name order.
    std::vector<std::size_t> OriginalTraversalOrder() const;
    SceneMonsterHandle Handle(std::size_t index) const;
    std::optional<SceneMonsterHandle> Find(std::string_view name) const;
    const SceneMonster& Source(SceneMonsterHandle) const;
    const Actor& Model(SceneMonsterHandle) const;
    const MonsterBehaviorDefinition& Behavior(SceneMonsterHandle) const;
    std::array<float,4> LocalSphere(SceneMonsterHandle) const;
    std::optional<ActorLocalBox> LocalBox(SceneMonsterHandle) const;
    Matrix InstanceTransform(SceneMonsterHandle) const;
    SceneMonsterTransform MotionTransform(SceneMonsterHandle) const;
    // 可绑定一个资源所有者，在发布前提交；允许规范化当前字段，不能修改另一字段。
    using TransformCommit=std::function<void(SceneMonsterTransform&,MonsterTransformChange)>;
    std::uint64_t BindTransform(SceneMonsterHandle,const SceneMonsterTransform& initial,TransformCommit);
    void UnbindTransform(SceneMonsterHandle,std::uint64_t binding);
    bool TransformBound(SceneMonsterHandle) const;
    bool OwnsTransformBinding(SceneMonsterHandle,std::uint64_t binding) const;
    void SetMotionPosition(SceneMonsterHandle,const std::array<float,3>&);
    void SetMotionRotation(SceneMonsterHandle,const std::array<float,3>&);
    bool SelectAction(SceneMonsterHandle,std::string_view action);
    void AdvanceAnimation(SceneMonsterHandle,float seconds);
    std::string_view Action(SceneMonsterHandle) const;
    float AnimationTime(SceneMonsterHandle) const;
    // Shared engine adapters must use the instance's actual playback owner.
    ActorPlayback& Playback(SceneMonsterHandle);
    void Sample(SceneMonsterHandle,std::vector<Vertex>& local_vertices);
    void WriteReport(std::ostream&) const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace pal4::portable
