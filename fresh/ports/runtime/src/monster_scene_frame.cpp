#include "pal4/portable/monster_scene_frame.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace pal4::portable {
MonsterSceneFrame::MonsterSceneFrame(MonsterMovementWorld& world,MonsterMovement& movement,Obstacles obstacles)
    :world_(world),movement_(movement),obstacles_(std::move(obstacles)) {
    if(!obstacles_ || !movement_.UsesWorld(world_))throw std::invalid_argument("monster scene requires matching movement and obstacle candidates");
}
void MonsterSceneFrame::Advance(float elapsed,std::uint32_t argument,std::span<MonsterPursuitFrame* const> actors) {
    if(failed_ || busy_)throw std::logic_error("monster scene frame unavailable");
    if(!std::isfinite(elapsed) || elapsed<0)throw std::invalid_argument("invalid monster scene frame time");
    for(std::size_t i=0;i<actors.size();++i)
        if(!actors[i] || !actors[i]->UsesWorld(world_) || std::find(actors.begin(),actors.begin()+i,actors[i])!=actors.begin()+i)
            throw std::invalid_argument("invalid or duplicate monster scene actor");
    busy_=true;
    try {
        world_.UpdateMembers(elapsed,[&](auto helper,float dt){
            const auto candidates=obstacles_(helper);
            (void)movement_.Update(helper,candidates,dt);
        });
        for(auto* actor:actors)(void)actor->AdvanceAfterGrid(elapsed,argument);
        busy_=false;
    } catch(...) {busy_=false;failed_=true;throw;}
}
}
