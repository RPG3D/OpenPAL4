#pragma once
#include "pal4/portable/monster_path_restore.h"
#include "pal4/portable/scene_monster_instances.h"
#include "pal4/portable/scene_pathways.h"
#include <functional>
namespace pal4::portable {
struct MonsterHelperHandle {
    std::uint64_t generation{};std::uint32_t token{};
    bool operator==(const MonsterHelperHandle&) const=default;
};
struct MonsterNeighbor {
    MonsterHelperHandle helper;SceneMonsterHandle owner;
    std::array<float,3> position{};float distance_squared{};bool member{};
};
struct MonsterMovementLimits {std::size_t max_helpers=65535;std::size_t mapped_bytes=64u*1024*1024;};
struct MonsterMovementWorldStats {
    std::size_t helpers{},members{},associations{},spatial_nodes{},blocks{},mapped_bytes{};bool closed{};
};
// Owns helper images, original grid/member arrays and spatial associations.
// Path owner is retained. Single game thread; generation changes on scene reload.
// Rand is required and consumes the caller's original shared stream (0..32767).
class MonsterMovementWorld final:public IMonsterPathRestoreWorld {
public:
    using Random=std::function<std::uint32_t()>;
    MonsterMovementWorld(std::uint64_t generation,std::shared_ptr<ScenePathways>,Random,
        MonsterMovementLimits limits={});
    ~MonsterMovementWorld() override;
    MonsterMovementWorld(const MonsterMovementWorld&)=delete;
    MonsterMovementWorld& operator=(const MonsterMovementWorld&)=delete;
    MonsterHelperHandle CreateEndpoint(SceneMonsterHandle owner,std::uint32_t identifier);
    MonsterHelperHandle CreateRandom(SceneMonsterHandle owner,std::uint32_t identifier);
    BehaviorPathEndpointState& Helper(MonsterHelperHandle);
    SceneMonsterHandle Owner(MonsterHelperHandle) const;
    bool Registered(MonsterHelperHandle) const;
    bool IsRandom(MonsterHelperHandle) const;
    bool Contains(MonsterHelperHandle) const;
    using FrameUpdate=std::function<void(MonsterHelperHandle,float)>;
    // Original member traversal, in registration order. The callback may query
    // and move helpers but cannot create/delete/rebind members during traversal.
    // All grid callbacks finish before the caller advances actor behavior queues.
    void UpdateMembers(float seconds,const FrameUpdate&);
    bool InGridFrame() const noexcept;
    // Normal member removal invokes the original deleting release callback.
    void DestroyHelper(MonsterHelperHandle);
    std::vector<MonsterNeighbor> QuerySphere(const std::array<float,3>& center,float radius);
    MonsterMovementWorldStats Stats() const;
    void Close();
    // Tokens are local to this world, not persistent host addresses.
    ScenePathHandle PathHandle(std::uint32_t path_token) const;
    // Original PathwayGrid queries through the retained ScenePathways owner.
    // Accept primary path tokens only; secondary interfaces are caller adapters.
    float PathClosest(std::uint32_t path_token,const std::array<float,3>& point);
    std::array<float,3> PathCoordinates(std::uint32_t path_token,float distance);
    ScenePathProjection PathProject(std::uint32_t path_token,const std::array<float,3>& point,
        const std::array<float,3>& initial_direction);
    std::uint32_t ReadWord(std::uint32_t address) override;
    void Bind(BehaviorPathEndpointState&,std::uint32_t wrapper) override;
    std::uint32_t UpdatePosition(std::uint32_t association,const BehaviorVector3&) override;
    std::uint32_t Rand() override;
    std::uint32_t FindPath(std::string_view) override;
    std::uint32_t GridParameter() override;
    void AddElement(BehaviorPathEndpointState&) override;
    // Compensation only: original list removal without deleting the helper.
    void RemoveElement(BehaviorPathEndpointState&) noexcept override;
    void ReleaseAssociation(BehaviorPathEndpointState&) noexcept override;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
