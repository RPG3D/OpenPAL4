#pragma once
#include "pal4/portable/controller.h"
#include "pal4/portable/player_action.h"
#include "pal4/portable/player_placement.h"
#include "pal4/portable/scene_collision.h"
namespace pal4::portable {
// 借用当前场景的碰撞与角色owner；场景切换前必须销毁。
class PlayerFreeMovement {
public:
    PlayerFreeMovement(PlayerRoster& players,ActorController& controller,PlayerActions& actions,
        SceneMovement& floor,SceneCollision& collision,IPlayerPlacementServices& services,std::function<void()> camera_access);
    ~PlayerFreeMovement();
    PlayerFreeMovement(const PlayerFreeMovement&)=delete;
    PlayerFreeMovement& operator=(const PlayerFreeMovement&)=delete;
    bool Advance(Point3 direction,float elapsed);
    // Modern input bypasses tank/key dispatch, reusing the same vector solver.
    // Like the injection adapter, publish the minimap after accepted movement.
    bool AdvanceModern(Point3 direction,float elapsed);
    bool AdvanceDiscrete(std::uint32_t command,float elapsed,bool any_input);
    void WriteReport(std::ostream& output) const;
private:
    bool AdvanceVector(Point3 direction,float elapsed,bool modern);
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
