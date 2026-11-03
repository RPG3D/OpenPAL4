#include "pal4/portable/monster_proximity.h"
#include "pal4/monster_behavior_manager.h"
#include "pal4/game_vector_math.h"
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>
namespace pal4::portable {
namespace {
void Require(bool b,const char* message){if(!b)throw std::logic_error(message);}
const GameMathTables& Math(){static const auto t=[]{GameMathTables v;(void)v.CalculateSquareRootTable();return v;}();return t;}
}
struct MonsterProximity::Impl final:IMonsterProximityRuntime {
    SceneMonsterInstances& actors;SceneMonsterHandle actor;MonsterBehaviorManagerState& state;
    std::uint32_t token,source;const CollisionMesh& movement;MonsterProximity::ControlledActor controlled;bool busy{};
    Impl(SceneMonsterInstances& a,SceneMonsterHandle h,MonsterBehaviorManagerState& s,std::uint32_t id,std::uint32_t src,const CollisionMesh& m,MonsterProximity::ControlledActor c)
        :actors(a),actor(h),state(s),token(id),source(src),movement(m),controlled(std::move(c)){
        Require(id && id%4==0 && id<=std::numeric_limits<std::uint32_t>::max()-sizeof(s) && src && bool(controlled),"invalid monster proximity dependencies");
        Require(state.words[0x654/4]==source,"monster proximity source is not bound");
        const auto radius=std::bit_cast<float>(actors.Source(actor).fields[1]);Require(std::isfinite(radius),"non-finite monster detection radius");
    }
    float Value(unsigned word) const {return std::bit_cast<float>(state.words[word]);}
    void Validate(){
        Require(state.words[0x654/4]==source && state.words[5] && actors.TransformBound(actor),"monster proximity requires bound resource and source");
        const auto pose=actors.MotionTransform(actor);
        for(unsigned a=0;a<3;++a){Require(Value(43+a)==pose.position[a] && Value(40+a)==pose.rotation[a],"monster proximity transform is stale");Require(std::isfinite(Value(56+a)),"non-finite monster collision center");}
    }
    MonsterProximityActor ControlledActor() override {
        auto value=controlled();if(!value.address || !value.active)return value;
        Require(value.address%4==0 && value.address!=token && std::isfinite(value.half_height) && value.half_height>=0,"invalid controlled actor snapshot");
        for(unsigned a=0;a<3;++a){Require(std::isfinite(value.position[a]) && std::isfinite(value.collision_position[a]),"non-finite controlled actor snapshot");}
        return value;
    }
    double PlanarDistance(float dx,float dz) override {return GameVectorMath::ApproximateMagnitude({dx,0,dz},Math());}
    float DetectionRadius(std::uint32_t config) override {Require(config==source,"monster proximity source changed");return std::bit_cast<float>(actors.Source(actor).fields[1]);}
    std::uint32_t CollisionClump() override {return movement.wall.empty()?0u:1u;}
    bool CollisionBlocked(const std::array<float,6>& endpoints,std::uint32_t clump) override {
        Require(clump==CollisionClump(),"monster proximity wall changed");return clump && SegmentBlocked(movement,{endpoints[0],endpoints[1],endpoints[2]},{endpoints[3],endpoints[4],endpoints[5]});
    }
    void NotifyReturnTargetReached(std::uint32_t id,std::uint32_t position,std::uint32_t argument) override {
        Require(id==token && position==token+0x6e8 && !argument,"invalid monster return transform call");
        actors.SetMotionPosition(actor,{Value(0x6e8/4),Value(0x6ec/4),Value(0x6f0/4)});
    }
    template<class F>bool Run(F f){Require(!busy,"monster proximity is not reentrant");busy=true;struct Guard{bool& b;~Guard(){b=false;}} guard{busy};Validate();return f();}
};
MonsterProximity::MonsterProximity(SceneMonsterInstances& a,SceneMonsterHandle h,MonsterBehaviorManagerState& s,std::uint32_t id,std::uint32_t src,const CollisionMesh& m,ControlledActor c)
    :impl_(std::make_unique<Impl>(a,h,s,id,src,m,std::move(c))){}
MonsterProximity::~MonsterProximity()=default;
bool MonsterProximity::Check(){auto& s=*impl_;return s.Run([&]{return MonsterProximityChecker::Check(s.state,s);});}
bool MonsterProximity::BeyondReturnThreshold(){auto& s=*impl_;return s.Run([&]{return MonsterProximityChecker::BeyondReturnThreshold(s.state,s);});}
bool MonsterProximity::CheckReturnTargetReached(){auto& s=*impl_;return s.Run([&]{return MonsterProximityChecker::CheckReturnTargetReached(s.state,s.token,s);});}
}
