#pragma once
#include "pal4/portable/scene_data.h"
#include "pal4/portable/actor.h"
#include "pal4/portable/object_collision.h"
#include "pal4/portable/world.h"
#include <functional>

namespace pal4::portable {
struct ArenaDescription {
    std::uint32_t id{}, type{}, earthball{};
    std::string world, scene, title;
    std::vector<std::string> camera_files, environment_files;
    bool object_collision_cache{},event_collision_cache{};
};
struct ArenaActor {
    std::string name, animation;
    Actor actor;
    std::array<float,3> position{}, rotation{};
    float scale{1};
    std::uint32_t state{};
    std::optional<std::uint32_t> gob_type;
    std::optional<ObjectCollisionGeometry> collision;
};
struct ArenaResources {
    ArenaDescription description;
    struct Environment {std::string kind;Scene mesh;};
    std::vector<Environment> environment;
    Scene world;
    CollisionMesh collision;
    SceneCatalog catalog;
    std::vector<std::optional<ObjectCollisionGeometry>> event_collisions;
    std::vector<ArenaActor> actors;
    bool monsters_verified_empty{},monster_pursuit_stopped{};
};
using ArenaAssetReader=std::function<std::vector<std::uint8_t>(std::string_view)>;
ArenaDescription ReadArenaDescription(std::istream& input);
ArenaResources LoadArenaResources(std::string_view world,std::string_view scene,const ArenaAssetReader& read,
                                  const std::function<CollisionMesh()>& original_collision={},bool original_events=false,
                                  const std::function<ObjectCollisionGeometry(const SceneGob&)>& original_object={},
                                  const std::function<ArenaDescription()>& original_description={},
                                  const std::function<Scene(std::string_view)>& original_environment={},
                                  const std::function<Scene()>& original_world={},
                                  const std::function<Actor(const SceneNpc&)>& original_npc={},
                                  const std::function<Actor(const SceneGob&)>& original_gob={});
void WriteArenaReport(std::ostream& output,const ArenaResources& arena);
void ValidateArenaGameplaySupport(const ArenaResources& arena);
} // namespace pal4::portable
