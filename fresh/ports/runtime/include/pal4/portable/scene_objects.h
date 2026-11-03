#pragma once
#include "pal4/portable/arena.h"
#include "pal4/portable/actor_playback.h"
#include "pal4/portable/minimap.h"
#include "pal4/portable/scene_collision.h"
namespace pal4::portable {
class EffectPlayback;
class ISceneEventScripts;
using GobMemberReader=std::function<std::optional<std::vector<std::uint8_t>>(std::string_view,std::string_view)>;
struct SceneObjectState {
    std::string name;
    std::uint32_t type{},visible{},active{},marker{},uses{};
    std::optional<std::size_t> actor;
    std::array<float,3> position{},rotation{};float scale{1};
    bool world_attached{},movement_registered{},camera_registered{};
    bool geometry_bound{},material_bound{},render_bound{};
    bool disappearance_requested{};
    bool movement_backend_bound{};std::size_t movement_shapes{};
};
// 借用当前ArenaResources及小地图。所有脚本修改落在同一可变对象集合。
class SceneObjects {
public:
    SceneObjects(const ArenaResources& arena,SceneMinimap& minimap,GobMemberReader read,EffectPlayback* effects=nullptr);
    ~SceneObjects();
    SceneObjects(const SceneObjects&)=delete;
    SceneObjects& operator=(const SceneObjects&)=delete;
    using CollisionEligibility=std::function<bool(std::size_t object,std::size_t atomic)>;
    void BindMovement(SceneCollision& collision,CollisionEligibility eligibility);
    void UnbindMovement();
    void ActivateInitialEffects();
    bool SetVisible(std::string_view name,bool visible);
    bool IsVisible(std::string_view name) const;
    bool ResetConfiguration(std::string_view name);
    bool SetResearch(std::string_view name,bool enabled);
    std::optional<std::size_t> FindInteraction(Point3 position,float vertical_radius);
    bool InteractionEnabled(std::size_t index) const;
    bool InteractionInRange(std::size_t index,Point3 position,float vertical_radius) const;
    std::int32_t InteractionEvent(std::size_t index) const;
    bool Interact(std::size_t index,ISceneEventScripts& scripts);
    void Advance(float seconds);
    std::span<const SceneObjectState> States() const;
    const ActorPlayback* Playback(std::size_t index) const;
    bool Drawable(std::size_t index) const;
    void Sample(std::size_t index,std::vector<Vertex>& vertices,std::vector<Matrix>* frames=nullptr);
    void WriteReport(std::ostream& output) const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
