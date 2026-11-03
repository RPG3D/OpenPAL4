#pragma once

#include "pal4/portable/monster_movement.h"
#include "pal4/portable/monster_scene_actors.h"
#include "pal4/portable/monster_scene_resources.h"

#include <cstddef>
#include <memory>
#include <vector>

namespace pal4::portable {

struct MonsterSceneActorTokens {
    std::uint32_t manager{},clump{},mode{},extra{};
};
// Borrowed production services must outlive CloseActors(). No default random,
// collision, radius, token, activation or animation policy is supplied here.
struct MonsterSceneActorBindings {
    PackageReader& actor_package;
    SceneMovement& ground;
    MonsterOrientationCaches& orientation_caches;
    CollisionSurface& floor;
    const CollisionMesh& collision_mesh;
    IMonsterPursuitSceneServices& scene_services;
    float collision_height;
    std::function<MonsterSceneActorTokens(SceneMonsterHandle)> tokens;
    std::function<float(SceneMonsterHandle)> manager_radius;
    MonsterMovementBehavior::Identifier identifiers;
    MovementCollisionQuery collision;
    MonsterSceneFrame::Obstacles obstacles;
};

// Owns the real scene resources and the path/grid services that depend on
// them. Actors may borrow these services, so callers must close actors first.
class MonsterSceneRuntime final {
public:
    MonsterSceneRuntime(std::uint64_t generation,
        std::shared_ptr<ArenaSource> source,
        std::span<const SceneMonster> monsters,
        std::span<const ScenePath> paths,
        MonsterMovementWorld::Random random,
        MonsterMovementLimits limits = {});
    ~MonsterSceneRuntime();

    MonsterSceneRuntime(const MonsterSceneRuntime&) = delete;
    MonsterSceneRuntime& operator=(const MonsterSceneRuntime&) = delete;

    [[nodiscard]] bool Closed() const noexcept;
    [[nodiscard]] MonsterSceneResources& Resources();
    [[nodiscard]] SceneMonsterInstances& Monsters();
    [[nodiscard]] MonsterMovementWorld& World();
    [[nodiscard]] MonsterMovement& Movement();
    [[nodiscard]] bool ActorsBound() const noexcept;
    [[nodiscard]] bool Failed() const noexcept;
    [[nodiscard]] std::size_t ActorCount() const noexcept;
    [[nodiscard]] MonsterSceneActor& Actor(std::size_t index);

    // Takes the complete actor set in original MST traversal order. The group
    // owns helper actors and the shared scene frame until CloseActors().
    void AdoptActors(std::vector<std::unique_ptr<MonsterSceneActor>> actors,
        MonsterSceneFrame::Obstacles obstacles);
    // Builds every original instance and owns its orientation dependency before
    // publishing the complete group. Failure releases the partial actor set.
    void CreateActors(const MonsterSceneActorBindings&);
    void Advance(float elapsed,std::uint32_t argument);
    MonsterSceneRenderResult Render(std::uint32_t camera,IMonsterSceneRenderServices&);
    // Call before releasing package, collision, orientation, or scene-service
    // owners borrowed by MonsterSceneActor; then Close() the runtime itself.
    void CloseActors();

    // Close after all actors/helpers borrowing World() have been released.
    // A failed close leaves every owned service available for cleanup/retry.
    void Close();

private:
    void RequireOpen() const;

    std::unique_ptr<MonsterSceneResources> resources_;
    std::unique_ptr<MonsterMovementWorld> world_;
    std::unique_ptr<MonsterMovement> movement_;
    std::unique_ptr<MonsterOrientation> orientation_;
    std::unique_ptr<MonsterSceneActors> actors_;
    bool actors_closed_{},failed_{};
};

} // namespace pal4::portable
