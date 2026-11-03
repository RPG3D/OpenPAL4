#include "pal4/portable/actor_resource_initialization.h"
#include "pal4/portable/world.h"
#include "pal4/game_resource_manager.h"
#include "actor_resource_assets.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace pal4::portable {
namespace {
void Require(bool b,const char* message){if(!b)throw std::logic_error(message);}
using Point=std::array<float,3>;
Point Apply(const Matrix& m,const Point& p){Point out{};for(unsigned a=0;a<3;++a){out[a]=m[a]*p[0]+m[4+a]*p[1]+m[8+a]*p[2]+m[12+a];Require(std::isfinite(out[a]),"non-finite initialized actor transform");}return out;}
struct PoseRuntime final:IGameResourcePoseRuntime {
    const Actor& actor;GameResourceManager& resource;std::uint32_t manager,clump;Point position{},rotation{};
    std::array<ActorLocalBox,2> boxes;std::array<std::array<float,4>,2> spheres;
    PoseRuntime(const Actor& a,GameResourceManager& r,std::uint32_t id,std::uint32_t body):actor(a),resource(r),manager(id),clump(body){
        for(unsigned mode=0;mode<2;++mode){const auto box=ActorBindBox(actor,mode!=0);const auto sphere=ActorBindSphere(actor,mode!=0);Require(box && sphere,"actor initialization requires original bounds metadata");boxes[mode]=*box;spheres[mode]=*sphere;}
    }
    void Frame(std::uint32_t id) const {Require(id==clump+4,"invalid actor initialization frame");}
    Matrix Transform() const {return InstanceMatrix(position,rotation);}
    std::uint32_t ClumpFrame(std::uint32_t id) override {Require(id==clump,"invalid actor initialization clump");return clump+4;}
    void FrameSetIdentity(std::uint32_t id) override {Frame(id);position={};rotation={};}
    void FrameRotate(std::uint32_t id,std::uint32_t axis,float angle,std::uint32_t combine) override {
        Frame(id);Require(std::isfinite(angle) && (combine==0 || combine==2),"invalid actor initialization rotation");if(!combine){position={};rotation={};}
        const unsigned a=axis==0x84b870?0:axis==0x84b87c?1:axis==0x84b888?2:3;Require(a<3,"invalid actor rotation axis");rotation[a]+=angle;
    }
    void FrameUpdateObjects(std::uint32_t id) override {Frame(id);for(float v:Transform()){Require(std::isfinite(v),"invalid actor root matrix");}}
    void FrameScale(std::uint32_t id,const float* values,std::uint32_t combine) override {
        Frame(id);Require(values && (combine==0 || combine==2) && values[0]==1 && values[1]==1 && values[2]==1,"actor initialization non-unit scale is not bound");if(!combine){position={};rotation={};}
    }
    void FrameTranslate(std::uint32_t id,const float* values,std::uint32_t combine) override {Frame(id);Require(values && combine==2,"invalid actor initialization translation");for(unsigned a=0;a<3;++a){Require(std::isfinite(values[a]),"non-finite actor position");position[a]+=values[a];}}
    std::uint32_t ObjectTransform(std::uint32_t id) override {Frame(id);return id;}
    void ApplyDialogTransform(std::uint32_t,std::uint32_t,std::uint32_t) override {throw std::logic_error("actor initialization secondary clump is not bound");}
    void TransformPoints(ResourceCorners& corners,std::uint32_t id) override {Frame(id);const auto matrix=Transform();for(auto& p:corners){const auto value=Apply(matrix,{p.x,p.y,p.z});p={value[0],value[1],value[2]};}}
    void CalculateBBox(std::uint32_t* output,const ResourceCorners& corners) override {
        Require(output==resource.words.data()+66,"invalid actor world box destination");Point high{-std::numeric_limits<float>::max(),-std::numeric_limits<float>::max(),-std::numeric_limits<float>::max()},low{std::numeric_limits<float>::max(),std::numeric_limits<float>::max(),std::numeric_limits<float>::max()};
        for(const auto& p:corners){const Point value{p.x,p.y,p.z};for(unsigned a=0;a<3;++a){high[a]=std::max(high[a],value[a]);low[a]=std::min(low[a],value[a]);}}
        for(unsigned a=0;a<3;++a){output[a]=std::bit_cast<std::uint32_t>(high[a]);output[3+a]=std::bit_cast<std::uint32_t>(low[a]);}
    }
    std::uint32_t CalculateCameraPosition(std::uint32_t* destination,std::uint32_t* source,std::uint32_t id) override {
        Frame(id);Require(destination==resource.words.data()+56 && source==resource.words.data()+52,"invalid actor sphere transform");const auto p=Apply(Transform(),{std::bit_cast<float>(source[0]),std::bit_cast<float>(source[1]),std::bit_cast<float>(source[2])});for(unsigned a=0;a<3;++a){destination[a]=std::bit_cast<std::uint32_t>(p[a]);}return manager+0xe0;
    }
    void ProcessAtomicBounds(std::uint32_t id,std::uint32_t mode,std::uint32_t* output) override {
        Require(id==clump && mode<2 && output==resource.words.data()+(mode?66:60),"invalid initial actor box call");for(unsigned a=0;a<3;++a){output[a]=std::bit_cast<std::uint32_t>(boxes[mode].maximum[a]);output[3+a]=std::bit_cast<std::uint32_t>(boxes[mode].minimum[a]);}
    }
    std::uint32_t ProcessAtomicBounds2(std::uint32_t id,std::uint32_t mode,std::uint32_t initial,std::uint32_t* output) override {
        Require(id==clump && mode<2 && initial==1 && output==resource.words.data()+(mode?56:52),"invalid initial actor sphere call");for(unsigned a=0;a<4;++a){output[a]=std::bit_cast<std::uint32_t>(spheres[mode][a]);}return clump;
    }
};
}
namespace {
void ValidateBinding(SceneMonsterInstances& actors,SceneMonsterHandle h,const MonsterBehaviorManagerState& state,std::uint32_t manager,std::uint32_t clump){
    Require(manager && manager%4==0 && manager<=std::numeric_limits<std::uint32_t>::max()-sizeof(state) && clump && clump%4==0 && clump!=manager && clump<=std::numeric_limits<std::uint32_t>::max()-4,"invalid actor resource identity");
    Require(!actors.TransformBound(h),"actor resource transform is already bound");
}
void BindResource(SceneMonsterInstances& actors,SceneMonsterHandle h,MonsterBehaviorManagerState& state,std::uint32_t manager,std::uint32_t clump,std::uint32_t mode,std::uint32_t extra,PackageReader& package,GameResourceManager resource){
    const auto& record=actors.Source(h);detail::RequireStaticActorResources(package,record.resource);
    resource.words[5]=clump;resource.words[6]=mode;resource.words[8]=extra;
    PoseRuntime runtime(actors.Model(h),resource,manager,clump);(void)resource.CalculateBoundingBox(runtime);resource.FinalizeClumpLoad(runtime);
    (void)resource.UpdateClumpTransform({record.position[0],record.position[1],record.position[2]},false,runtime);
    resource.UpdateClumpRotation({record.rotation[0],record.rotation[1],record.rotation[2]},false,runtime);
    std::copy(resource.words.begin(),resource.words.end(),state.words.begin());
}
}
void InitializeActorResource(SceneMonsterInstances& actors,SceneMonsterHandle h,MonsterBehaviorManagerState& state,std::uint32_t manager,std::uint32_t clump,std::uint32_t mode,std::uint32_t extra,PackageReader& package){
    ValidateBinding(actors,h,state,manager,clump);
    Require(std::all_of(state.words.begin(),state.words.begin()+92,[](auto v){return v==0;}),"actor resource is already constructed");
    GameResourceManager resource;(void)resource.Construct();
    BindResource(actors,h,state,manager,clump,mode,extra,package,resource);
}
void BindConstructedActorResource(SceneMonsterInstances& actors,SceneMonsterHandle h,MonsterBehaviorManagerState& state,std::uint32_t manager,std::uint32_t clump,std::uint32_t mode,std::uint32_t extra,PackageReader& package){
    ValidateBinding(actors,h,state,manager,clump);
    Require(state.words[0]!=0 && state.words[5]==0 && state.words[7]==0 && state.words[9]==0,"actor resource requires an empty constructed primary");
    for(unsigned a=0;a<6;++a)Require(state.words[46+a]==std::bit_cast<std::uint32_t>(1.f),"constructed actor resource requires unit scale");
    GameResourceManager resource;std::copy_n(state.words.begin(),resource.words.size(),resource.words.begin());
    BindResource(actors,h,state,manager,clump,mode,extra,package,resource);
}
}
