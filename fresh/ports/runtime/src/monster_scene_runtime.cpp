#include "pal4/portable/monster_scene_runtime.h"

#include <stdexcept>
#include <cmath>
#include <set>
#include <utility>

namespace pal4::portable {

MonsterSceneRuntime::MonsterSceneRuntime(std::uint64_t generation,
    std::shared_ptr<ArenaSource> source,
    std::span<const SceneMonster> monsters,
    std::span<const ScenePath> paths,
    MonsterMovementWorld::Random random,
    MonsterMovementLimits limits) {
    resources_ = std::make_unique<MonsterSceneResources>(generation,
        std::move(source), monsters, paths);
    world_ = std::make_unique<MonsterMovementWorld>(generation,
        resources_->Paths(), std::move(random), limits);
    movement_ = std::make_unique<MonsterMovement>(*world_);
}

MonsterSceneRuntime::~MonsterSceneRuntime() = default;

bool MonsterSceneRuntime::Closed() const noexcept {
    return !resources_ && !world_ && !movement_ && !actors_;
}

void MonsterSceneRuntime::RequireOpen() const {
    if (Closed()) throw std::logic_error("monster scene runtime is closed");
}

MonsterSceneResources& MonsterSceneRuntime::Resources() {
    RequireOpen();
    return *resources_;
}

SceneMonsterInstances& MonsterSceneRuntime::Monsters() {
    return Resources().Monsters();
}

MonsterMovementWorld& MonsterSceneRuntime::World() {
    RequireOpen();
    return *world_;
}

MonsterMovement& MonsterSceneRuntime::Movement() {
    RequireOpen();
    return *movement_;
}

bool MonsterSceneRuntime::ActorsBound() const noexcept {
    return actors_ && !actors_closed_ && !actors_->Closed();
}

bool MonsterSceneRuntime::Failed() const noexcept {
    return failed_ || (actors_ && actors_->Failed());
}

std::size_t MonsterSceneRuntime::ActorCount() const noexcept {
    return ActorsBound() ? actors_->Size() : 0;
}

MonsterSceneActor& MonsterSceneRuntime::Actor(std::size_t index) {
    RequireOpen();
    if(!ActorsBound())throw std::logic_error("monster scene actors are not bound");
    return actors_->At(index);
}

void MonsterSceneRuntime::AdoptActors(std::vector<std::unique_ptr<MonsterSceneActor>> actors,
    MonsterSceneFrame::Obstacles obstacles) {
    RequireOpen();
    if(actors_ || actors_closed_)throw std::logic_error("monster scene actors already closed or bound");
    const auto order=Monsters().OriginalTraversalOrder();
    for(const auto& actor:actors) {
        if(!actor)throw std::invalid_argument("monster actor set contains a null owner");
        if(!actor->UsesInstances(Monsters()))
            throw std::invalid_argument("monster actor belongs to another instance owner");
    }
    if(actors.size()!=order.size())throw std::invalid_argument("monster actor set is incomplete");
    for(std::size_t i=0;i<order.size();++i) {
        if(actors[i]->Handle()!=Monsters().Handle(order[i]))
            throw std::invalid_argument("monster actors are not in original MST traversal order");
    }
    actors_=std::make_unique<MonsterSceneActors>(*world_,*movement_,std::move(obstacles),std::move(actors));
}

void MonsterSceneRuntime::Advance(float elapsed,std::uint32_t argument) {
    RequireOpen();
    if(failed_)throw std::logic_error("monster scene runtime failed");
    if(!ActorsBound())throw std::logic_error("monster scene actors are not bound");
    try {
        actors_->Advance(elapsed,argument);
    } catch(...) {
        failed_=true;
        throw;
    }
}

void MonsterSceneRuntime::CreateActors(const MonsterSceneActorBindings& bindings) {
    RequireOpen();
    if(actors_ || actors_closed_ || orientation_)
        throw std::logic_error("monster scene actors already closed or bound");
    if(!bindings.tokens || !bindings.manager_radius || !bindings.identifiers ||
       !bindings.collision || !bindings.obstacles || !std::isfinite(bindings.collision_height) || bindings.collision_height<0)
        throw std::invalid_argument("monster actor production services are incomplete");
    auto orientation=std::make_unique<MonsterOrientation>(*world_,Monsters(),bindings.ground,bindings.orientation_caches);
    std::vector<std::unique_ptr<MonsterSceneActor>> actors;
    const auto order=Monsters().OriginalTraversalOrder();actors.reserve(order.size());
    std::set<std::uint32_t> managers,clumps;
    for(const auto index:order) {
        const auto handle=Monsters().Handle(index);const auto tokens=bindings.tokens(handle);
        if(!tokens.manager || !tokens.clump || !managers.insert(tokens.manager).second || !clumps.insert(tokens.clump).second)
            throw std::invalid_argument("monster actor tokens are zero or duplicated");
        auto radius=bindings.manager_radius;
        actors.push_back(std::make_unique<MonsterSceneActor>(*world_,Monsters(),handle,*orientation,
            [radius,handle]{return radius(handle);},bindings.identifiers,bindings.actor_package,
            tokens.manager,tokens.clump,tokens.mode,tokens.extra,bindings.floor,bindings.collision_mesh,
            bindings.collision_height,bindings.collision,bindings.scene_services));
    }
    AdoptActors(std::move(actors),bindings.obstacles);
    orientation_=std::move(orientation);
}
MonsterSceneRenderResult MonsterSceneRuntime::Render(std::uint32_t camera,IMonsterSceneRenderServices& services){
    RequireOpen();
    if(!camera)throw std::invalid_argument("monster scene render requires a camera");
    if(!ActorsBound() || Failed())throw std::logic_error("monster scene render owner is not bound or has failed");
    try{return actors_->Render(camera,services);}catch(...){failed_=true;throw;}
}

void MonsterSceneRuntime::CloseActors() {
    RequireOpen();
    if(!actors_) {
        if(world_->Stats().helpers!=0)
            throw std::logic_error("monster scene runtime still has unowned helpers");
        actors_closed_=true;
        return;
    }
    actors_->Close();
    if(world_->Stats().helpers!=0)
        throw std::logic_error("monster scene runtime still has helpers after actor close");
    actors_.reset();
    orientation_.reset();
    actors_closed_=true;
}

void MonsterSceneRuntime::Close() {
    if (Closed()) return;
    CloseActors();
    if (world_->Stats().helpers != 0)
        throw std::logic_error("monster scene runtime still has borrowed helpers");
    world_->Close();
    movement_.reset();
    world_.reset();
    resources_.reset();
}

} // namespace pal4::portable
