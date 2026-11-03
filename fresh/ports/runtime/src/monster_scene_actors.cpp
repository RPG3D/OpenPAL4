#include "pal4/portable/monster_scene_actors.h"
#include <exception>
#include <stdexcept>
#include <utility>

namespace pal4::portable {
MonsterSceneActors::MonsterSceneActors(MonsterMovementWorld& world,MonsterMovement& movement,MonsterSceneFrame::Obstacles obstacles,
    std::vector<std::unique_ptr<MonsterSceneActor>> actors)
    :scheduler_(world,movement,std::move(obstacles)),actors_(std::move(actors)) {
    frames_.reserve(actors_.size());
    for(auto& actor:actors_){
        if(!actor || !actor->CanAttachScene() || !actor->Frame().UsesWorld(world))throw std::invalid_argument("invalid monster scene actor owner");
        frames_.push_back(&actor->Frame());
    }
    for(auto& actor:actors_)actor->AttachScene(&closing_);
}
MonsterSceneActors::~MonsterSceneActors(){try{Close();}catch(...) {}}
MonsterSceneActor& MonsterSceneActors::At(std::size_t index){
    if(closed_)throw std::logic_error("monster scene is closed");
    return *actors_.at(index);
}
void MonsterSceneActors::Advance(float elapsed,std::uint32_t argument){
    if(closed_ || busy_ || render_failed_)throw std::logic_error("monster scene unavailable");
    busy_=true;
    try{scheduler_.Advance(elapsed,argument,frames_);busy_=false;}
    catch(...){busy_=false;throw;}
}
MonsterSceneRenderResult MonsterSceneActors::Render(std::uint32_t camera,IMonsterSceneRenderServices& services){
    if(closed_ || busy_ || Failed() || !camera)throw std::logic_error("monster scene render unavailable");
    busy_=true;struct Guard{bool& busy;~Guard(){busy=false;}} guard{busy_};
    MonsterSceneRenderResult total;
    try{
        for(auto& actor:actors_){
            const auto value=actor->Render(camera,services);
            total.active_objects+=value.active_objects;total.render_successes+=value.render_successes;
            total.visible_objects+=value.visible_objects;total.animation_calls+=value.animation_calls;
            total.character_updates+=value.character_updates;
        }
    }catch(...){render_failed_=true;throw;}
    return total;
}
void MonsterSceneActors::Close(){
    if(closed_)return;
    if(busy_)throw std::logic_error("monster scene close during frame");
    frames_.clear();closing_=true;std::exception_ptr error;
    while(!actors_.empty()){
        try{actors_.back()->Close();}catch(...){if(!error)error=std::current_exception();}
        actors_.pop_back();
    }
    closed_=true;
    if(error)std::rethrow_exception(error);
}
}
