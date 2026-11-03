#pragma once
#include "pal4/portable/arena.h"
#include "pal4/portable/scene_collision.h"
namespace pal4::portable {
class ISceneEventScripts {
public:
    virtual ~ISceneEventScripts()=default;
    virtual bool Contains(std::string_view world,std::string_view declaration)=0;
    virtual void LoadWorld(std::string_view world)=0;
    virtual std::int32_t Push(std::string_view world,std::string_view declaration)=0;
};
struct SceneEventState {
    std::string name;std::uint32_t type{};bool active{},camera_requested{},secondary_requested{};
    std::size_t movement_shapes{};
};
// 借用arena、碰撞及脚本服务；这些owner必须晚于本对象销毁。
class SceneEvents {
public:
    SceneEvents(const ArenaResources&,SceneCollision&,ISceneEventScripts&);
    ~SceneEvents();
    SceneEvents(const SceneEvents&)=delete;
    SceneEvents& operator=(const SceneEvents&)=delete;
    void ActivateAll();
    bool SetVisible(std::string_view name,bool visible);
    void OnCollision(std::uint32_t resource,bool entering,std::uint32_t previous);
    std::optional<std::size_t> FindInteraction(Point3 position,float vertical_tolerance,float range) const;
    bool TriggerInteraction(std::size_t index);
    std::span<const SceneEventState> States() const;
    void WriteReport(std::ostream&) const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
