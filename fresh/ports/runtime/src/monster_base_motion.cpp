#include "pal4/portable/monster_base_motion.h"
#include "pal4/portable/world.h"
#include "pal4/base_object_motion.h"
#include "pal4/game_vector_math.h"
#include "pal4/resource_rotation_angle.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace pal4::portable {
namespace {
void Require(bool b,const char* s){if(!b)throw std::logic_error(s);}
float Float(std::uint32_t v){return std::bit_cast<float>(v);}
const GameMathTables& Math(){static const auto t=[]{GameMathTables v;(void)v.CalculateSquareRootTable();return v;}();return t;}
}
struct MonsterBaseMotion::Impl final:IBaseObjectMotionRuntime {
    SceneMonsterInstances& actors;SceneMonsterHandle actor;MonsterBehaviorManagerState& state;std::uint32_t token;
    ResourceUpdate resource;CollisionSurface& floor;SurfaceMotion motion;BaseObjectState base{};MonsterBaseMotionResult result{};bool busy{},failed{},internal_transform{},closed{},animation_restored{};std::uint64_t transform_binding{},animation_binding{};
    static std::uint32_t CollisionToken(std::uint32_t id){Require(id && id%4==0 && id<=std::numeric_limits<std::uint32_t>::max()-sizeof(MonsterBehaviorManagerState),"invalid monster base token");return id+0x564;}
    static Point3 Position(const MonsterBehaviorManagerState& s){return {Float(s.words[43]),Float(s.words[44]),Float(s.words[45])};}
    static std::optional<SurfaceMotionImage> CollisionImage(const MonsterBehaviorManagerState& s,ObjectMemory* storage){
        if(!storage){Require(s.words[0x564/4+21]==0,"constructed monster movement requires original object storage");return {};}
        SurfaceMotionImage image;std::copy_n(s.words.begin()+0x564/4,image.size(),image.begin());return image;
    }
    Impl(SceneMonsterInstances& a,SceneMonsterHandle h,MonsterBehaviorManagerState& s,std::uint32_t id,CollisionSurface& surface,float height,MovementCollisionQuery collision,ResourceUpdate update,ObjectMemory* storage)
        :actors(a),actor(h),state(s),token(id),resource(std::move(update)),floor(surface),motion(surface,Position(s),height,std::move(collision),CollisionToken(id),storage,CollisionImage(s,storage)){
        Require(id && id%4==0 && id<=std::numeric_limits<std::uint32_t>::max()-sizeof(s) && bool(resource),"invalid monster base motion binding");
        Require(!actors.Model(actor).frames.empty() && state.words[5],"monster motion requires actual loaded body");
        Require(!actors.TransformBound(actor),"monster transform resource owner is already bound");
        Reload();ValidateTransform();
        const SceneMonsterTransform initial{{Float(base.words[43]),Float(base.words[44]),Float(base.words[45])},{Float(base.words[40]),Float(base.words[41]),Float(base.words[42])}};
        const auto previous=actors.MotionTransform(actor);
        transform_binding=actors.BindTransform(actor,initial,[this](auto& value,auto change){CommitTransform(value,change);});
        try {
            animation_binding=actors.Playback(actor).BindAnimationImage(std::span<std::uint32_t,31>(state.words.data()+0x170/4,31),token+0x170,state.words[5]);
            Reload();PublishMotion();Publish();
        }catch(...){actors.UnbindTransform(actor,transform_binding);actors.SetMotionPosition(actor,previous.position);actors.SetMotionRotation(actor,previous.rotation);throw;}
    }
    ~Impl(){(void)Close();}
    bool Close(){
        if(closed)return animation_restored;
        Require(!busy,"monster motion close during frame");
        if(actors.Playback(actor).OwnsAnimationImage(animation_binding))animation_restored=actors.Playback(actor).UnbindAnimationImage(animation_binding);
        const bool owns=actors.OwnsTransformBinding(actor,transform_binding);if(owns)actors.UnbindTransform(actor,transform_binding);
        motion.Close();if(owns){Reload();PublishMotion();Publish();}
        closed=true;return animation_restored;
    }
    void Reload(){std::copy_n(state.words.begin(),base.words.size(),base.words.begin());}
    void Publish(){std::copy(base.words.begin(),base.words.end(),state.words.begin());}
    void PublishMotion(){const auto image=motion.Image();std::copy(image.begin(),image.end(),base.words.begin()+0x564/4);}
    void ValidateTransform(){
        Require(base.Read(0x14)!=0 && base.Read(0x30)==0,"monster motion secondary clump is not bound");
        for(unsigned a=0;a<3;++a)Require(Float(base.words[46+a])==1.f,"monster motion non-unit scale is not bound");
    }
    void Match(BaseObjectState& s){Require(&s==&base,"monster base motion image mismatch");}
    void Owned(){Require(actors.OwnsTransformBinding(actor,transform_binding),"monster transform binding is no longer owned");Require(actors.Playback(actor).OwnsAnimationImage(animation_binding),"monster animation binding is no longer owned");}
    void Check(){Require(!busy && !failed && !closed,"monster base motion is not ready");Owned();}
    void CheckPublishedTransform(){
        Require(state.words[0x170/4+1]==base.words[5],"monster animation body identity differs from resource body");
        const auto value=actors.MotionTransform(actor);
        for(unsigned a=0;a<3;++a)Require(Float(base.words[43+a])==value.position[a] && Float(base.words[40+a])==value.rotation[a],"resource service overwrote the bound monster transform");
    }
    void UpdateAndRenderClump(float elapsed) override {
        Publish();try{resource(state,elapsed);}catch(...){Reload();throw;}Reload();ValidateTransform();CheckPublishedTransform();
        const auto image=motion.Image();Require(std::equal(image.begin(),image.end(),base.words.begin()+0x564/4),"resource service overwrote owned CollisionActor");++result.resource_updates;
    }
    void ReloadAnimation(){std::copy_n(state.words.begin()+0x170/4,31,base.words.begin()+0x170/4);}
    void UpdateInterpolator(BaseObjectState& s,std::uint32_t offset,float elapsed) override {
        Match(s);Require(offset==0x170,"invalid monster animation slot");
        try{actors.AdvanceAnimation(actor,elapsed);}catch(...){ReloadAnimation();throw;}
        ReloadAnimation();++result.animation_updates;
    }
    void UpdateCharacter(BaseObjectState& s,std::uint32_t offset,std::uint32_t argument,float elapsed,float scale) override {
        Match(s);Require(offset==0x564 && scale==1,"invalid monster collision movement slot");
        try{(void)motion.Advance(elapsed,argument);}catch(...){PublishMotion();throw;}PublishMotion();++result.collision_updates;
    }
    void NormalizeAndClamp(BaseObjectVector3& p) override {GameVector3 v{p.x,p.y,p.z};GameVectorMath::NormalizeAndClamp(v,Math());p={v[0],v[1],v[2]};}
    double AngleFromCosine(float cosine) override {return Math().ApproximateArccosine(cosine);}
    Matrix Transform() const{return InstanceMatrix({Float(base.words[43]),Float(base.words[44]),Float(base.words[45])},{Float(base.words[40]),Float(base.words[41]),Float(base.words[42])});}
    static Point3 Apply(const Matrix& m,Point3 p){Point3 out{};for(unsigned a=0;a<3;++a){out[a]=m[a]*p[0]+m[4+a]*p[1]+m[8+a]*p[2]+m[12+a];}return out;}
    void Corners(){
        if(!base.words[8])return;
        const auto matrix=Transform();Point3 maximum{-std::numeric_limits<float>::max(),-std::numeric_limits<float>::max(),-std::numeric_limits<float>::max()},minimum{std::numeric_limits<float>::max(),std::numeric_limits<float>::max(),std::numeric_limits<float>::max()};
        for(unsigned i=0;i<8;++i){const auto p=Apply(matrix,{Float(base.words[(i&1)?60:63]),Float(base.words[(i&2)?61:64]),Float(base.words[(i&4)?62:65])});
            for(unsigned a=0;a<3;++a){maximum[a]=std::max(maximum[a],p[a]);minimum[a]=std::min(minimum[a],p[a]);}}
        for(unsigned a=0;a<3;++a){base.words[66+a]=std::bit_cast<std::uint32_t>(maximum[a]);base.words[69+a]=std::bit_cast<std::uint32_t>(minimum[a]);}
    }
    void CommitTransform(SceneMonsterTransform& value,MonsterTransformChange change){
        Owned();Require(!failed && (!busy || internal_transform),"monster transform mutation during base frame");
        const bool external=!internal_transform;if(external){busy=true;Reload();}
        struct Guard{bool& busy;bool external;~Guard(){if(external)busy=false;}} guard{busy,external};
        try{
            ValidateTransform();CheckPublishedTransform();
            if(change==MonsterTransformChange::Rotation){
                for(unsigned a=0;a<3;++a){value.rotation[a]=NormalizeResourceRotationAngle(value.rotation[a]);base.words[40+a]=std::bit_cast<std::uint32_t>(value.rotation[a]);}
            }else{
                for(unsigned a=0;a<3;++a){base.words[43+a]=std::bit_cast<std::uint32_t>(value.position[a]);}
            }
            Corners();
            if(change==MonsterTransformChange::Position){const auto center=Apply(Transform(),{Float(base.words[52]),Float(base.words[53]),Float(base.words[54])});for(unsigned a=0;a<3;++a){base.words[56+a]=std::bit_cast<std::uint32_t>(center[a]);}}
            Publish();
        }catch(...){failed=true;Publish();throw;}
    }
    template<class F>void InternalTransform(F operation){
        Require(!internal_transform,"monster transform callback is not reentrant");internal_transform=true;struct Guard{bool& flag;~Guard(){flag=false;}} guard{internal_transform};operation();
    }
    void SetPose(BaseObjectState& s,const BaseObjectVector3& p,std::uint32_t mode) override {
        Match(s);Require(mode==0,"invalid monster pose mode");InternalTransform([&]{actors.SetMotionRotation(actor,{p.x,p.y,p.z});});
    }
    std::uint32_t SetDirection(BaseObjectState& s,const BaseObjectVector3& p) override {
        Match(s);InternalTransform([&]{actors.SetMotionPosition(actor,{p.x,p.y,p.z});});
        return token+56*4; // RwV3dTransformPoint returns the destination at +E0.
    }
};
MonsterBaseMotion::MonsterBaseMotion(SceneMonsterInstances& a,SceneMonsterHandle h,MonsterBehaviorManagerState& s,std::uint32_t token,CollisionSurface& surface,float height,MovementCollisionQuery collision,ResourceUpdate resource,ObjectMemory* storage)
    :impl_(std::make_unique<Impl>(a,h,s,token,surface,height,std::move(collision),std::move(resource),storage)){}
