#pragma once
#include "pal4/monster_proximity_checker.h"
#include "pal4/monster_pursuit_collision_speed.h"
#include "pal4/portable/controller.h"
#include "pal4/portable/player.h"

namespace pal4::portable {
// 借用当前玩家和控制器；身份使用现有章节宿主的玩家逻辑 token。
class MonsterPlayerView final {
public:
    MonsterPlayerView(const PlayerRoster&,const ActorController&);
    MonsterProximityActor ControlledActor() const;
    MonsterPursuitTarget Target(std::uint32_t token) const;
    std::array<float,3> TargetForward(std::uint32_t token) const;
    static std::uint32_t Token(std::uint32_t index);
private:
    const PlayerRoster& players_;
    const ActorController& controller_;
};
}
