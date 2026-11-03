#include "pal4/portable/monster_pursuit_paths.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>
namespace pal4::portable {
namespace {void Require(bool b,const char* message){if(!b)throw std::logic_error(message);}}
struct MonsterPursuitPaths::Impl final:IMonsterPursuitReturnRuntime {
    SceneMonsterInstances& actors;SceneMonsterHandle actor;MonsterBehaviorManagerState& state;std::uint32_t token,source;
    MonsterBaseMotion& motion;CollisionSurface& floor;TargetResolver target;Reset reset;bool busy{},failed{};
    Impl(SceneMonsterInstances& a,SceneMonsterHandle h,MonsterBehaviorManagerState& s,std::uint32_t id,std::uint32_t src,MonsterBaseMotion& m,CollisionSurface& f,TargetResolver t,Reset r)
        :actors(a),actor(h),state(s),token(id),source(src),motion(m),floor(f),target(std::move(t)),reset(std::move(r)){
        Require(source && bool(target) && bool(reset),"monster pursuit services are not bound");Validate();
    }
    void Validate(){
        motion.ValidateBinding(actors,actor,state,token,floor);Require(state.words[0x654/4]==source,"monster pursuit source changed");
        const auto pose=actors.MotionTransform(actor);for(unsigned a=0;a<3;++a){Require(std::bit_cast<float>(state.words[43+a])==pose.position[a],"stale monster pursuit position");}
    }
    MonsterPursuitTarget Target(std::uint32_t id) override {
        Require(id && id%4==0 && id!=token,"invalid monster pursuit target");auto value=target(id);
        if(value.active){for(auto v:value.position){Require(std::isfinite(v),"non-finite monster pursuit target");}}return value;
    }
    std::uint32_t FindPursuitPath(float,Point3& point) override {for(auto v:point){Require(std::isfinite(v),"non-finite monster return target");}return floor.Snap(point);}
    bool ApplyPursuitPath(std::uint32_t id,const Point3& from,const Point3& to,std::uint32_t face,std::uint32_t argument) override {
        Require(id==token+0x564 && !argument,"invalid monster pursuit path call");return motion.Start(from,to,face);
    }
    float MaxPursuitSpeed(std::uint32_t src,float fallback) override {
        Require(src==source,"monster pursuit configuration mismatch");const auto* attribute=actors.Source(actor).cache.Root().FindAttribute("AAB_MST_ESSBEH_max_pursuit_speed");
        const auto speed=attribute?attribute->ToFloat(fallback):fallback;Require(std::isfinite(speed) && speed>=0,"invalid monster pursuit speed");return speed;
    }
    std::uint32_t SetPursuitSpeed(std::uint32_t id,float speed) override {Require(id==token+0x564,"invalid pursuit speed target");motion.SetSpeed(speed);return state.words[(0x564+36)/4];}
    std::uint32_t ResetPursuit(MonsterBehaviorManagerState& s,std::uint32_t id) override {Require(&s==&state && id==token,"invalid pursuit reset call");return reset();}
    template<class F>auto Run(float elapsed,F operation){
        Require(!busy && !failed,"monster pursuit paths are not ready");Require(std::isfinite(elapsed) && elapsed>=0,"invalid pursuit frame time");Validate();busy=true;
        struct Guard{bool& value;~Guard(){value=false;}} guard{busy};try{return operation();}catch(...){failed=true;throw;}
    }
};
MonsterPursuitPaths::MonsterPursuitPaths(SceneMonsterInstances& a,SceneMonsterHandle h,MonsterBehaviorManagerState& s,std::uint32_t id,std::uint32_t src,MonsterBaseMotion& m,CollisionSurface& f,TargetResolver t,Reset r)
    :impl_(std::make_unique<Impl>(a,h,s,id,src,m,f,std::move(t),std::move(r))){}
MonsterPursuitPaths::~MonsterPursuitPaths()=default;
bool MonsterPursuitPaths::Process(float elapsed,std::uint32_t target){auto& s=*impl_;return s.Run(elapsed,[&]{return MonsterPursuitCollisionSpeed::Process(s.state,s.token,elapsed,target,s);});}
std::uint32_t MonsterPursuitPaths::EnterReturn(float elapsed){auto& s=*impl_;return s.Run(elapsed,[&]{return MonsterPursuitReturnState::Enter(s.state,s.token,elapsed,s);});}
bool MonsterPursuitPaths::Failed() const {return impl_->failed;}
}