MonsterBaseMotion::~MonsterBaseMotion()=default;
bool MonsterBaseMotion::Busy() const noexcept{return impl_->busy;}
bool MonsterBaseMotion::Start(Point3 from,Point3 to,std::uint32_t face){auto& s=*impl_;s.Check();s.Reload();s.busy=true;struct Guard{bool& busy;~Guard(){busy=false;}} guard{s.busy};try{const auto result=s.motion.Start(from,to,face);s.PublishMotion();s.Publish();return result;}catch(...){s.PublishMotion();s.Publish();s.failed=true;throw;}}
void MonsterBaseMotion::SetSpeed(float speed){auto& s=*impl_;s.Check();s.Reload();s.motion.SetSpeed(speed);s.PublishMotion();s.Publish();}
void MonsterBaseMotion::Stop(){auto& s=*impl_;Require(!s.busy && !s.closed,"monster motion stop during frame or after close");s.Owned();s.Reload();s.motion.Stop();s.PublishMotion();s.Publish();}
bool MonsterBaseMotion::Close(){return impl_->Close();}
MonsterBaseMotionResult MonsterBaseMotion::Update(float elapsed,std::uint32_t argument){
    auto& s=*impl_;s.Check();Require(std::isfinite(elapsed) && elapsed>=0,"invalid monster base frame time");s.busy=true;
    struct Guard{bool& busy;~Guard(){busy=false;}} guard{s.busy};s.result={};s.Reload();
    try{s.result.position_result=BaseObjectMotion::UpdatePosition(s.base,s.token,elapsed,argument,s);s.Publish();return s.result;}
    catch(...){s.Publish();s.failed=true;throw;}
}
void MonsterBaseMotion::ValidateBinding(SceneMonsterInstances& a,SceneMonsterHandle h,MonsterBehaviorManagerState& state,std::uint32_t token,CollisionSurface& floor) const {
    auto& s=*impl_;Require(&a==&s.actors && h==s.actor && &state==&s.state && token==s.token && &floor==&s.floor,"monster pursuit motion binding mismatch");s.Check();
}
bool MonsterBaseMotion::MovementActive() const{return impl_->motion.Active();}
bool MonsterBaseMotion::Failed() const{return impl_->failed;}
void MonsterBaseMotion::ValidateComponentOwner(SceneMonsterInstances& actors,SceneMonsterHandle actor,MonsterBehaviorManagerState& state,std::uint32_t token) const{
    auto& s=*impl_;Require(&actors==&s.actors && actor==s.actor && &state==&s.state && token==s.token,"monster component motion belongs to another object");
    s.Check();s.Reload();s.ValidateTransform();s.CheckPublishedTransform();
}
}
