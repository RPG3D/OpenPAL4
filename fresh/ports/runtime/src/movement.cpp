#include "pal4/portable/movement.h"
#include "pal4/actor_vector_movement.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace pal4::portable {
namespace {
using Vec=pal4::ActorControlVec3;
Point3 Point(Vec v) { return {v.x,v.y,v.z}; }
class GeometryMovement final : public pal4::IActorVectorMovementRuntime {
public:
    GeometryMovement(MovementState& state,const MovementConfig& config,const CollisionMesh& mesh,Point3 input)
        : state_(state),config_(config),mesh_(mesh),input_(input) {}
    void Begin() override {}
    Vec Position() override { return {state_.position[0],state_.position[1],state_.position[2]}; }
    float Speed() override { return config_.speed; }
    Vec Forward() override {
        const float angle=state_.yaw*.017453292519943295f;
        return {std::sin(angle),0,std::cos(angle)};
    }
    pal4::ActorMovementRadii PrepareBounds(Vec position) override {
        const double height=config_.height,half=height*.5;
        lower_={position.x,static_cast<float>(half*.75+position.y),position.z};
        upper_={position.x,static_cast<float>(half*1.25+position.y),position.z};
        radii_={static_cast<float>(height*double(std::bit_cast<float>(0x3eb33333u))),
                static_cast<float>(height*double(std::bit_cast<float>(0x3f07ae14u)))};
        return radii_;
    }
    std::uint32_t Mode() override { return state_.mode; }
    void SelectMoveAnimation(std::string_view name) override { state_.animation=name; }
    void Preprocess(Vec& displacement) override {
        displacement_=Point(displacement);
        const float length=std::hypot(displacement_[0],displacement_[2]);
        if (length>0) {
            const auto probe=[&](Point3 start,float radius) {
                Point3 end=start;
                for (int axis=0;axis<3;++axis)
                    end[axis]+=displacement_[axis]+displacement_[axis]/length*radius;
                return SegmentBlocked(mesh_,start,end);
            };
            blocked_=probe(lower_,radii_.lower)||probe(upper_,radii_.upper);
        }
        Point3 candidate=state_.position;
        for (int axis=0;axis<3;++axis) candidate[axis]+=displacement_[axis];
        const bool no_floor=!FindFloor(mesh_,candidate,config_.step_up,config_.step_down);
        state_.last_obstacle=blocked_?MovementObstacle::wall:(no_floor?MovementObstacle::no_floor:MovementObstacle::none);
        if (blocked_ || no_floor) {
            // The source flow allows collision preprocessing to shorten delta.
            // Keep turn/animation processing alive at an obstacle while refusing
            // translation through it. Full original wall sliding is still open.
            displacement={}; ++state_.clipped_steps;
        }
    }
    void UpdateGround(Vec& position,pal4::ActorMovementRadii) override {
        candidate_floor_=FindFloor(mesh_,Point(position),config_.step_up,config_.step_down);
        if (candidate_floor_) position.y=candidate_floor_->position[1];
    }
    bool CanMove(Vec&,pal4::ActorMovementRadii) override {
        if (!candidate_floor_) {
            ++state_.rejected_steps;
            state_.last_obstacle=MovementObstacle::no_floor;
            return false;
        }
        return true;
    }
    void SetPosition(Vec position) override {
        const auto next=Point(position); double squared{};
        for (int i=0;i<3;++i) squared+=double(next[i]-state_.position[i])*(next[i]-state_.position[i]);
        state_.distance+=std::sqrt(squared); state_.position=next; ++state_.accepted_steps;
    }
    void FaceInput() override {
        double angle=std::acos(std::clamp(double(input_[2]),-1.,1.));
        if (input_[0]<0) angle=6.283185307179586-angle;
        state_.yaw=static_cast<float>(angle*double(std::bit_cast<float>(0x42652ee1u)));
    }
    void UpdateFloor() override { state_.floor=candidate_floor_; }
private:
    MovementState& state_;
    const MovementConfig& config_;
    const CollisionMesh& mesh_;
    Point3 input_,displacement_{},lower_{},upper_{};
    std::optional<FloorHit> candidate_floor_;
    pal4::ActorMovementRadii radii_{};
    bool blocked_{};
};
}
void StepActorMovement(MovementState& state,const MovementConfig& config,
                       const CollisionMesh& mesh,Point3 input,float elapsed) {
    for (float x:{elapsed,config.speed,config.height,config.step_up,config.step_down,state.yaw,
                  state.position[0],state.position[1],state.position[2],input[0],input[1],input[2]})
        if (!std::isfinite(x)) throw std::runtime_error("non-finite movement input/configuration");
    if (elapsed<0 || elapsed>.05f || config.speed<0 || config.height<=0 || config.step_up<0 || config.step_down<0)
        throw std::runtime_error("invalid movement timestep/configuration");
    const float length=std::hypot(input[0],input[2]);
    state.input_active=length>1e-6f; state.last_accepted=false;
    if (!state.input_active || elapsed==0) { state.animation="C01"; return; }
    input={input[0]/length,0,input[2]/length};
    GeometryMovement runtime(state,config,mesh,input);
    state.last_accepted=pal4::DispatchActorVectorMovement(elapsed,runtime);
}
const char* MovementObstacleName(MovementObstacle reason) {
    switch (reason) { case MovementObstacle::wall:return "wall"; case MovementObstacle::no_floor:return "no_floor"; default:return "none"; }
}
}  // namespace pal4::portable
