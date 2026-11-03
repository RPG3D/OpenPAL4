#include "pal4/portable/surface_motion.h"
#include "pal4/collision_actor.h"
#include "pal4/base_object_callback_data.h"
#include <bit>
#include <cmath>
#include "pal4/portable/object_memory.h"
#include <stdexcept>
namespace pal4::portable {
static_assert(sizeof(SurfaceMotionImage)==sizeof(pal4::CollisionActorState));
namespace {
void Require(bool value,const char* message){if(!value)throw std::logic_error(message);}
pal4::D3dTriangleVector3 Vector(Point3 p){for(auto v:p)Require(std::isfinite(v),"invalid motion point");return {p[0],p[1],p[2]};}
Point3 Point(pal4::D3dTriangleVector3 p){return {p.x,p.y,p.z};}
}
struct SurfaceMotion::Impl final:pal4::ICollisionActorPathRuntime,pal4::ICollisionActorMovementRuntime,
    pal4::ICollisionActorPlacementRuntime,pal4::IBaseObjectCollisionRuntime {
    CollisionSurface& surface;MovementCollisionQuery collision;pal4::CollisionActorState state{};
    ObjectMemory local_memory{{0x100000,0x70000000,64u*1024u*1024u,100000}};ObjectMemory& memory;
    unsigned trace_budget{},actor_token{};bool closed{},borrowed{};
    Impl(CollisionSurface& s,MovementCollisionQuery c,unsigned token,ObjectMemory* shared=nullptr):surface(s),collision(std::move(c)),memory(shared?*shared:local_memory),actor_token(token){Require(bool(collision),"world collision query is not bound");Require(token && token%4==0 && token<=0xffffffffu-sizeof(state),"invalid collision actor token");}
    unsigned ReadWord(unsigned address) override{return memory.ReadDword(address);}
    void WriteWord(unsigned address,unsigned value) override{memory.WriteDword(address,value);}
    unsigned Allocate(unsigned size) override {Require(size==24,"invalid path allocation");return memory.Allocate(size);}
    void Free(unsigned address) override{(void)memory.Free(address);}
    int FaceCount(unsigned tree) override{Require(tree==1,"invalid path tree");return static_cast<int>(surface.Size());}
    int RandomValue() override{throw std::logic_error("random movement is not bound");}
    unsigned FaceAt(unsigned tree,unsigned index) override{(void)FaceCount(tree);Require(index<surface.Size(),"invalid path face index");return index+1;}
    pal4::D3dTriangleVector3 Centroid(unsigned) override{throw std::logic_error("random path centroid is not bound");}
    unsigned SelectFace(unsigned tree,const pal4::D3dTriangleVector3& p) override{Require(tree==1,"invalid placement tree");return surface.Select(Point(p));}
    pal4::D3dTriangleVector3 Project(unsigned tree,unsigned face,const pal4::D3dTriangleVector3& p) override{Require(tree==1,"invalid projection tree");return Vector(surface.Project(face,Point(p)));}
    bool TracePath(unsigned tree,unsigned path_address,unsigned start_face,const pal4::D3dTriangleVector3& start,const pal4::D3dTriangleVector3& target,unsigned target_face,unsigned mode) override {
        Require(tree==1 && path_address==actor_token+0x2c && mode==0,"invalid route request");
        std::vector<SurfacePathPoint> route;if(!surface.TracePath(start_face,Point(start),target_face,Point(target),route))return false;
        pal4::D3dFacePathState path;for(unsigned i=0;i<12;++i)path.words[i]=state.words[11+i];
        (void)pal4::D3dFacePath::Reset(path,1,start,start_face,target,target_face,*this);
        for(std::size_t i=1;i+1<route.size();++i)(void)pal4::D3dFacePath::Append(path,Vector(route[i].position),route[i].face,*this);
        (void)pal4::D3dFacePath::AppendEnd(path,*this);
        for(unsigned i=0;i<12;++i)state.words[11+i]=path.words[i];
        return true;
    }
    unsigned Trace(unsigned face,const pal4::D3dLine2State& line,unsigned& neighbor) override{Require(trace_budget>0,"collision trace budget exceeded");--trace_budget;return surface.Trace(face,{line.start.x,0,line.start.z},{line.end.x,0,line.end.z},neighbor);}
    void ClampFacePoint(unsigned face,pal4::D3dTriangleVector3& p) override{auto point=Point(p);surface.Clamp(face,point);p=Vector(point);}
    bool ProcessCollision(const pal4::BaseObjectVector3& first,const pal4::BaseObjectVector3& last,unsigned world) override {Require(world==1,"invalid world collision handle");return collision({first.x,first.y,first.z},{last.x,last.y,last.z});}
    bool TestClearance(unsigned callback,const pal4::D3dTriangleVector3& first,const pal4::D3dTriangleVector3& last,unsigned world) override{Require(callback==0x444390,"invalid clearance callback");return pal4::BaseObjectCallbackData::CheckCollision({first.x,first.y,first.z},{last.x,last.y,last.z},world,*this);}
    unsigned AdvanceAcrossFaces(unsigned tree,const pal4::D3dTriangleVector3& from,unsigned face,pal4::D3dTriangleVector3& to,unsigned& final_face) override{Require(tree==1,"invalid motion tree");auto destination=Point(to);const auto status=surface.Advance(Point(from),face,destination,final_face);to=Vector(destination);return status;}
};
SurfaceMotion::SurfaceMotion(CollisionSurface& surface,Point3 position,float height,MovementCollisionQuery collision,unsigned actor_token)
    :SurfaceMotion(surface,position,height,std::move(collision),actor_token,nullptr){}
SurfaceMotion::SurfaceMotion(CollisionSurface& surface,Point3 position,float height,MovementCollisionQuery collision,unsigned actor_token,ObjectMemory* memory,std::optional<SurfaceMotionImage> constructed):impl_(std::make_unique<Impl>(surface,std::move(collision),actor_token,memory)) {
    Require(std::isfinite(height) && height>=0,"invalid movement height");
    if(constructed){
        Require(memory!=nullptr,"borrowed movement requires original object storage");
        const auto& image=*constructed;const auto sentinel=image[21];
        Require(image[0]==pal4::CollisionActor::kVtableAddress && image[1]==0 && image[2]==0 && (image[10]&0xff)==0 && image[22]==0 && sentinel,
            "borrowed movement requires an empty constructed CollisionActor");
        Require(memory->ReadDword(sentinel)==sentinel && memory->ReadDword(sentinel+4)==sentinel,"borrowed movement path sentinel is not empty");
        impl_->state.words=image;impl_->borrowed=true;
    }else (void)pal4::CollisionActor::Construct(impl_->state,0,*impl_);
    try {
        const auto face=surface.Select(position);Require(face!=0,"movement start has no floor");
        (void)pal4::CollisionActor::Place(impl_->state,impl_->actor_token,1,0x41873333,Vector(position),face,std::bit_cast<unsigned>(height),*impl_);
        (void)pal4::CollisionActor::SetCallback108(impl_->state,0x444390);
    } catch(...) {
        if(!impl_->borrowed){(void)pal4::CollisionActor::Destroy(impl_->state,*impl_);}
        throw;
    }
}
SurfaceMotion::~SurfaceMotion(){Close();}
void SurfaceMotion::Close(){if(impl_->closed)return;(void)pal4::CollisionActor::StopMovement(impl_->state);if(impl_->borrowed)(void)pal4::CollisionActor::ClearBinding(impl_->state);else (void)pal4::CollisionActor::Destroy(impl_->state,*impl_);impl_->closed=true;}
bool SurfaceMotion::Start(Point3 from,Point3 to,unsigned target_face){Require(!impl_->closed,"closed surface motion");(void)Vector(from);(void)Vector(to);return pal4::CollisionActor::RequestPath(impl_->state,impl_->actor_token,Vector(from),Vector(to),target_face,0,*impl_);}
void SurfaceMotion::SetSpeed(float speed){Require(!impl_->closed,"closed surface motion");Require(std::isfinite(speed) && speed>=0,"invalid movement speed");(void)pal4::CollisionActor::SetSpeedBits(impl_->state,std::bit_cast<unsigned>(speed));}
void SurfaceMotion::Advance(float elapsed){(void)Advance(elapsed,1);}
std::uint32_t SurfaceMotion::Advance(float elapsed,std::uint32_t event){Require(!impl_->closed,"closed surface motion");Require(std::isfinite(elapsed) && elapsed>=0,"invalid movement elapsed");impl_->trace_budget=static_cast<unsigned>(impl_->surface.Size())*100+100;return pal4::CollisionActor::UpdateMovement(impl_->state,impl_->actor_token,event,elapsed,*impl_);}
SurfaceMotionImage SurfaceMotion::Image() const{return impl_->state.words;}
void SurfaceMotion::Stop(){Require(!impl_->closed,"closed surface motion");(void)pal4::CollisionActor::StopMovement(impl_->state);}
bool SurfaceMotion::Active() const{return !impl_->closed && pal4::CollisionActor::PathEnabled(impl_->state)!=0;}
Point3 SurfaceMotion::Position() const{return {std::bit_cast<float>(impl_->state.words[3]),std::bit_cast<float>(impl_->state.words[4]),std::bit_cast<float>(impl_->state.words[5])};}
Point3 SurfaceMotion::Velocity() const{return {std::bit_cast<float>(impl_->state.words[6]),std::bit_cast<float>(impl_->state.words[7]),std::bit_cast<float>(impl_->state.words[8])};}
unsigned SurfaceMotion::Face() const{return impl_->state.words[2];}
unsigned SurfaceMotion::Status() const{return impl_->state.words[36];}
std::vector<SurfacePathPoint> SurfaceMotion::Path() const {
    std::vector<SurfacePathPoint> result;const auto sentinel=impl_->state.words[21];
    for(auto node=impl_->ReadWord(sentinel);node!=sentinel;node=impl_->ReadWord(node)){Require(result.size()<100000,"cyclic movement path");result.push_back({{std::bit_cast<float>(impl_->ReadWord(node+8)),std::bit_cast<float>(impl_->ReadWord(node+12)),std::bit_cast<float>(impl_->ReadWord(node+16))},impl_->ReadWord(node+20)});}return result;
}
}
