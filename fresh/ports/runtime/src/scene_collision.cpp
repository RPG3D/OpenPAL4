#include "pal4/portable/scene_collision.h"
#include "pal4/config_composite_collision.h"
#include "pal4/collision_triangle_containment.h"
#include "pal4/game_vector_math.h"
#include "pal4/pal_movement_collision_router.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <ostream>
#include <stdexcept>
namespace pal4::portable {
namespace {
void Require(bool value,const char* message){if(!value)throw std::logic_error(message);}
void Finite(Point3 value){for(float v:value)if(!std::isfinite(v))throw std::invalid_argument("non-finite collision geometry");}
Point3 Difference(Point3 a,Point3 b){for(unsigned i=0;i<3;++i)a[i]-=b[i];return a;}
double Dot(Point3 a,Point3 b){double sum{};for(unsigned i=0;i<3;++i)sum+=double(a[i])*b[i];return sum;}
Point3 Normal(const CollisionTriangle& t){
    const auto a=Difference(t.points[1],t.points[0]),b=Difference(t.points[2],t.points[0]);
    return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};
}
const pal4::GameMathTables& Tables(){static const auto tables=[]{pal4::GameMathTables t;(void)t.CalculateSquareRootTable();return t;}();return tables;}
}
struct SceneCollision::Impl final:pal4::IConfigCompositeRuntime,pal4::IConfigCompositeCollisionRuntime,pal4::IPalMovementCollisionRuntime {
    struct Shape {
        CollisionGroup group;std::vector<CollisionTriangle> triangles;CollisionObjectProperties properties;std::function<bool()> eligibility;
        std::vector<Point3> normals;std::vector<std::array<Point3,2>> bounds;
        void Prepare(){
            normals.reserve(triangles.size());bounds.reserve(triangles.size());
            for(const auto& triangle:triangles){
                const auto normal=Normal(triangle);Finite(normal);normals.push_back(normal);
                auto low=triangle.points[0],high=low;
                for(auto point:triangle.points)for(unsigned i=0;i<3;++i){low[i]=std::min(low[i],point[i]);high[i]=std::max(high[i],point[i]);}
                bounds.push_back({low,high});
            }
        }
    };
    pal4::ConfigCompositeState state;
    std::map<std::uint32_t,std::uint32_t> words;
    std::map<std::uint32_t,Shape> shapes;
    std::uint32_t next_node{0x1000},next_shape{0x100000},resolves{},sphere_queries{},segment_queries{};
    const CollisionTriangle* current_triangle{};
    Event event;bool resolving{},inside_event{};std::vector<std::uint32_t> retired_nodes;
    std::array<float,4>* upper{};std::array<float,4>* lower{};Point3* movement{};
    explicit Impl(Event e):event(std::move(e)){(void)pal4::ConfigComposite::Initialize(state,0x70000000,*this);}
    ~Impl(){(void)pal4::ConfigComposite::CleanupFull(state,*this);}
    Shape& Get(std::uint32_t handle){const auto it=shapes.find(handle);Require(it!=shapes.end(),"invalid collision object handle");return it->second;}
    const Shape& Get(std::uint32_t handle) const{const auto it=shapes.find(handle);Require(it!=shapes.end(),"invalid collision object handle");return it->second;}
    std::uint32_t AllocateNode(std::uint32_t size) override {
        Require((size==8 || size==32) && next_node<0xf0000,"invalid collision list allocation");const auto result=next_node;next_node+=32;
        for(unsigned offset=0;offset<size;offset+=4)words.emplace(result+offset,0);
        return result;
    }
    void WriteWord(std::uint32_t address,std::uint32_t value) override {
        const auto found=words.find(address);Require(found!=words.end(),"invalid collision list write");found->second=value;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if(const auto found=words.find(address);found!=words.end())return found->second;
        const auto handle=address & ~0xffu;
        if(const auto found=shapes.find(handle);found!=shapes.end()){
            if(address==handle+4)return handle;
            if(address==handle+60)return found->second.properties.owner;
        }
        throw std::logic_error("invalid collision list read");
    }
    std::uint32_t FreeNode(std::uint32_t handle) override {
        Require(words.contains(handle),"invalid collision list release");
        if(resolving)retired_nodes.push_back(handle);
        else for(unsigned i=0;i<32;i+=4)words.erase(handle+i);
        return 1;
    }
    std::uint32_t DestroyClump(std::uint32_t) override {throw std::logic_error("collision adapter owns triangles, not clumps");}
    std::uint32_t GameInstance() override {throw std::logic_error("collision adapter does not load resources");}
    std::uint32_t LoadClump(const char*,std::uint32_t,std::uint32_t) override {throw std::logic_error("collision adapter does not load resources");}
    void AddClumpBounds(std::uint32_t,std::uint32_t,pal4::ConfigCompositeState&,std::uint32_t) override {throw std::logic_error("collision clump bounds are not bound");}
    std::uint32_t ForEachAtomic(std::uint32_t,std::uint32_t,pal4::ConfigCompositeState&) override {throw std::logic_error("collision adapter requires typed registration");}
    std::uint32_t ForEachAtomicWithContext(std::uint32_t,std::uint32_t,const std::array<std::uint32_t,5>&) override {throw std::logic_error("collision adapter requires typed registration");}
    bool CheckLineSegment(const pal4::ConfigCollisionPosition& origin,std::uint32_t atomic,Point3& delta,Point3* normal,float* distance,std::uint32_t mode) override {
        return pal4::ConfigCompositeCollision::CheckLineSegmentIntersection(&origin,atomic,delta,normal,distance,mode,*this);
    }
    bool ProcessWorldCollision(const pal4::CollisionSegment& segment,std::uint32_t atomic,pal4::ConfigCollisionScratch& scratch) override {
        ++segment_queries;const auto direction=Difference(segment.end,segment.start);const auto squared=Dot(direction,direction);
        if(squared==0)return false;
        const auto& shape=Get(atomic);
        for(std::size_t index=0;index<shape.triangles.size();++index){
            const auto& triangle=shape.triangles[index];const auto& bounds=shape.bounds[index];bool overlap=true;
            for(unsigned axis=0;axis<3;++axis)if(std::max(segment.start[axis],segment.end[axis])<bounds[0][axis] || std::min(segment.start[axis],segment.end[axis])>bounds[1][axis]){overlap=false;break;}
            if(!overlap)continue;
            Point3 hit{};if(!pal4::CollisionTriangleContainment::IntersectLineWithTriangle(segment.start,direction,triangle.points,&hit))continue;
            const auto fraction=Dot(Difference(hit,segment.start),direction)/squared;
            if(fraction>=0 && fraction<=1){scratch[0]=triangle.triangle;scratch[1]=atomic;return true;}
        }
        return false;
    }
    void NormalizeMovement(Point3& output,const Point3& value) override {
        const auto squared=Dot(value,value);output={};if(squared==0)return;
        const double inverse=1/std::sqrt(squared);for(unsigned i=0;i<3;++i)output[i]=static_cast<float>(value[i]*inverse);
    }
    bool HasPatchMesh(std::uint32_t handle) override{return Get(handle).properties.patch;}
    bool HasHierarchy(std::uint32_t handle) override{return Get(handle).properties.hierarchy;}
    bool HasPluginData(std::uint32_t handle) override{const auto& shape=Get(handle);return shape.eligibility?shape.eligibility():shape.properties.plugin;}
    void SendEvent(std::uint32_t callback,bool entering,std::uint32_t previous){
        Require(bool(event),"collision event callback is not bound");inside_event=true;
        try{event(callback,entering,previous);}catch(...){inside_event=false;throw;}
        inside_event=false;
    }
    void NotifyEnter(std::uint32_t callback,std::uint32_t previous,pal4::ConfigCompositeState&) override {SendEvent(callback,true,previous);}
    void NotifyLeave(std::uint32_t callback,pal4::ConfigCompositeState&) override {SendEvent(callback,false,1);}
    void FinishQuery(){
        resolving=false;upper=lower=nullptr;movement=nullptr;current_triangle=nullptr;
        for(auto handle:retired_nodes)for(unsigned i=0;i<32;i+=4)words.erase(handle+i);
        retired_nodes.clear();
    }
    Point3 VertexWorldPosition(std::uint32_t reference,std::uint32_t transform) override {
        Require(current_triangle && transform==1 && reference>=1 && reference<=3,"invalid collision vertex handle");return current_triangle->points[reference-1];
    }
    Point3 TransformNormal(const Point3& normal,std::uint32_t transform) override {Require(transform==1,"collision geometry must use world coordinates");return normal;}
    float VectorMagnitudeApprox(const Point3& value) override {return static_cast<float>(pal4::GameVectorMath::ApproximateMagnitude(value,Tables()));}
    std::uint32_t ObjectTransform(std::uint32_t frame) override {(void)Get(frame);return 1;}
    void ForAllAtomicIntersections(std::uint32_t atomic,const pal4::ConfigCollisionSphereQuery& query,std::uint32_t callback,pal4::ConfigCollisionClosestPoint& closest) override {
        Require(callback==0x5ffcb0 && query.type==3,"invalid collision sphere callback");++sphere_queries;
        Finite(query.center);const auto& shape=Get(atomic);
        for(std::size_t index=0;index<shape.triangles.size();++index){
            const auto& triangle=shape.triangles[index];const auto normal=shape.normals[index];if(Dot(normal,normal)==0)continue;
            const auto& bounds=shape.bounds[index];double squared_distance{};
            for(unsigned axis=0;axis<3;++axis){const double d=double(query.center[axis])-std::clamp(query.center[axis],bounds[0][axis],bounds[1][axis]);squared_distance+=d*d;}
            if(squared_distance>double(query.radius)*query.radius)continue;
            current_triangle=&triangle;pal4::ConfigCollisionTriangle source;source.normal=normal;source.vertex_references={1,2,3};
            pal4::ConfigCollisionClosestPoint candidate;candidate.distance=std::numeric_limits<float>::max();candidate.object_transform=1;
            (void)pal4::ConfigCompositeCollision::RecordClosestTrianglePoint(query.center,source,0,candidate,*this);
            // 公共球体查询的筛选边界；最近点与位移修正仍调用原游戏处理器。
            const auto delta=Difference(query.center,candidate.point);
            if(Dot(delta,delta)<=double(query.radius)*query.radius && candidate.distance<closest.distance)closest=candidate;
        }
        current_triangle=nullptr;
    }
    std::uint32_t ProcessClumpCollision(const pal4::CollisionSegment&,std::uint32_t,pal4::ConfigCollisionScratch&) override {throw std::logic_error("floor collision uses SceneMovement");}
    void ValidateStage(std::uint32_t object,std::uint32_t first,std::uint32_t second,std::uint32_t third,std::uint32_t a,std::uint32_t b){
        Require(resolving && upper && lower && movement && object==0x70000000 && first==1 && second==2 && third==3 && a==0 && b==0,"invalid collision stage handles");
    }
    std::uint32_t CheckFirst(std::uint32_t object,std::uint32_t first,std::uint32_t second,std::uint32_t third,std::uint32_t&,std::uint32_t a,std::uint32_t b) override {
        ValidateStage(object,first,second,third,a,b);return pal4::ConfigCompositeCollision::CheckSecondList(state,upper,lower,*movement,0,nullptr,nullptr,*this);
    }
    std::uint32_t CheckSecond(std::uint32_t object,std::uint32_t first,std::uint32_t second,std::uint32_t third,std::uint32_t& hit,std::uint32_t a,std::uint32_t b) override {
        ValidateStage(object,first,second,third,a,b);return pal4::ConfigCompositeCollision::CheckThirdList(state,upper,lower,*movement,hit,0,0,*this);
    }
    std::uint32_t CheckThird(std::uint32_t object,std::uint32_t first,std::uint32_t second,std::uint32_t excluded,std::uint32_t third,std::uint32_t& hit,std::uint32_t a,std::uint32_t b) override {
        ValidateStage(object,first,second,third,a,b);return pal4::ConfigCompositeCollision::CheckFourthList(state,upper,lower,excluded,*movement,hit,0,0,*this);
    }
};
SceneCollision::SceneCollision(Event event):impl_(std::make_unique<Impl>(std::move(event))){}
SceneCollision::~SceneCollision()=default;
std::uint32_t SceneCollision::Add(CollisionGroup group,std::span<const CollisionTriangle> triangles,CollisionObjectProperties properties,std::function<bool()> eligibility){
    auto& s=*impl_;Require(!s.resolving,"collision list mutation during query");
    Require(group==CollisionGroup::intri || group==CollisionGroup::wall || group==CollisionGroup::object,"invalid collision group");
    Require(group==CollisionGroup::object || properties==CollisionObjectProperties{},"static collision groups use original fixed properties");
    Require(group!=CollisionGroup::object || properties.owner!=0,"configured collision requires its source owner");
    Require(!triangles.empty() && triangles.size()<=1000000 && s.next_shape<0x10000000,"invalid collision shape size");
    for(const auto& triangle:triangles)for(auto point:triangle.points)Finite(point);
    const auto handle=s.next_shape;s.next_shape+=0x100;
    Impl::Shape shape{group,{triangles.begin(),triangles.end()},properties,std::move(eligibility),{},{}};shape.Prepare();
    s.shapes.emplace(handle,std::move(shape));
    try{
        if(group==CollisionGroup::intri)(void)pal4::ConfigComposite::RegisterIntriAtomic(s.state,handle,s);
        else if(group==CollisionGroup::wall)(void)pal4::ConfigComposite::RegisterWallAtomic(s.state,handle,s);
        else (void)pal4::ConfigComposite::RegisterConfiguredAtomic(s.state,handle,{0x70000000,properties.mode,properties.hard_block,properties.proximity,properties.callback},s);
    }catch(...){s.shapes.erase(handle);throw;}
    return handle;
}
void SceneCollision::Replace(std::uint32_t handle,std::span<const CollisionTriangle> triangles){
    auto& s=*impl_;Require(!s.resolving && s.Get(handle).group==CollisionGroup::object,"collision replacement requires an idle configured object");
    Require(!triangles.empty() && triangles.size()<=1000000,"invalid replacement collision size");
    for(const auto& triangle:triangles)for(auto point:triangle.points)Finite(point);
    const auto& old=s.Get(handle);Impl::Shape replacement{old.group,{triangles.begin(),triangles.end()},old.properties,old.eligibility,{},{}};
    replacement.Prepare();std::swap(s.Get(handle),replacement);
}
void SceneCollision::Remove(std::uint32_t handle){
    auto& s=*impl_;Require((!s.resolving || s.inside_event) && s.Get(handle).group==CollisionGroup::object,"collision removal requires a configured object");
    (void)pal4::ConfigComposite::UnregisterAtomic(s.state,handle,s);s.shapes.erase(handle);
}
void SceneCollision::Resolve(std::array<float,4>& upper,std::array<float,4>& lower,Point3& displacement,std::uint32_t excluded){
    auto& s=*impl_;Require(!s.resolving,"recursive collision query");Finite(displacement);
    for(const auto* point:{&upper,&lower}){Finite({(*point)[0],(*point)[1],(*point)[2]});Require(std::isfinite((*point)[3]) && (*point)[3]>=0,"invalid collision radius");}
    s.upper=&upper;s.lower=&lower;s.movement=&displacement;s.resolving=true;
    try{++s.resolves;(void)pal4::PalMovementCollisionRouter::CheckStages(0x70000000,1,1,1,1,2,3,excluded,s);}
    catch(...){s.FinishQuery();throw;}
    s.FinishQuery();
}
void SceneCollision::InterruptEventTraversal(){auto& s=*impl_;Require(s.resolving && s.inside_event,"collision interrupt requires an event callback");s.state.words[22]=1;}
void SceneCollision::WriteReport(std::ostream& output) const {
    const auto& s=*impl_;output<<"{\"shapes\":"<<s.shapes.size()<<",\"intri\":"<<s.state.words[6]<<",\"wall\":"<<s.state.words[9]<<",\"objects\":"<<s.state.words[12]<<",\"resolves\":"<<s.resolves<<",\"sphere_queries\":"<<s.sphere_queries<<",\"segment_queries\":"<<s.segment_queries<<'}';
    if(!output)throw std::runtime_error("cannot write scene collision report");
}
}
