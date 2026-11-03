#pragma once
#include "pal4/portable/scene_npcs.h"
#include "pal4/portable/scene_objects.h"
#include "pal4/portable/scene_events.h"
#include "pal4/pal_actor_control_game_event.h"
namespace pal4::portable {
// 借用当前场景与提示服务；先销毁控制器再释放这些owner。
class SceneInteraction {
public:
    SceneInteraction(const ArenaResources&,SceneNpcs&,SceneObjects&,SceneEvents&,
        ISceneEventScripts&,pal4::IActorControlGameEventRuntime& hints,
        SceneNpcs::InteractionEligibility npc_eligibility);
    ~SceneInteraction();
    SceneInteraction(const SceneInteraction&)=delete;
    SceneInteraction& operator=(const SceneInteraction&)=delete;
    // pressed为按键电平；只在按下沿搜索，释放沿清除原words20～22。
    bool Advance(bool pressed,bool blocked,Point3 position,float bind_radius,float world_radius,std::uint32_t actor_index);
    void Reset(bool pressed=false);
    void WriteReport(std::ostream&) const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
