#include "pal4/portable/monster_orientation.h"
#include "pal4/behavior_helper_configuration.h"
#include "pal4/base_object_child_transform.h"
#include "pal4/base_object_orientation.h"
#include "pal4/game_vector_math.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace pal4::portable {
namespace {
void Require(bool value,const char* message){if(!value)throw std::invalid_argument(message);}
const GameMathTables& Math(){
    static const GameMathTables tables=[]{GameMathTables t;t.InitializeConstants();(void)t.CalculateSquareRootTable();return t;}();return tables;
}
}
struct MonsterOrientation::Impl {
    MonsterMovementWorld& world;SceneMonsterInstances& actors;SceneMovement& floor;MonsterOrientationCaches& caches;bool busy{};
    Impl(MonsterMovementWorld& w,SceneMonsterInstances& a,SceneMovement& f,MonsterOrientationCaches& c):world(w),actors(a),floor(f),caches(c){}
    class Runtime final:public IMonsterPathEndpointUpdateRuntime,private IBaseObjectOrientationRuntime {
        static constexpr std::uint32_t manager_token=1;
        Impl& owner_;MonsterHelperHandle helper_;SceneMonsterHandle actor_;BaseObjectState base_;
        void Manager(std::uint32_t token){Require(token==manager_token,"invalid monster orientation manager token");}
        void Helper(std::uint32_t token){Require(token==helper_.token,"invalid monster orientation helper token");}
        void NormalizeAndClamp(BaseObjectVector3& value) override {
            GameVector3 vector{value.x,value.y,value.z};GameVectorMath::NormalizeAndClamp(vector,Math());value={vector[0],vector[1],vector[2]};
        }
        double AngleFromCosine(double value) override {return std::acos(value);}
        void SetPose(BaseObjectState& state,const BaseObjectVector3& value,std::uint32_t mode) override {
            Require(&state==&base_ && !mode,"invalid monster orientation pose target");owner_.actors.SetMotionRotation(actor_,{value.x,value.y,value.z});
        }
        void SetDirection(BaseObjectState& state,const BaseObjectVector3& value,std::uint32_t mode) override {
            Require(&state==&base_ && !mode,"invalid monster orientation position target");owner_.actors.SetMotionPosition(actor_,{value.x,value.y,value.z});
        }
    public:
        bool floor_hit{};
        Runtime(Impl& owner,MonsterHelperHandle helper,SceneMonsterHandle actor,float manager_radius):owner_(owner),helper_(helper),actor_(actor){
            // The instance owner has a real loaded body; 4440C0 only tests this
            // field for presence. No host pointer is encoded in the base image.
            Require(!owner_.actors.Model(actor_).frames.empty(),"monster orientation body is absent");base_.Write(0x14,1);
            base_.Write(0xdc,std::bit_cast<std::uint32_t>(manager_radius));const auto transform=owner_.actors.MotionTransform(actor_);
            for(unsigned axis=0;axis<3;++axis)base_.Write(0xa0+axis*4,std::bit_cast<std::uint32_t>(transform.rotation[axis]));
        }
        double ManagerDoubleProperty(std::uint32_t token) override {Manager(token);return BaseObjectChildTransform::GetDoublePropertyAtThis(base_,0);}
        BehaviorVector3 ReadHelperVector(std::uint32_t token,std::uint32_t slot) override {Helper(token);return BehaviorHelperConfiguration::ReadVector(owner_.world.Helper(helper_),slot);}
        void InitializeMovementAdapter() override {
            // The scene owner already provides its loaded movement composite.
            // No singleton allocation is necessary at the portable boundary.
            (void)owner_.floor.ActiveChild();
        }
        void CheckCollision(BehaviorVector3& candidate,float above,float below) override {floor_hit=owner_.floor.CheckPrimaryAndAdjust(candidate,above,below);}
        void ApplyHelperPosition(std::uint32_t token,const BehaviorVector3& value) override {
            Helper(token);BehaviorHelperConfiguration::SetVector(owner_.world.Helper(helper_),0x1c,value);
            // Original setter does not re-notify the spatial node here.
        }
        void UpdateOrientation(std::uint32_t token,const BehaviorVector3& direction,const BehaviorVector3&,const BehaviorVector3&,const BehaviorVector3& position) override {
            Manager(token);BaseObjectOrientation::Update(base_,{direction[0],direction[1],direction[2]},0,0,{position[0],position[1],position[2]},*this);
        }
        void Run(std::int32_t kind,MonsterPathEndpointUpdateGlobals& cache){
            MonsterPathEndpointTargetState target{};target.words[(kind==3?0x04:kind==0?0x1c:0x14)/4]=manager_token;
            if(kind==0)MonsterPathEndpointUpdate::Process(target,owner_.world.Helper(helper_),helper_.token,cache,*this);
            else if(kind==1)MonsterPathEndpointC03Update::Process(target,owner_.world.Helper(helper_),helper_.token,cache,*this);
            else MonsterRandomPointUpdate::Process(target,owner_.world.Helper(helper_),helper_.token,cache,*this);
        }
    };
    MonsterOrientationResult Update(std::int32_t kind,MonsterHelperHandle h,float manager_radius){
        if(busy){throw std::logic_error("monster orientation is not reentrant");}
        busy=true;struct Guard{bool& busy;~Guard(){busy=false;}} guard{busy};
        Require(kind==0 || kind==1 || kind==3,"unsupported monster orientation kind");
        Require(std::isfinite(manager_radius) && manager_radius>=0 && std::isfinite(static_cast<float>(double(manager_radius)*2*2.5299999713897705)),"invalid monster manager radius");
        const auto actor=world.Owner(h);const auto& helper=world.Helper(h);Require(world.IsRandom(h)==(kind==3) && helper.words[0]==(kind==3?0x842ae8u:0x842898u),"invalid monster orientation helper type");
        const bool active=((helper.words[46]>>8)&0xff)!=0;
        Runtime runtime(*this,h,actor,manager_radius);runtime.Run(kind,kind==3?caches.random:caches.kinds[static_cast<std::size_t>(kind)]);
        return {active,runtime.floor_hit,actors.MotionTransform(actor)};
    }
};
MonsterOrientation::MonsterOrientation(MonsterMovementWorld& w,SceneMonsterInstances& a,SceneMovement& f,MonsterOrientationCaches& c):impl_(std::make_unique<Impl>(w,a,f,c)){}
MonsterOrientation::~MonsterOrientation()=default;
MonsterOrientationResult MonsterOrientation::Update(std::int32_t kind,MonsterHelperHandle helper,float manager_radius){return impl_->Update(kind,helper,manager_radius);}
}
