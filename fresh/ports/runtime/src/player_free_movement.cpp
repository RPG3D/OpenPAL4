#include "pal4/portable/player_free_movement.h"
#include "pal4/actor_vector_movement.h"
#include "pal4/actor_control_discrete_movement.h"
#include "pal4/game_math_tables.h"
#include <bit>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <ostream>
#include <stdexcept>
namespace pal4::portable {
namespace {
void Require(bool value,const char* message){if(!value)throw std::logic_error(message);}
using Vec=pal4::ActorControlVec3;
Point3 Point(Vec value){return {value.x,value.y,value.z};}
Vec Vector(Point3 value){return {value[0],value[1],value[2]};}
bool SameAnimation(std::string_view a,std::string_view b){
    if(a.size()!=b.size())return false;
    for(std::size_t i=0;i<a.size();++i)if(std::tolower(static_cast<unsigned char>(a[i]))!=std::tolower(static_cast<unsigned char>(b[i])))return false;
    return true;
}
}
struct PlayerFreeMovement::Impl final:pal4::IActorVectorMovementRuntime {
    PlayerRoster& players;ActorController& controller;PlayerActions& actions;SceneMovement& floor;SceneCollision& collision;
    IPlayerPlacementServices& services;std::function<void()> camera_access;
    std::uint32_t index{},dispatches{},accepted{},rejected{},disabled{};
    Point3 direction{};std::array<float,4> lower{},upper{};double height{},distance{};
    float above{},below{};std::string phase{"idle"};
    Point3 requested_delta{},resolved_delta{};
    Impl(PlayerRoster& p,ActorController& c,PlayerActions& a,SceneMovement& f,SceneCollision& collision_owner,
        IPlayerPlacementServices& s,std::function<void()> camera):players(p),controller(c),actions(a),floor(f),collision(collision_owner),services(s),camera_access(std::move(camera)) {
        Require(bool(camera_access),"free movement camera access is not bound");
    }
    void Begin() override{phase="camera";camera_access();}
    Vec Position() override{return Vector(players.Slots()[index].position);}
    float Speed() override{return controller.State().input.speed;}
    Vec Forward() override{
        const auto& player=players.Slots()[index];const auto matrix=InstanceMatrix(player.position,player.rotation);return {matrix[8],matrix[9],matrix[10]};
    }
    pal4::ActorMovementRadii PrepareBounds(Vec position) override {
        phase="bounds";const auto& player=players.Slots()[index];Require(bool(player.actor),"free movement actor resource is missing");
        const auto sphere=ActorBindSphere(*player.actor);Require(sphere.has_value(),"free movement requires original actor bounds");height=double((*sphere)[3])*2;
        above=static_cast<float>(height*double(std::bit_cast<float>(0x3eb33333u)));
        below=static_cast<float>(height*double(std::bit_cast<float>(0x3f07ae14u)));
        const auto radius=controller.State().collision_radius;
        lower={position.x,static_cast<float>(height*.5*.75+position.y),position.z,radius};
        upper={position.x,static_cast<float>(height*.5*1.25+position.y),position.z,radius};
        return {above,below};
    }
    std::uint32_t Mode() override{return controller.State().input.mode;}
    void SelectMoveAnimation(std::string_view action) override {
        phase="animation";
        if(SameAnimation(players.Slots()[index].animation,action))return;
        Require(actions.SelectTaskAction(index,action),"original free movement action is unavailable");
        services.SelectAnimation(index,action);
    }
    void Preprocess(Vec& displacement) override {
        phase="collision";auto delta=Point(displacement);
        collision.Resolve(upper,lower,delta,0x10000+index*0xb14);displacement=Vector(delta);
    }
    void UpdateGround(Vec& position,pal4::ActorMovementRadii radii) override {
        phase="floor_select";(void)floor.SelectAndUpdate(Point(position),radii.lower,radii.upper);
    }
    bool CanMove(Vec& position,pal4::ActorMovementRadii radii) override {
        phase="floor_check";auto adjusted=Point(position);
        if(!floor.CheckAndAdjust(adjusted,radii.lower,radii.upper))return false;
        position=Vector(adjusted);return true;
    }
    void SetPosition(Vec position) override {
        phase="position";const auto before=players.Slots()[index].position;const auto after=Point(position);
        players.SetPosition(index,after);double squared{};for(unsigned i=0;i<3;++i){const double delta=double(after[i])-before[i];squared+=delta*delta;}distance+=std::sqrt(squared);
        if(players.Slots()[index].auto_seek)services.FollowPlayer(index);
    }
    void FaceInput() override {
        phase="rotation";static const pal4::GameMathTables math;double angle=math.ApproximateArccosine(direction[2]);
        if(direction[0]<0)angle=6.283185307179586-angle;
        auto rotation=players.Slots()[index].rotation;rotation[1]=static_cast<float>(angle*double(std::bit_cast<float>(0x42652ee1u)));players.SetRotation(index,rotation);
    }
    void UpdateFloor() override {
        phase="attributes";const auto hit=floor.LastHit();Require(hit.has_value(),"accepted free movement lost its floor hit");
        services.UpdateFloorAttributes(floor.ActiveChild(),*hit);
    }
};
namespace {
struct DiscreteMovement final:pal4::ActorControlDiscreteMovementRuntimeStub {
    PlayerRoster& players;ActorController& controller;PlayerActions& actions;SceneMovement& floor;SceneCollision& collision;
    IPlayerPlacementServices& services;std::function<void()>& camera;std::uint32_t index;bool input,accepted{};
    double& height;double& distance;float& above;float& below;Point3& requested_delta;Point3& resolved_delta;
    DiscreteMovement(PlayerRoster& p,ActorController& c,PlayerActions& a,SceneMovement& f,SceneCollision& col,
        IPlayerPlacementServices& s,std::function<void()>& cam,std::uint32_t i,bool active,
        double& h,double& d,float& top,float& bottom,Point3& requested,Point3& resolved)
        :players(p),controller(c),actions(a),floor(f),collision(col),services(s),camera(cam),index(i),input(active),
         height(h),distance(d),above(top),below(bottom),requested_delta(requested),resolved_delta(resolved){}
    std::uint32_t CameraSystem() override{return 0x6000;}
    bool CameraActive(std::uint32_t system) override{Require(system==0x6000,"invalid discrete camera");return true;}
    void GetActiveCamera(std::uint32_t system) override{Require(system==0x6000,"invalid discrete camera access");camera();}
    std::uint32_t ReadWord(std::uint32_t address) override{
        if(address==0x1014)return 0x2000;
        if(address==0x2004)return 0x3000;
        if(address==0x5024)return std::bit_cast<std::uint32_t>(controller.State().collision_radius);
        const auto& player=players.Slots()[index];
        if(address>=0x10ac && address<=0x10b4 && address%4==0)return std::bit_cast<std::uint32_t>(player.position[(address-0x10ac)/4]);
        if(address>=0x4020 && address<=0x4028 && address%4==0){const auto m=InstanceMatrix(player.position,player.rotation);return std::bit_cast<std::uint32_t>(m[8+(address-0x4020)/4]);}
        throw std::logic_error("invalid discrete movement address");
    }
    std::uint32_t ObjectTransform(std::uint32_t object) override{Require(object==0x3000,"invalid discrete model");return 0x4000;}
    double ActorProperty(std::uint32_t actor) override{Require(actor==0x1000,"invalid discrete actor");const auto sphere=ActorBindSphere(*players.Slots()[index].actor);Require(sphere.has_value(),"discrete actor bounds missing");height=double((*sphere)[3])*2;return height;}
    std::uint32_t Controller() override{return 0x5000;}
    void Normalize(Vec& v) override{const auto length=std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z);if(length>0){v.x/=length;v.y/=length;v.z/=length;}}
    void SetAnimationSpeed(std::uint32_t,float speed) override{actions.SetTaskSpeed(index,speed);}
    std::string AnimationName(std::uint32_t) override{return players.Slots()[index].animation;}
    void SetAnimationName(std::uint32_t,std::string_view name) override{Require(actions.SelectTaskAction(index,name),"discrete action unavailable");services.SelectAnimation(index,name);}
    void SetAnimationFlag(std::uint32_t,std::uint32_t flag) override{actions.SetTaskRestart(index,flag!=0);}
    bool AnyInputActive(const pal4::ActorControlInputComponentState&) override{return input;}
    std::uint32_t MovementAdapter() override{return 0x7000;}
    void PreprocessMovement(std::uint32_t,pal4::ActorControlMovementGlobals& globals,Vec& v,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) override{
        auto delta=Point(v);std::array<float,4> lower{globals.lower.x,globals.lower.y,globals.lower.z,std::bit_cast<float>(globals.lower_tag)};
        std::array<float,4> upper{globals.upper.x,globals.upper.y,globals.upper.z,std::bit_cast<float>(globals.upper_tag)};
        requested_delta=delta;collision.Resolve(upper,lower,delta,0x10000+index*0xb14);resolved_delta=delta;v=Vector(delta);
    }
    void UpdateMovement(std::uint32_t,Vec& p,float top,float bottom,std::uint32_t) override{above=top;below=bottom;(void)floor.SelectAndUpdate(Point(p),top,bottom);}
    bool CanMove(std::uint32_t,Vec& p,float top,float bottom) override{auto point=Point(p);if(!floor.CheckAndAdjust(point,top,bottom))return false;p=Vector(point);return true;}
    void SetActorPosition(std::uint32_t,const Vec& p) override{const auto before=players.Slots()[index].position;const auto after=Point(p);double squared{};for(unsigned i=0;i<3;++i){const double delta=double(after[i])-before[i];squared+=delta*delta;}distance+=std::sqrt(squared);players.SetPosition(index,after);accepted=true;if(players.Slots()[index].auto_seek)services.FollowPlayer(index);}
    void RotateActor(std::uint32_t,const Vec& v,std::uint32_t relative) override{auto rotation=players.Slots()[index].rotation;rotation[1]=relative?rotation[1]+v.y:v.y;players.SetRotation(index,rotation);}
    void UpdateMinimap(const Vec& p,std::uint32_t) override{services.UpdateGrid(Point(p),players.Slots()[index].rotation);}
    void UpdateFloorAttributes() override{const auto hit=floor.LastHit();Require(hit.has_value(),"discrete movement floor hit missing");services.UpdateFloorAttributes(floor.ActiveChild(),*hit);}
};
}
PlayerFreeMovement::PlayerFreeMovement(PlayerRoster& p,ActorController& c,PlayerActions& a,SceneMovement& f,SceneCollision& collision,IPlayerPlacementServices& s,std::function<void()> camera):impl_(std::make_unique<Impl>(p,c,a,f,collision,s,std::move(camera))){}
PlayerFreeMovement::~PlayerFreeMovement()=default;
bool PlayerFreeMovement::Advance(Point3 direction,float elapsed){
    return AdvanceVector(direction,elapsed,false);
}
bool PlayerFreeMovement::AdvanceModern(Point3 direction,float elapsed){
    const bool moved=AdvanceVector(direction,elapsed,true);
    impl_->controller.SetAnalogMovementResult(moved);
    return moved;
}
bool PlayerFreeMovement::AdvanceVector(Point3 direction,float elapsed,bool modern){
    auto& s=*impl_;if(!std::isfinite(elapsed) || elapsed<0)throw std::invalid_argument("invalid free movement elapsed");
    for(float v:direction)if(!std::isfinite(v))throw std::invalid_argument("non-finite free movement direction");
    Require(s.players.Prepared() && s.controller.Initialized(),"free movement owners are not ready");
    if(!s.controller.State().enabled || !s.controller.State().actor){++s.disabled;return false;}
    Require(*s.controller.State().actor==s.players.Leader(),"free movement controller and leader differ");
    Require(modern || s.controller.State().input.event_mode==0,"free movement called in event input mode");
    s.index=*s.controller.State().actor;s.direction=direction;++s.dispatches;
    const bool accepted=pal4::DispatchActorVectorMovement(elapsed,s);
    if(accepted){++s.accepted;s.phase="accepted";if(modern)s.services.UpdateGrid(s.players.Slots()[s.index].position,s.players.Slots()[s.index].rotation);}else{++s.rejected;s.phase="rejected";}
    return accepted;
}
bool PlayerFreeMovement::AdvanceDiscrete(std::uint32_t command,float elapsed,bool any_input){
    auto& s=*impl_;if(!std::isfinite(elapsed) || elapsed<0 || command<1 || command>4)throw std::invalid_argument("invalid discrete movement command");
    Require(s.players.Prepared() && s.controller.Initialized(),"discrete movement owners are not ready");
    if(!s.controller.State().enabled || !s.controller.State().actor){++s.disabled;return false;}
    Require(*s.controller.State().actor==s.players.Leader() && s.controller.State().input.event_mode!=0,"invalid discrete movement controller");
    s.index=*s.controller.State().actor;pal4::ActorControlInputComponentState image;
    image.words[1]=std::bit_cast<std::uint32_t>(s.controller.State().input.speed);
    image.words[2]=std::bit_cast<std::uint32_t>(s.controller.State().input.secondary_speed);
    image.words[10]=s.controller.State().input.mode;
    s.requested_delta={};s.resolved_delta={};
    pal4::ActorControlMovementGlobals globals;DiscreteMovement runtime(s.players,s.controller,s.actions,s.floor,s.collision,s.services,s.camera_access,s.index,any_input,s.height,s.distance,s.above,s.below,s.requested_delta,s.resolved_delta);
    ++s.dispatches;const auto result=pal4::ActorControlInputComponent::DispatchDiscreteMovement(image,globals,0x1000,command,elapsed,runtime);
    if(command<=2){if(runtime.accepted)++s.accepted;else ++s.rejected;}
    s.phase=runtime.accepted?"accepted":command>2?"turn":"rejected";return result!=0;
}
void PlayerFreeMovement::WriteReport(std::ostream& output) const {
    const auto& s=*impl_;output<<std::setprecision(9)<<"{\"dispatches\":"<<s.dispatches<<",\"accepted\":"<<s.accepted<<",\"rejected\":"<<s.rejected<<",\"disabled\":"<<s.disabled<<",\"phase\":"<<std::quoted(s.phase)<<",\"actor_height\":"<<s.height<<",\"above\":"<<s.above<<",\"below\":"<<s.below<<",\"distance\":"<<s.distance<<",\"requested_delta\":["<<s.requested_delta[0]<<','<<s.requested_delta[1]<<','<<s.requested_delta[2]<<"],\"resolved_delta\":["<<s.resolved_delta[0]<<','<<s.resolved_delta[1]<<','<<s.resolved_delta[2]<<"]}";
    if(!output)throw std::runtime_error("cannot write free movement report");
}
}
