#pragma once
#include "pal4/portable/actor_playback.h"
#include "pal4/portable/world.h"
#include <memory>
namespace pal4::portable {
struct ArenaResources;
struct SceneNpcState {
    std::string name,resource;
    std::size_t actor{};
    Point3 position{},rotation{};
    std::uint32_t active{};
    bool world_attached{},movement_registered{},camera_registered{},geometry_bound{},material_bound{},render_bound{},collision_bound{};
};
class SceneNpcs {
public:
    SceneNpcs(const ArenaResources& arena,ActorMemberReader read);
    ~SceneNpcs();
    SceneNpcs(const SceneNpcs&)=delete;
    SceneNpcs& operator=(const SceneNpcs&)=delete;
    bool SetVisible(std::string_view name,bool visible);
    bool HasAction(std::size_t index,std::string_view action) const;
    ActorPlayback* Playback(std::size_t index);
    using InteractionEligibility=std::function<bool(std::size_t,std::size_t)>;
    std::optional<std::size_t> FindInteraction(Point3 position,const InteractionEligibility& eligibility={}) const;
    bool InteractionEnabled(std::size_t index) const;
    std::string_view InteractionScript(std::size_t index) const;
    float InteractionRadius(std::size_t index) const;
    void Advance(float seconds);
    void CleanupWeaponEffects(std::size_t index);
    float MovementRadius(std::size_t index) const;
    void SetMotionPosition(std::size_t index,Point3);
    void SetMotionRotation(std::size_t index,Point3);
    void BindScriptMotion();
    void UnbindScriptMotion() noexcept;
    std::span<const SceneNpcState> States() const;
    bool Drawable(std::size_t index) const;
    void Sample(std::size_t index,std::vector<Vertex>& output,std::vector<Matrix>* frames=nullptr);
    void WriteReport(std::ostream& output,bool include_source=true) const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
