#include "pal4/portable/actor_resource_frame.h"
#include "pal4/game_resource_manager.h"
#include "actor_resource_assets.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace pal4::portable {
namespace {void Require(bool b,const char* message){if(!b)throw std::logic_error(message);}}
struct ActorResourceFrame::Impl final:IGameResourceFrameRuntime,IRwDictSchemaUpdateRuntime {
    SceneMonsterInstances& actors;SceneMonsterHandle actor;MonsterBehaviorManagerState& state;ActorResourceFrameStats stats;bool busy{},failed{};
    Impl(SceneMonsterInstances& a,SceneMonsterHandle h,MonsterBehaviorManagerState& s,PackageReader& package):actors(a),actor(h),state(s){
        const auto& model=actors.Source(h).resource;Require(!actors.Model(h).frames.empty() && state.words[5],"actor resource frame requires a loaded body");
        detail::RequireStaticActorResources(package,model);Validate();
    }
    void Validate(){
        (void)actors.Model(actor);Require(state.words[5] && !state.words[37] && !state.words[38],"unbound actor dynamic resource schema");
        Require(!state.words[11] && !state.words[12] && !state.words[13],"unbound actor shadow resource state");
    }
    void RenderCameraScene(std::uint32_t*,float,std::uint32_t*) override {throw std::logic_error("unexpected actor shadow submission");}
    std::uint32_t ReadWord(std::uint32_t) override {throw std::logic_error("unexpected actor UV material read");}
    std::uint32_t ListBegin(std::uint32_t) override {throw std::logic_error("unexpected actor UV list");}
    std::uint32_t ListEnd(std::uint32_t) override {throw std::logic_error("unexpected actor UV list");}
    void AdvanceUVAnim(std::uint32_t,float) override {throw std::logic_error("unbound actor UV animation");}
    void ApplyUVAnim(std::uint32_t) override {throw std::logic_error("unbound actor UV animation");}
};
ActorResourceFrame::ActorResourceFrame(SceneMonsterInstances& a,SceneMonsterHandle h,MonsterBehaviorManagerState& s,PackageReader& package):impl_(std::make_unique<Impl>(a,h,s,package)){}
ActorResourceFrame::~ActorResourceFrame()=default;
bool ActorResourceFrame::Update(MonsterBehaviorManagerState& state,float elapsed){
    auto& s=*impl_;Require(!s.busy && !s.failed && &state==&s.state,"invalid or failed actor resource frame");Require(std::isfinite(elapsed) && elapsed>=0,"invalid actor resource frame time");s.busy=true;
    struct Guard{bool& value;~Guard(){value=false;}} guard{s.busy};try{
        s.Validate();GameResourceManager resource;std::copy_n(state.words.begin(),resource.words.size(),resource.words.begin());
        ++s.stats.calls;const bool updated=resource.UpdateAndRenderClump(elapsed,s,s);std::copy(resource.words.begin(),resource.words.end(),state.words.begin());
        if(updated)++s.stats.updated;else ++s.stats.skipped;return updated;
    }catch(...){s.failed=true;throw;}
}
ActorResourceFrameStats ActorResourceFrame::Stats() const {return impl_->stats;}
bool ActorResourceFrame::Failed() const {return impl_->failed;}
}
