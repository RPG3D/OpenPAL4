#include "pal4/portable/monster_movement.h"
#include "pal4/behavior_helper_configuration.h"
#include "pal4/behavior_helper_physics.h"
#include "pal4/behavior_path_endpoint_movement.h"
#include "pal4/behavior_path_collision_state.h"
#include "pal4/object_collision_processor.h"
#include "pal4/object_collision_time.h"
#include "pal4/object_complex_collision.h"
#include "pal4/object_movement_vectors.h"
#include "pal4/object_movement_scalars.h"
#include "pal4/object_random_movement_collision.h"
#include "pal4/object_random_movement_update.h"
#include "pal4/global_render_state.h"
#include "pal4/vector3_operations.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <type_traits>

namespace pal4::portable {
namespace {
void Require(bool value,const char* message){if(!value)throw std::invalid_argument(message);}
template<class T,class U>void Copy(T& to,const U& from,std::size_t bytes){
    static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_copyable_v<U>);
    const auto source=std::as_bytes(std::span{&from,1u});auto target=std::as_writable_bytes(std::span{&to,1u});
    Require(bytes<=source.size() && bytes<=target.size(),"path movement prefix extent");std::copy_n(source.begin(),bytes,target.begin());
}
struct FrameGuard {
    bool& busy;
    explicit FrameGuard(bool& b):busy(b){if(busy)throw std::logic_error("monster path movement is not reentrant");busy=true;}
    ~FrameGuard(){busy=false;}
};
ObjectMovementState Prefix(const BehaviorPathEndpointState& helper){
    Require(helper.words[0]==0x842898u || helper.words[0]==0x842ae8u,"unsupported movement helper table");
    ObjectMovementState image{};Copy(image,helper,sizeof(image));return image;
}
GridManagerState Physics(const BehaviorPathEndpointState& helper){GridManagerState image{};Copy(image,helper,sizeof(image));return image;}

// Separate adapter because movement slot10 means forward, whereas collision
// slot18 means current position. Their legacy C++ callback names are identical.
class PathPrediction final:public IObjectCollisionRuntime,private IObjectMovementScalarRuntime {
    MonsterMovementWorld& world_;BehaviorPathEndpointState& helper_;std::uint32_t path_;
    void Target(std::uint32_t proxy){Require(proxy==path_+0x54u,"invalid path prediction secondary interface");}
    BehaviorVector3 ReadSlot10Vector(ObjectMovementState& state) override {
        BehaviorVector3 output{};(void)ObjectMovementVectors::CopyAt28(state,output);return output;
    }
public:
    PathPrediction(MonsterMovementWorld& world,BehaviorPathEndpointState& helper):world_(world),helper_(helper),path_(helper.words[47]){
        (void)world_.PathHandle(path_);
        Require(world_.ReadWord(path_)==0x84b5e4u && world_.ReadWord(path_+4)==0x84b5f8u &&
            world_.ReadWord(path_+80)==0 && world_.ReadWord(path_+84)==0x84b5d4u &&
            world_.ReadWord(path_+8)>=2,"invalid path prediction grid");
    }
    float MovementScale(ObjectMovementState& state) override {return ObjectMovementScalars::Get25(state);}
    BehaviorVector3 Motion(ObjectMovementState&,float elapsed) override {return BehaviorHelperPhysics::PredictPosition(Physics(helper_),elapsed);}
    BehaviorVector3 Position(ObjectMovementState&) override {return BehaviorHelperConfiguration::ReadVector(helper_,0x18u);}
    float Measure(std::uint32_t target,const BehaviorVector3& point) override {Target(target);return world_.PathClosest(path_,point);}
    ObjectCollisionSweep Sweep(std::uint32_t target,const BehaviorVector3& point) override {
        Target(target);const auto hit=world_.PathProject(path_,point,{});return {hit.point,hit.outside};
    }
    BehaviorVector3 Predict(std::uint32_t target,const BehaviorVector3&,float distance) override {Target(target);return world_.PathCoordinates(path_,distance);}
    void Adjust(ObjectMovementState&,const BehaviorVector3&,BehaviorVector3&,BehaviorVector3&,float) override {
        // Endpoint table slot84 is the verified 42B060 shared callback.
        ObjectMovementVectors::IgnoreFourArguments(0,0,0,0);
    }
    BehaviorVector3 Offset(ObjectMovementState& state) override {
        BehaviorVector3 output{};(void)ObjectMovementScalars::ScaledSlot10Vector(state,output,*this);return output;
    }
    BehaviorVector3 GlobalZeroVector() override {return {};}
    BehaviorVector3 Correct(bool clamped){
        auto image=Prefix(helper_);BehaviorVector3 output{};
        if(clamped)(void)ObjectCollisionProcessor::ProcessClamped(image,output,std::bit_cast<std::int32_t>(image.words[48]),2.5f,path_+0x54u,*this);
        else (void)ObjectCollisionProcessor::ProcessUnclamped(image,output,2.5f,path_+0x54u,*this);
        return output;
    }
};
}
struct MonsterMovement::Impl {
    MonsterMovementWorld& world;MonsterMovementStats stats{};bool busy{};
    std::vector<std::uint32_t> neighbors,obstacle_tokens;
    explicit Impl(MonsterMovementWorld& w):world(w){}
    class Runtime final:public IBehaviorPathMovementRuntime,public IObjectRandomMovementUpdateRuntime,private IObjectMovementRuntime,
        private IBehaviorPathCollisionStateRuntime,private IVectorRandomSource,
        private IObjectRandomMovementCollisionRuntime,private IObjectCollisionTimeRuntime,
        private IObjectComplexCollisionRuntime,private IObjectMovementVectorRuntime,
        private IObjectMovementScalarRuntime,private IGridRandomRuntime {
        static constexpr std::uint32_t context_token=1,requested_token=2,position_token=3,obstacle_base=0x80000000u;
        Impl& owner_;MonsterHelperHandle handle_;BehaviorPathEndpointState& helper_;
        std::span<const MonsterMovementObstacle> obstacles_;
        BehaviorVector3 requested_{},position_{};ObjectRandomMovementState* random_image_{};
        BehaviorPathEndpointState& Image(std::uint32_t token){return owner_.world.Helper({handle_.generation,token});}
        BehaviorVector3 Vector(std::uint32_t token,std::uint32_t slot){return BehaviorHelperConfiguration::ReadVector(Image(token),slot);}
        const MonsterMovementObstacle& Obstacle(std::uint32_t token){
            Require(token>=obstacle_base && (token-obstacle_base)%20==0,"invalid path obstacle token");
            const auto index=(token-obstacle_base)/20;Require(index<obstacles_.size(),"path obstacle token out of range");return obstacles_[index];
        }
        void Check(BehaviorPathEndpointState& state){Require(&state==&helper_,"path frame helper mismatch");}
        void Check(ObjectRandomMovementState& state){Require(&state==random_image_,"random frame helper mismatch");}
        static BehaviorVector3 RandomVectorAt(const ObjectRandomMovementState& state,std::uint32_t slot){
            return BehaviorHelperConfiguration::ReadVector(std::bit_cast<BehaviorPathEndpointState>(state),slot);
        }
        void SyncDirection(ObjectMovementState& state){state.words[48]=helper_.words[48];}
        BehaviorVector3 Position(ObjectMovementState& state) override {
            BehaviorVector3 output{};(void)ObjectMovementVectors::CopyAt28(state,output);return output;
        }
        std::int32_t Rand() override {const auto value=owner_.world.Rand();++owner_.stats.random_draws;return static_cast<std::int32_t>(value);}
        float CandidateRange(ObjectMovementState& state) override {return ObjectMovementScalars::Get25(state);}
        void CalculateCandidate(ObjectMovementState& state,std::uint32_t token,ObjectCollisionCandidate& output) override {
            ++owner_.stats.candidate_tests;ObjectCollisionTime::Calculate(state,token,output,*this);owner_.stats.candidate_hits+=output.HasHit();
        }
        void NotifyCandidate(ObjectMovementState& state,std::uint32_t token,float range) override {
            const auto& obstacle=Obstacle(token);
            const std::array<float,5> record{std::bit_cast<float>(obstacle.word0),obstacle.radius,obstacle.center[0],obstacle.center[1],obstacle.center[2]};
            BehaviorPathCollisionState::UpdateFromRecord(helper_,&record,std::bit_cast<std::uint32_t>(range),*this);SyncDirection(state);
        }
        BehaviorVector3 CandidateResponse(ObjectMovementState&,const ObjectCollisionCandidate& candidate,float) override {
            ObjectRandomMovementState image{};Copy(image,helper_,sizeof(image));return ObjectRandomMovementCollision::ObstacleCorrection(image,candidate,*this);
        }
        BehaviorVector3 ComparisonVector() override {return {};}
        float MovementRange(ObjectMovementState& state) override {return ObjectMovementScalars::Get27(state);}
        void CopyGlobalCollection() override {owner_.neighbors.clear();}
        void NotifyTarget(ObjectMovementState& state,float radius,ObjectMovementTarget& target) override {
            Require(state.words[42]==helper_.words[42] && state.words[42]!=0,"path frame association mismatch");
            const auto center=Scan(radius);target.collision_input=center;target.unclamped_input=center;
        }
        BehaviorVector3 Scan(float radius){
            const auto center=Vector(handle_.token,0x18u);++owner_.stats.sphere_queries;
            const auto found=owner_.world.QuerySphere(center,radius);
            for(const auto& entry:found){owner_.neighbors.push_back(entry.helper.token);++owner_.stats.neighbor_visits;}
            return center;
        }
        BehaviorVector3 ComplexCollision(ObjectMovementState& state,const BehaviorVector3&) override {
            BehaviorVector3 output{};(void)ObjectComplexCollision::Process(state,handle_.token,output,3.f,owner_.neighbors,*this);SyncDirection(state);return output;
        }
        float RandomClamped(float value,float radius) override {++owner_.stats.drift_updates;return Vector3Operations::RandomRangeClamped(value,radius,-1.f,1.f,*this);}
        BehaviorVector3 DirectionDrift(ObjectMovementState&,float first,float second,const BehaviorVector3&) override {
            BehaviorVector3 up{},right{},output{};(void)Vector3Operations::Scale(Vector(handle_.token,8),up,second);
            (void)Vector3Operations::Scale(Vector(handle_.token,0),right,first);(void)Vector3Operations::Add(right,output,up);return output;
        }
        std::uint32_t ResolveCollisionProxy(std::uint32_t path) override {return path?path+0x54u:0;}
        BehaviorVector3 ClampedCollision(ObjectMovementState&,const BehaviorVector3&,std::uint32_t option,std::uint32_t proxy) override {
            Require(proxy==helper_.words[47]+0x54u && option==helper_.words[48],"path frame clamped target mismatch");
            ++owner_.stats.path_corrections;return PathPrediction(owner_.world,helper_).Correct(true);
        }
        BehaviorVector3 UnclampedCollision(ObjectMovementState&,const BehaviorVector3&,std::uint32_t proxy) override {
            Require(proxy==helper_.words[47]+0x54u,"path frame unclamped target mismatch");
            ++owner_.stats.path_corrections;return PathPrediction(owner_.world,helper_).Correct(false);
        }
        BehaviorVector3 ReadPosition(std::uint32_t token,BehaviorContext3&) override {return Vector(token,0x18u);}
        BehaviorVector3 CollisionPosition(ObjectRandomMovementState&) override {return Vector(handle_.token,0x18u);}
        BehaviorVector3 CandidateVector(std::uint32_t token) override {Require(token>=8,"invalid obstacle vector token");return Obstacle(token-8).center;}
        BehaviorVector3 CollisionAxis(ObjectRandomMovementState&) override {return Vector(handle_.token,0x10u);}
        float CorrectionScale(ObjectRandomMovementState&) override {return ObjectMovementScalars::Get26(Prefix(helper_));}
        BehaviorVector3 RelativeVector(ObjectMovementState& state,std::uint32_t token) override {
            BehaviorVector3 output{};(void)ObjectMovementVectors::DifferenceAndProcess(state,output,CandidateVector(token),*this);return output;
        }
        float ObjectRadius(ObjectMovementState& state) override {return ObjectMovementScalars::Get24(state);}
        float CandidateRadius(std::uint32_t token) override {return Obstacle(token).radius;}
        void ProcessDifference(ObjectMovementState& state,BehaviorVector3& output,const BehaviorVector3& difference) override {
            GlobalRenderState basis;for(unsigned word=0;word<13;++word)basis.WriteDword(word*4,state.words[word]);(void)basis.TransformVector(output,difference);
        }
        BehaviorVector3 TransformVector(ObjectMovementState&,std::uint32_t) override {throw std::logic_error("collision time does not request world-vector addition");}
        float Radius(std::uint32_t token) override {return ObjectMovementScalars::Get24(Prefix(Image(token)));}
        float Speed(std::uint32_t token) override {return ObjectMovementScalars::Get25(Prefix(Image(token)));}
        BehaviorVector3 Position(std::uint32_t token) override {return Vector(token,0x18u);}
        BehaviorVector3 Offset(std::uint32_t token) override {
            auto image=Prefix(Image(token));BehaviorVector3 output{};(void)ObjectMovementScalars::ScaledSlot10Vector(image,output,*this);return output;
        }
        BehaviorVector3 Forward(std::uint32_t token) override {return Vector(token,0x10u);}
        BehaviorVector3 Direction(std::uint32_t token) override {return Vector(token,0);}
        BehaviorVector3 GlobalZeroVector() override {return {};}
        BehaviorVector3 ReadSlot10Vector(ObjectMovementState& state) override {BehaviorVector3 output{};(void)ObjectMovementVectors::CopyAt28(state,output);return output;}
        void NotifyProximity(std::uint32_t self,std::uint32_t other,std::uint32_t argument) override {
            Require(self==handle_.token,"movement proximity owner mismatch");++owner_.stats.proximity;
            if(random_image_)ObjectMovementVectors::IgnoreFourArguments(self,other,argument,0);
            else BehaviorPathCollisionState::UpdateFromObject(helper_,other,argument,*this);
        }
        void NotifyCollision(std::uint32_t self,std::uint32_t other,float sign,const BehaviorVector3&,const BehaviorVector3& projected_other) override {
            Require(self==handle_.token,"path collision owner mismatch");++owner_.stats.predicted;
            if(random_image_)ObjectMovementVectors::IgnoreFourArguments(self,other,std::bit_cast<std::uint32_t>(sign),0);
            else BehaviorPathCollisionState::UpdateFromVector(helper_,other,std::bit_cast<std::uint32_t>(sign),0,&projected_other,*this);
        }
    public:
        Runtime(Impl& owner,MonsterHelperHandle handle,BehaviorPathEndpointState& helper,std::span<const MonsterMovementObstacle> obstacles)
            :owner_(owner),handle_(handle),helper_(helper),obstacles_(obstacles){
            Require(obstacles.size()<=65535,"too many path obstacles");owner_.obstacle_tokens.clear();owner_.obstacle_tokens.reserve(obstacles.size());
            for(std::size_t i=0;i<obstacles.size();++i){const auto& obstacle=obstacles[i];
                Require(std::isfinite(obstacle.radius) && obstacle.radius>=0,"invalid path obstacle radius");
                for(auto value:obstacle.center)Require(std::isfinite(value),"non-finite path obstacle center");
                owner_.obstacle_tokens.push_back(obstacle_base+static_cast<std::uint32_t>(i)*20);
            }
        }
        BehaviorVector3 Requested() const {return requested_;}
        void Run(float elapsed){
            if(helper_.words[0]==0x842898u){(void)BehaviorPathEndpointMovement::Update(helper_,context_token,elapsed,*this);return;}
            auto image=std::bit_cast<ObjectRandomMovementState>(helper_);random_image_=&image;
            try{(void)ObjectRandomMovementUpdate::Update(image,context_token,elapsed,0,*this);}
            catch(...){helper_=std::bit_cast<BehaviorPathEndpointState>(image);random_image_=nullptr;throw;}
            helper_=std::bit_cast<BehaviorPathEndpointState>(image);random_image_=nullptr;
        }
        BehaviorVector3 Position(ObjectRandomMovementState& state) override {Check(state);return RandomVectorAt(state,0x18u);}
        BehaviorVector3 RandomVector() override {GridVector3 output{};return GridVectorStatics::RandomInsideUnitSphere(output,*this);}
        BehaviorVector3 BaseVector(ObjectRandomMovementState& state) override {Check(state);return RandomVectorAt(state,0x10u);}
        float Speed(ObjectRandomMovementState& state) override {Check(state);return std::bit_cast<float>(state.words[25]);}
        float FallbackRange(ObjectRandomMovementState& state) override {Check(state);return std::bit_cast<float>(state.words[27]);}
        void CalculateCandidate(ObjectRandomMovementState& state,std::uint32_t token,ObjectCollisionCandidate& output) override {
            Check(state);auto image=Prefix(std::bit_cast<BehaviorPathEndpointState>(state));CalculateCandidate(image,token,output);
        }
        void NotifyCandidate(ObjectRandomMovementState& state,std::uint32_t token,float range) override {
            Check(state);ObjectMovementVectors::IgnoreFourArguments(handle_.token,token,std::bit_cast<std::uint32_t>(range),0);
        }
        BehaviorVector3 NotifyMemberScan(ObjectRandomMovementState& state,std::uint32_t member,float radius) override {
            Check(state);Require(member==helper_.words[42],"random scan association mismatch");return Scan(radius);
        }
        BehaviorVector3 ComplexCollision(ObjectRandomMovementState& state,const BehaviorVector3&,float maximum) override {
            Check(state);auto image=Prefix(std::bit_cast<BehaviorPathEndpointState>(state));BehaviorVector3 output{};
            (void)ObjectComplexCollision::Process(image,handle_.token,output,maximum,owner_.neighbors,*this);
            std::copy_n(image.words.begin()+15,6,state.words.begin()+15);return output;
        }
        BehaviorVector3 ProcessCollision(ObjectRandomMovementState& state,std::uint32_t input,float elapsed) override {
            Check(state);Require(input==context_token,"invalid random frame context");
            (void)ObjectRandomMovementCollision::Process(state,requested_,elapsed,owner_.obstacle_tokens,*this);return requested_;
        }
        void UpdateBase(ObjectRandomMovementState& state,std::uint32_t input,float elapsed,const BehaviorVector3& requested) override {
            Check(state);Require(input==context_token,"invalid random physics context");
            auto image=Physics(std::bit_cast<BehaviorPathEndpointState>(state));BehaviorHelperPhysics::Integrate(image,requested,elapsed);Copy(state,image,sizeof(image));
        }
        std::uint32_t MemberVtable(std::uint32_t member) override {return owner_.world.ReadWord(member);}
        std::uint32_t PositionToken(ObjectRandomMovementState& state) override {Check(state);position_=RandomVectorAt(state,0x18u);return position_token;}
        std::uint32_t NotifyMember(std::uint32_t table,std::uint32_t target,std::uint32_t token) override {return NotifyTargetSlot04(table,target,token);}
        std::uint32_t ProcessCollision(BehaviorPathEndpointState& state,BehaviorContext3&,std::uint32_t input,float elapsed) override {
            Check(state);Require(input==context_token,"invalid path frame context");auto image=Prefix(state);
            (void)ObjectMovementProcessor::Process(image,requested_,elapsed,owner_.obstacle_tokens,*this);
            std::copy_n(image.words.begin()+13,8,state.words.begin()+13);return requested_token;
        }
        void UpdateBase(BehaviorPathEndpointState& state,std::uint32_t token,float elapsed) override {
            Check(state);Require(token==requested_token,"invalid path physics token");auto image=Physics(state);
            BehaviorHelperPhysics::Integrate(image,requested_,elapsed);Copy(state,image,sizeof(image));
        }
        BehaviorPositionResult ReadCurrentPosition(BehaviorPathEndpointState& state,BehaviorContext3&) override {
            Check(state);position_=Vector(handle_.token,0x18u);return {position_token,position_};
        }
        float ReadPathRadius(std::uint32_t path) override {(void)owner_.world.PathHandle(path);return std::bit_cast<float>(owner_.world.ReadWord(path+16));}
        std::uint32_t Read32(std::uint32_t address) override {return owner_.world.ReadWord(address);}
        std::uint32_t NotifyTargetSlot04(std::uint32_t table,std::uint32_t target,std::uint32_t token) override {
            Require(table==0x84b6c0u && target==helper_.words[42] && token==position_token,"invalid path frame position notification");
            return owner_.world.UpdatePosition(target,position_);
        }
    };
    MonsterMovementResult Update(MonsterHelperHandle handle,std::span<const MonsterMovementObstacle> obstacles,float elapsed){
        FrameGuard guard(busy);Require(std::isfinite(elapsed) && elapsed>=0,"invalid path frame time");
        auto& helper=world.Helper(handle);const bool random=world.IsRandom(handle);
        Require(helper.words[0]==(random?0x842ae8u:0x842898u),"movement helper type differs from owner");
        const auto range=std::bit_cast<float>(helper.words[47]);if(random)Require(std::isfinite(range),"non-finite random movement range");
        const bool active=random?range>0:helper.words[47]!=0;const auto direction=helper.words[48];
        if(active){
            if(!random)(void)world.PathHandle(helper.words[47]);
            Require(world.ReadWord(helper.words[42])==0x84b6c0u &&
                world.ReadWord(helper.words[42]+16)==handle.token &&
                world.ReadWord(helper.words[42]+32)==world.ReadWord(world.GridParameter()+4),
                "path frame association belongs to another helper or grid");
        }
        struct Clear {Impl& owner;~Clear(){owner.neighbors.clear();owner.obstacle_tokens.clear();}} clear{*this};
        Runtime runtime(*this,handle,helper,obstacles);runtime.Run(elapsed);
        if(active){++stats.frames;stats.direction_flips+=!random && helper.words[48]!=direction;}
        return {active,runtime.Requested()};
    }
};
MonsterMovement::MonsterMovement(MonsterMovementWorld& world):impl_(std::make_unique<Impl>(world)){}
MonsterMovement::~MonsterMovement()=default;
MonsterMovementResult MonsterMovement::Update(MonsterHelperHandle helper,std::span<const MonsterMovementObstacle> obstacles,float elapsed){return impl_->Update(helper,obstacles,elapsed);}
const MonsterMovementStats& MonsterMovement::Stats() const{return impl_->stats;}
bool MonsterMovement::UsesWorld(const MonsterMovementWorld& world) const noexcept{return &impl_->world==&world;}
}
