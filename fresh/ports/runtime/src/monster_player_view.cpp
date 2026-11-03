#include "pal4/portable/monster_player_view.h"
#include "pal4/portable/world.h"
#include "pal4/player_manager_object.h"
#include <cmath>
#include <stdexcept>

namespace pal4::portable {
namespace {
std::uint32_t PlayerIndex(std::uint32_t token,const PlayerRoster& players){
    if(token<0x10000u || (token-0x10000u)%PlayerManagerObject::kElementBytes)throw std::invalid_argument("invalid monster target player token");
    const auto index=(token-0x10000u)/PlayerManagerObject::kElementBytes;
    if(index>=4 || !players.Prepared())throw std::invalid_argument("unknown monster target player");
    return index;
}
}
MonsterPlayerView::MonsterPlayerView(const PlayerRoster& players,const ActorController& controller)
    :players_(players),controller_(controller) {}
std::uint32_t MonsterPlayerView::Token(std::uint32_t index) {
    if(index>=4)throw std::invalid_argument("invalid monster target player index");
    return 0x10000u+index*PlayerManagerObject::kElementBytes;
}
MonsterProximityActor MonsterPlayerView::ControlledActor() const {
    if(!controller_.Initialized() || !controller_.State().actor)return {};
    if(!players_.Prepared())throw std::logic_error("controlled player roster is not ready");
    const auto index=*controller_.State().actor;
    const auto& player=players_.Slots().at(index);
    MonsterProximityActor result{};result.address=Token(index);result.active=player.active;
    if(!result.active)return result;
    if(!player.actor)throw std::logic_error("controlled player model is missing");
    const auto sphere=ActorBindSphere(*player.actor);
    if(!sphere)throw std::logic_error("controlled player bounds are missing");
    const auto matrix=InstanceMatrix(player.position,player.rotation);
    result.position=player.position;result.half_height=(*sphere)[3];
    for(unsigned a=0;a<3;++a){
        result.collision_position[a]=matrix[a]*(*sphere)[0]+matrix[4+a]*(*sphere)[1]+matrix[8+a]*(*sphere)[2]+matrix[12+a];
        if(!std::isfinite(result.position[a]) || !std::isfinite(result.collision_position[a]))throw std::logic_error("invalid controlled player transform");
    }
    return result;
}
MonsterPursuitTarget MonsterPlayerView::Target(std::uint32_t token) const {
    const auto index=PlayerIndex(token,players_);
    const auto& player=players_.Slots()[index];
    if(!player.active)return {};
    for(float value:player.position)if(!std::isfinite(value))throw std::logic_error("invalid monster target position");
    return {true,player.position};
}
std::array<float,3> MonsterPlayerView::TargetForward(std::uint32_t token) const {
    const auto index=PlayerIndex(token,players_);
    const auto& player=players_.Slots()[index];
    if(!player.active || !player.actor)throw std::logic_error("monster target player is not active");
    const auto matrix=InstanceMatrix(player.position,player.rotation);
    return {matrix[8],matrix[9],matrix[10]};
}
}
