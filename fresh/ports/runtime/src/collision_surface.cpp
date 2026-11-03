#include "pal4/portable/collision_surface.h"
#include "pal4/collision_tree_storage.h"
#include "pal4/d3d_face_point_projection.h"
#include "pal4/d3d_plane3.h"
#include "pal4/d3d_line2.h"
#include <cmath>
#include <bit>
#include <cstring>
#include <stdexcept>
namespace pal4::portable {
namespace {
pal4::D3dTriangleVector3 Vector(Point3 p){for(float x:p)if(!std::isfinite(x))throw std::invalid_argument("non-finite collision surface point");return {p[0],p[1],p[2]};}
struct FaceTrace final:pal4::ID3dTriangleTraceRuntime {
    std::uint32_t Classify(pal4::D3dTriangleFaceState& face,std::uint32_t offset,const pal4::D3dTriangleVector2& point,float threshold) override {
        pal4::D3dLine2State edge{};auto* bytes=reinterpret_cast<std::byte*>(&face)+offset;std::memcpy(&edge,bytes,sizeof(edge));
        const auto result=pal4::D3dLine2::Classify(edge,point,threshold);std::memcpy(bytes,&edge,sizeof(edge));return static_cast<std::uint32_t>(result);
    }
    std::uint32_t Intersect(const pal4::D3dTriangleEdgeProbe& probe,pal4::D3dTriangleFaceState& face,std::uint32_t offset,pal4::D3dTriangleVector2& point) override {
        const auto path=std::bit_cast<pal4::D3dLine2State>(probe);pal4::D3dLine2State edge{};std::memcpy(&edge,reinterpret_cast<const std::byte*>(&face)+offset,sizeof(edge));
        return static_cast<std::uint32_t>(pal4::D3dLine2::Intersect(edge,path,&point));
    }
};
struct Height final:pal4::ID3dHeightRuntime {
    const pal4::D3dTriangleFaceState& face;explicit Height(const pal4::D3dTriangleFaceState& f):face(f){}
    pal4::D3dHeightResult Evaluate(float x,float z) override {
        const pal4::D3dPlane3State plane{{pal4::D3dTriangleFace::FloatAt(face,0),pal4::D3dTriangleFace::FloatAt(face,4),pal4::D3dTriangleFace::FloatAt(face,8)},
            {pal4::D3dTriangleFace::FloatAt(face,12),pal4::D3dTriangleFace::FloatAt(face,16),pal4::D3dTriangleFace::FloatAt(face,20)},pal4::D3dTriangleFace::FloatAt(face,24)};
        return {static_cast<float>(pal4::D3dPlane3::HeightAt(plane,x,z)),0};
    }
};
}
struct CollisionSurface::Impl final:pal4::ICollisionTreeFaceQueryRuntime,
    pal4::ICollisionTreeNeighborRuntime,pal4::ICollisionTreePathRuntime,
    pal4::ID3dTriangleStateRuntime,pal4::ID3dTriangleNeighborRuntime,
    pal4::ICollisionTreeMovementRuntime,pal4::ICollisionTreePointProjectionRuntime {
    std::vector<pal4::D3dTriangleFaceState> faces;pal4::CollisionTreeStorageState tree{};FaceTrace trace;
    bool navigation{};std::size_t linked{};std::vector<pal4::D3dFaceCandidate> candidates;
    pal4::D3dFaceTraversalState* traversal{};std::vector<SurfacePathPoint> route;SurfacePathPoint terminal;
    pal4::D3dTriangleFaceState& Face(std::uint32_t handle) override {if(!handle || handle>faces.size())throw std::out_of_range("collision surface face");return faces[handle-1];}
    std::uint32_t ReadFacePointer(std::uint32_t slot) override {if(slot<0x1000 || (slot-0x1000)%4 || (slot-0x1000)/4>=faces.size())throw std::out_of_range("collision surface slot");return (slot-0x1000)/4+1;}
    bool IsOutsideEdges3D(std::uint32_t handle,const pal4::D3dTriangleVector3& point) override{return pal4::D3dTriangleFace::IsOutsideEdges3D(Face(handle),point,trace);}
    void ProjectHeight(std::uint32_t handle,pal4::D3dTriangleVector3& point) override {Height height(Face(handle));(void)pal4::D3dHeightProjection::Project(point,height);}
    pal4::D3dTriangleVector3 Centroid(std::uint32_t handle) override {const auto& f=Face(handle);return {pal4::D3dTriangleFace::FloatAt(f,64),pal4::D3dTriangleFace::FloatAt(f,68),pal4::D3dTriangleFace::FloatAt(f,72)};}
    std::uint32_t TraceToPoint(std::uint32_t handle,const pal4::D3dTriangleVector2& from,const pal4::D3dTriangleVector2& to,pal4::D3dTriangleVector2& point) override {
        pal4::D3dTriangleEdgeProbe probe{};probe.first=from;probe.last=to;std::uint32_t neighbor{},edge{};
        return pal4::D3dTriangleFace::TraceEdges(Face(handle),probe,neighbor,edge,point,trace);
    }
    std::uint32_t Handle(const pal4::D3dTriangleFaceState& face){const auto i=&face-faces.data();if(i<0 || static_cast<std::size_t>(i)>=faces.size())throw std::logic_error("foreign collision face");return static_cast<std::uint32_t>(i)+1;}
    std::span<pal4::D3dFaceCandidate> Entries(pal4::D3dFaceTraversalState& state) override {
        if(state.words[2]!=0x1000000 || state.words[3]<state.words[2] || (state.words[3]-state.words[2])%8 || (state.words[3]-state.words[2])/8>candidates.size())throw std::logic_error("invalid face queue");
        return {candidates.data(),(state.words[3]-state.words[2])/8};
    }
    std::span<pal4::D3dFaceCandidate> Append(pal4::D3dFaceTraversalState& state,const pal4::D3dFaceCandidate& entry) override {
        const auto count=Entries(state).size();if(count>=candidates.size())throw std::length_error("face queue capacity exceeded");candidates[count]=entry;state.words[3]+=8;return Entries(state);
    }
    void InitializeSeedFace(std::uint32_t face,pal4::D3dFaceTraversalState& state) override {traversal=&state;(void)pal4::D3dTriangleFace::UpdateState(Face(face),1,0,0,*this);}
    void PropagateFace(std::uint32_t face,pal4::D3dFaceTraversalState& state) override {if(&state!=traversal)throw std::logic_error("face traversal changed");(void)pal4::D3dTriangleFace::PropagateNeighborState(Face(face),1,*this);}
    std::uint32_t Marker(std::uint32_t face) override{return pal4::D3dTriangleFace::Marker(Face(face));}
    pal4::D3dTriangleVector3 Midpoint(std::uint32_t face,std::uint32_t edge) override {pal4::D3dTriangleVector3 point{};return pal4::D3dTriangleFace::CopyMidpoint(Face(face),edge,point);}
    pal4::D3dTriangleVector3 Project(std::uint32_t face,const pal4::D3dTriangleVector3& point) override {Height height(Face(face));pal4::D3dTriangleVector3 result{};(void)pal4::D3dFacePointProjection::Project(result,Face(face),point,trace,height);return result;}
    std::uint32_t Neighbor(std::uint32_t face,std::uint32_t edge) override{return pal4::D3dTriangleFace::Neighbor(Face(face),edge);}
    void BeginActorPath(std::uint32_t,const pal4::CollisionTreeStorageState&,const pal4::D3dTriangleVector3& start,std::uint32_t start_face,const pal4::D3dTriangleVector3& target,std::uint32_t target_face) override {route.clear();route.push_back({{start.x,start.y,start.z},start_face});terminal={{target.x,target.y,target.z},target_face};}
    void AddActorPoint(std::uint32_t,const pal4::D3dTriangleVector3& point,std::uint32_t face) override {route.push_back({{point.x,point.y,point.z},face});if(route.size()>faces.size()+2)throw std::logic_error("cyclic face route");}
    void EndActorPath(std::uint32_t) override {route.push_back(terminal);}
    std::uint32_t InstructionData(std::uint32_t instruction) override {if(instruction!=1 || !traversal)throw std::logic_error("invalid face instruction");return traversal->words[5];}
    pal4::D3dTriangleVector3 VectorData(std::uint32_t instruction) override {(void)InstructionData(instruction);return {std::bit_cast<float>(traversal->words[6]),std::bit_cast<float>(traversal->words[7]),std::bit_cast<float>(traversal->words[8])};}
    void UpdateTransform(std::uint32_t instruction,pal4::D3dTriangleFaceState& face,bool incremental) override {(void)InstructionData(instruction);if(incremental)(void)pal4::D3dFaceTraversal::UpdateFacePriority(*traversal,Handle(face),face,*this);else pal4::D3dFaceTraversal::PushFace(*traversal,Handle(face),face,*this);}
    void UpdateNeighbor(std::uint32_t neighbor,std::uint32_t instruction,pal4::D3dTriangleFaceState& source,float distance) override {(void)pal4::D3dTriangleFace::UpdateState(Face(neighbor),instruction,Handle(source),distance,*this);}
    std::uint32_t Trace(std::uint32_t face,const pal4::D3dLine2State& line,std::uint32_t& neighbor,std::uint32_t& edge,pal4::D3dTriangleVector2& intersection) override {pal4::D3dTriangleEdgeProbe probe{};probe.first=line.start;probe.last=line.end;return pal4::D3dTriangleFace::TraceEdges(Face(face),probe,neighbor,edge,intersection,trace);}
    void AdjustEdge(std::uint32_t face,std::uint32_t edge,pal4::D3dLine2State& line) override {pal4::D3dTriangleEdgeProbe probe{};probe.first=line.start;probe.last=line.end;(void)pal4::D3dTriangleFace::AdjustProbeAlongEdge(Face(face),edge,probe,trace);line.start=probe.first;line.end=probe.last;}
    void ClampPoint(std::uint32_t face,pal4::D3dTriangleVector2& point) override {(void)pal4::D3dTriangleFace::ClampProbeToFace(Face(face),point,trace);}
    std::uint32_t SelectFace(std::uint32_t tree_handle,const pal4::D3dTriangleVector3& point) override {if(tree_handle!=1)throw std::logic_error("invalid surface tree");return pal4::CollisionTreeStorage::SelectFace(tree,point,*this);}
    pal4::D3dTriangleVector3 Project(std::uint32_t tree_handle,std::uint32_t face,const pal4::D3dTriangleVector3& point) override {if(tree_handle!=1)throw std::logic_error("invalid surface tree");return Project(face,point);}
    bool IsOutsideEdges(std::uint32_t face,const pal4::D3dTriangleVector3& point) override{return IsOutsideEdges3D(face,point);}
    void ClampPoint(std::uint32_t face,pal4::D3dTriangleVector3& point) override {(void)pal4::D3dTriangleFace::ClampPointToFace(Face(face),point,trace);}
};
CollisionSurface::CollisionSurface(std::span<const CollisionTriangle> floor):impl_(std::make_unique<Impl>()) {
    if(floor.size()>1000000)throw std::invalid_argument("too many collision surface faces");
    // P4CL保留DFF流中的前两个顶点顺序；原RpGeometryStreamRead
    // 788A09..788A23交换这两个halfword后才交给游戏建面。
    impl_->faces.resize(floor.size());for(std::size_t i=0;i<floor.size();++i){const auto& p=floor[i].points;pal4::D3dTriangleFace::Construct(impl_->faces[i],Vector(p[1]),Vector(p[0]),Vector(p[2]));}
    (void)pal4::CollisionTreeStorage::Initialize(impl_->tree,1);
    impl_->tree.words[1]=0x1000;impl_->tree.words[2]=0x1000+static_cast<std::uint32_t>(floor.size())*4;
}
CollisionSurface::~CollisionSurface()=default;
std::size_t CollisionSurface::Size() const{return impl_->faces.size();}
std::uint32_t CollisionSurface::Select(Point3 point){return pal4::CollisionTreeStorage::SelectFace(impl_->tree,Vector(point),*impl_);}
Point3 CollisionSurface::Project(std::uint32_t handle,Point3 point) {
    auto& face=impl_->Face(handle);Height height(face);pal4::D3dTriangleVector3 result{};
    (void)pal4::D3dFacePointProjection::Project(result,face,Vector(point),impl_->trace,height);return {result.x,result.y,result.z};
}
std::uint32_t CollisionSurface::Snap(Point3& point) {
    auto p=Vector(point);if(!Select(point))return 0;
    const auto face=pal4::CollisionTreeStorage::SnapPointToFace(1,p,*impl_);point={p.x,p.y,p.z};return face;
}
void CollisionSurface::EnableNavigation() {
    if(impl_->navigation)return;
    (void)pal4::CollisionTreeStorage::LinkFaceNeighbors(impl_->tree,*impl_);
    for(const auto& face:impl_->faces)for(unsigned edge=0;edge<3;++edge)if(pal4::D3dTriangleFace::Neighbor(face,edge))++impl_->linked;
    impl_->candidates.resize(impl_->faces.size());impl_->tree.words[8]=impl_->tree.words[9]=0x1000000;
    impl_->tree.words[10]=0x1000000+static_cast<unsigned>(impl_->faces.size())*8;impl_->navigation=true;
}
bool CollisionSurface::TracePath(std::uint32_t from_face,Point3 from,std::uint32_t target_face,Point3 target,std::vector<SurfacePathPoint>& output) {
    output.clear();if(!from_face || !target_face)return false;EnableNavigation();
    try {const auto found=pal4::CollisionTreeStorage::TraceFacePath(impl_->tree,1,from_face,Vector(from),target_face,Vector(target),*impl_);impl_->traversal=nullptr;if(found)output=impl_->route;return found;}
    catch(...){impl_->traversal=nullptr;throw;}
}
std::uint32_t CollisionSurface::Trace(std::uint32_t face,Point3 from,Point3 to,std::uint32_t& neighbor) {EnableNavigation();const auto a=Vector(from),b=Vector(to);pal4::D3dLine2State line{};pal4::D3dLine2::SetEndpoints(line,{a.x,a.z},{b.x,b.z});unsigned edge{};pal4::D3dTriangleVector2 intersection{};return impl_->Trace(face,line,neighbor,edge,intersection);}
std::uint32_t CollisionSurface::Advance(Point3 from,std::uint32_t face,Point3& to,std::uint32_t& final_face) {EnableNavigation();auto destination=Vector(to);const auto status=pal4::CollisionTreeStorage::AdvanceAcrossFaces(impl_->tree,Vector(from),face,destination,final_face,*impl_);to={destination.x,destination.y,destination.z};return status;}
void CollisionSurface::Clamp(std::uint32_t face,Point3& point){auto value=Vector(point);impl_->ClampPoint(face,value);point={value.x,value.y,value.z};}
std::size_t CollisionSurface::LinkedEdges() const{return impl_->linked;}
}
