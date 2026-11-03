#include "pal4/portable/game_camera.h"
#include "pal4/camera_matrix.h"
#include "pal4/camera_system_initializer.h"
#include "pal4/camera_mode_configuration.h"
#include "pal4/camera_controller_access.h"
#include "pal4/camera_bounds.h"
#include "pal4/camera_parameter_setters.h"
#include "pal4/camera_environment_probe.h"
#include "pal4/camera_obstacle_check.h"
#include "pal4/collision_triangle_containment.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <ostream>
#include <stdexcept>

namespace pal4::portable {
namespace {
using Vector=std::array<float,3>;
float Read(const pal4::CameraDataState& s,std::size_t o){return std::bit_cast<float>(s.At(o));}
void Write(pal4::CameraDataState& s,std::size_t o,float value){s.At(o)=std::bit_cast<std::uint32_t>(value);}
Vector ReadVector(const pal4::CameraDataState& s,std::size_t o){return {Read(s,o),Read(s,o+4),Read(s,o+8)};}
float Dot(Vector a,Vector b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
void Finite(Vector p){for(float f:p)if(!std::isfinite(f))throw std::invalid_argument("non-finite game camera vector");}
class Camera final:public pal4::ICameraDataRuntime,public pal4::ICameraMatrixRuntime,
    public pal4::ICameraBoundsRuntime,public pal4::ICameraParameterRuntime,
    public pal4::ICameraAnimationRuntime,public pal4::ICameraEnvironmentRuntime,public pal4::ICameraObstacleRuntime {
public:
    pal4::CameraDataState state;
    GameCameraConfig config;
    const Scene* world{};
    Vector minimum{},maximum{};
    std::uint32_t anchor{1};
    pal4::CameraBounds bounds{*this};pal4::CameraMatrix matrix{*this,bounds};
    pal4::CameraParameterSetters parameters{*this};
    pal4::CameraEnvironmentProbe environment{*this};pal4::CameraObstacleCheck obstacle{*this};
    pal4::CameraAnimation animation{*this,environment,obstacle};
    explicit Camera(GameCameraConfig value):config(value) {
        pal4::CameraData data(*this);data.Construct(state);state.At(0)=1;
        // CameraSystemInitializer: target 0, distance 350, pitch 30, yaw 225.
        Set({0,0,0},{225,30,0},350);
    }
    void Set(Vector target,Vector angles,float distance) {
        anchor=1;
        Finite(target);Finite(angles);if(!std::isfinite(distance))throw std::invalid_argument("non-finite camera distance");
        for(unsigned i=0;i<3;++i)Write(state,0x74+i*4,target[i]);
        (void)parameters.SetAnglesAndDistance(state,angles,distance,1);
    }
    float MinimumDistance() override{return config.near_clip;}
    float MaximumDistance() override{return config.far_clip;}
    std::uint32_t UpdateMatrix(pal4::CameraDataState& data,std::uint32_t mode) override{return matrix.Update(data,mode).value_or(0);}
    void InitializeNested(pal4::CameraDataState&,std::uint32_t) override{}
    void DestroyNested(pal4::CameraDataState&,std::uint32_t) override{}
    void DestroyCamera(pal4::CameraDataState&) override{}
    std::uint32_t NullFunctionSentinel() override{return 0;}
    void Rotate(pal4::CameraDataState& data,std::uint32_t offset,const Vector& axis,float degrees,std::uint32_t mode) override {
        if(mode!=0 && mode!=2)throw std::runtime_error("unsupported game camera matrix combine");
        const float angle=degrees*0.017453292519943295f,c=std::cos(angle),s=std::sin(angle);
        // RwMatrix stores transformed basis vectors; POSTCONCAT rotates each
        // vector in world coordinates. The axes passed by PAL4 are unit axes.
        const auto rotate=[&](Vector v){
            const Vector cross{axis[1]*v[2]-axis[2]*v[1],axis[2]*v[0]-axis[0]*v[2],axis[0]*v[1]-axis[1]*v[0]};
            const auto d=Dot(axis,v);for(unsigned n=0;n<3;++n)v[n]=v[n]*c+cross[n]*s+axis[n]*d*(1-c);return v;
        };
        for(unsigned row=0;row<4;++row) {
            Vector source=ReadVector(data,offset+row*16);
            if(mode==0)source=row==0?Vector{1,0,0}:row==1?Vector{0,1,0}:row==2?Vector{0,0,1}:Vector{};
            const auto value=rotate(source);for(unsigned n=0;n<3;++n)Write(data,offset+row*16+n*4,value[n]);
        }
    }
    std::uint32_t CameraFrame(std::uint32_t) override{return 1;}
    void SetFrameIdentity(std::uint32_t) override{}
    void ApplyTransform(std::uint32_t,pal4::CameraDataState&,std::uint32_t,std::uint32_t) override{}
    void UpdateFrameObjects(std::uint32_t) override{}
    std::uint32_t CalculateBoundingBox(pal4::CameraDataState&,std::uint32_t,const pal4::CameraBoundsVertices& vertices,
        const pal4::CameraBoundsMatrix&,std::uint32_t) override {
        minimum=maximum=vertices[0];for(const auto& v:vertices)for(unsigned n=0;n<3;++n){minimum[n]=std::min(minimum[n],v[n]);maximum[n]=std::max(maximum[n],v[n]);}return 0;
    }
    std::int32_t Random() override{throw std::runtime_error("original camera shake RNG is not bound");}
    std::uint32_t EnvironmentFrame(std::uint32_t) override{return world?1:0;}
    bool ProbeAuxiliary(std::uint32_t,const pal4::CameraSegment&,float&,pal4::CameraPoint&) override {
        throw std::runtime_error("dynamic camera obstacle callback is not bound");
    }
    bool Intersect(std::uint32_t,const pal4::CameraSegment& segment,pal4::CameraCollision& hit) override {
        const Vector start{segment[0],segment[1],segment[2]},end{segment[3],segment[4],segment[5]};
        const Vector direction{end[0]-start[0],end[1]-start[1],end[2]-start[2]};
        const auto length=Dot(direction,direction);if(!world || length==0)return false;
        bool found=false;hit.fraction=1;
        for(std::size_t i=0;i+2<world->vertices.size();i+=3) {
            std::array<Vector,3> triangle{};for(unsigned p=0;p<3;++p)std::copy_n(world->vertices[i+p].position,3,triangle[p].begin());
            Vector point{};if(!pal4::CollisionTriangleContainment::IntersectLineWithTriangle(start,direction,triangle,&point))continue;
            const Vector relative{point[0]-start[0],point[1]-start[1],point[2]-start[2]};const auto fraction=Dot(relative,direction)/length;
            if(fraction>=0 && fraction<=hit.fraction){hit={fraction,point};found=true;}
        }
        return found;
    }
    float Magnitude(const Vector& v) override{return std::sqrt(Dot(v,v));}
    bool Intersects(pal4::CameraDataState& data,std::uint32_t,std::uint32_t point,std::uint32_t) override {
        const auto p=ReadVector(data,point);for(unsigned n=0;n<3;++n)if(p[n]<minimum[n] || p[n]>maximum[n])return false;return true;
    }
};
}
struct GameCameraSystem::Impl final:pal4::ICameraSystemInitializerRuntime,
    pal4::ICameraModeConfigurationRuntime,pal4::ICameraControllerAccessRuntime,pal4::ICameraParameterRuntime {
    GameCameraConfig config;std::array<std::unique_ptr<Camera>,4> cameras;bool tracks_bound{};
    pal4::CameraControllerState controller;std::array<pal4::CameraSystemObjectState,4> objects;unsigned allocated{};
    static constexpr unsigned address=0x8000;
    static unsigned ObjectIndex(unsigned object){if(object<0x1000 || object>=0x1400 || (object-0x1000)%0x100)throw std::logic_error("invalid camera object handle");return (object-0x1000)/0x100;}
    static unsigned CameraIndex(unsigned camera){if(camera<0x2000 || camera>=0x2400 || (camera-0x2000)%0x100)throw std::logic_error("invalid camera data handle");return (camera-0x2000)/0x100;}
    void LogInitialization(bool) override{}
    pal4::CameraRenderRect RenderRect() override{return {0,0,800,600};}
    unsigned AllocateObject(unsigned bytes) override{if(bytes!=32 || allocated>=4)throw std::logic_error("invalid camera allocation");return 0x1000+allocated++*0x100;}
    void ConstructObject(unsigned object) override{objects.at(ObjectIndex(object))={};}
    bool SetupObject(unsigned object,const pal4::CameraSetupParameters& parameters) override{
        const auto i=ObjectIndex(object);objects[i].camera=0x2000+i*0x100;
        cameras[i]->Set(parameters.target,{parameters.angle_y,parameters.angle_x,parameters.angle_z},parameters.distance);return true;
    }
    pal4::CameraDataState& CameraDataForObject(unsigned object) override{return cameras.at(ObjectIndex(object))->state;}
    pal4::CameraSystemObjectState& ObjectAt(unsigned object) override{return objects.at(ObjectIndex(object));}
    pal4::CameraDataState& CameraDataAt(unsigned camera) override{return cameras.at(CameraIndex(camera))->state;}
    void ClearObjectCollection(unsigned object,unsigned offset) override{if(offset!=16)throw std::logic_error("invalid camera collection");ObjectAt(object).tasks={};}
    void SetFarClip(unsigned,float) override{throw std::logic_error("per-camera far clip is not bound");}
    std::optional<unsigned> FindNamedObject(const pal4::CameraControllerState&,unsigned,const char*) override{return {};}
    void ReportMissingCameraName(const char*) override{throw std::logic_error("camera name is not bound");}
    float MinimumDistance() override{return config.near_clip;}
    float MaximumDistance() override{return config.far_clip;}
    unsigned UpdateMatrix(pal4::CameraDataState& state,unsigned mode) override{
        for(auto& camera:cameras)if(&camera->state==&state){camera->anchor=mode;return camera->matrix.Update(state,mode).value_or(0);}
        throw std::logic_error("camera matrix owner is missing");
    }
};
GameCameraSystem::GameCameraSystem(GameCameraConfig config):impl_(std::make_unique<Impl>()) {
    if(!std::isfinite(config.near_clip) || !std::isfinite(config.far_clip) || !std::isfinite(config.view_scale) || config.near_clip<=0 || config.far_clip<=config.near_clip || config.view_scale<=0)
        throw std::invalid_argument("invalid game camera projection");
    impl_->config=config;for(auto& camera:impl_->cameras)camera=std::make_unique<Camera>(config);
    pal4::CameraSystemInitializer initializer(*impl_);if(!initializer.Initialize(impl_->controller,Impl::address))throw std::runtime_error("camera initialization failed");
    // 原初始化写的是x86回调地址；此CPU适配器仍只绑定静态world探测，不能将其当宿主函数。
    impl_->cameras[0]->state.At(0x2bc)=0;
}
GameCameraSystem::~GameCameraSystem()=default;
void GameCameraSystem::SetCollision(bool enabled){for(unsigned i=0;i<2;++i)impl_->cameras[i]->state.At(0x1a8)=enabled;}
void GameCameraSystem::SetParameters(std::size_t index,Vector target,Vector angles,float distance){impl_->cameras.at(index)->Set(target,angles,distance);}
void GameCameraSystem::SetTarget(std::size_t index,Vector target){Finite(target);auto& c=*impl_->cameras.at(index);c.anchor=1;(void)c.parameters.SetLookAt(c.state,target,1);}
bool GameCameraSystem::SetMode(unsigned index){auto& s=*impl_;pal4::CameraControllerAccess access(s);pal4::CameraParameterSetters parameters(s);pal4::CameraModeConfiguration modes(parameters,access,s);return modes.SetModeByIndex(s.controller,Impl::address,index);}
unsigned GameCameraSystem::Mode() const{return impl_->controller.At(0xc0);}
void GameCameraSystem::SetYaw(float yaw){if(!std::isfinite(yaw))throw std::invalid_argument("invalid camera yaw");auto& c=*impl_->cameras[0];c.anchor=1;(void)c.parameters.SetFirstAngle(c.state,yaw,1);}
bool GameCameraSystem::TracksBound() const{return impl_->tracks_bound;}
std::uint32_t GameCameraSystem::TaskWord(std::size_t index,std::uint32_t offset) const {
    if(offset<0x3c || offset>0x88 || offset%4)throw std::invalid_argument("invalid task camera word");
    return impl_->cameras.at(index)->state.At(offset);
}
void GameCameraSystem::TrackVector(std::size_t index,bool eye,Vector value,std::uint32_t mode) {
    Finite(value);if(mode>1)throw std::invalid_argument("invalid track camera anchor");
    auto& c=*impl_->cameras.at(index);c.anchor=mode;impl_->tracks_bound=true;
    const auto result=eye?c.parameters.SetEye(c.state,value,mode):c.parameters.SetLookAt(c.state,value,mode);
    if(!result)throw std::runtime_error("camera track vector failed");
}
void GameCameraSystem::TrackScalar(std::size_t index,unsigned parameter,float value,std::uint32_t mode) {
    if(!std::isfinite(value) || mode>1 || parameter>3)throw std::invalid_argument("invalid track camera scalar");
    auto& c=*impl_->cameras.at(index);c.anchor=mode;impl_->tracks_bound=true;
    if(parameter==0)(void)c.parameters.SetFirstAngle(c.state,value,mode);
    else if(parameter==1)(void)c.parameters.SetSecondAngle(c.state,value,mode);
    else if(parameter==2)(void)c.parameters.SetThirdAngle(c.state,value,mode);
    else if(!c.parameters.SetDistance(c.state,value,mode))throw std::runtime_error("camera track distance failed");
}
void GameCameraSystem::Update(float elapsed,const Scene& world) {
    if(!std::isfinite(elapsed) || elapsed<0)throw std::invalid_argument("invalid game camera frame time");
    // MainCamera is the active camera after original startup. Task-driven
    // selection of ScriptCamera/CombatCamera is not bound yet.
    auto& c=impl_->cameras[0];c->world=&world;(void)c->animation.Update(c->state,elapsed);(void)c->matrix.Update(c->state,c->anchor);c->world=nullptr;
}
GameCameraPose GameCameraSystem::Pose(std::size_t index) const {
    const auto& s=impl_->cameras.at(index)->state;
    return {ReadVector(s,0x68),ReadVector(s,0x74),ReadVector(s,0x5c),ReadVector(s,0x80),ReadVector(s,0x50),ReadVector(s,0x3c),Read(s,0x4c),Read(s,0x48),s.At(0x1a8)!=0};
}
std::array<float,16> GameCameraSystem::Matrix(float aspect,std::size_t index) const {
    if(!std::isfinite(aspect) || aspect<=0)throw std::invalid_argument("invalid camera aspect");
    const auto p=Pose(index);const auto config=impl_->config;
    const std::array<float,16> view{p.right[0],p.up[0],p.forward[0],0,p.right[1],p.up[1],p.forward[1],0,p.right[2],p.up[2],p.forward[2],0,-Dot(p.right,p.eye),-Dot(p.up,p.eye),-Dot(p.forward,p.eye),1};
    const auto x=aspect>1?config.view_scale:config.view_scale*aspect,y=aspect>1?config.view_scale/aspect:config.view_scale;
    const auto n=config.near_clip,f=config.far_clip;
    // RwCamera公开viewMatrix把frame right映射到屏幕左侧；SDK探针：+X投影0.4，中心0.5。
    // OpenGL保留原世界/相机姿态，仅在投影中转换X方向及深度[-1,1]。
    const std::array<float,16> projection{-1/x,0,0,0,0,1/y,0,0,0,0,(f+n)/(f-n),1,0,0,-2*f*n/(f-n),0};
    std::array<float,16> result{};for(unsigned col=0;col<4;++col)for(unsigned row=0;row<4;++row)for(unsigned k=0;k<4;++k)result[col*4+row]+=projection[k*4+row]*view[col*4+k];return result;
}
void WriteGameCameraReport(std::ostream& output,const GameCameraSystem& cameras) {
    output<<"{\"original_matrix_bound\":true,\"static_world_collision_bound\":true,\"dynamic_collision_bound\":false,\"camera_tracks_bound\":"<<(cameras.TracksBound()?"true":"false")<<",\"mode\":"<<cameras.Mode()<<",\"cameras\":[";
    for(unsigned i=0;i<4;++i){if(i)output<<',';const auto p=cameras.Pose(i);output<<"{\"collision\":"<<(p.collision?"true":"false")<<",\"distance\":"<<p.distance<<",\"eye\":["<<p.eye[0]<<','<<p.eye[1]<<','<<p.eye[2]<<"],\"target\":["<<p.target[0]<<','<<p.target[1]<<','<<p.target[2]<<"]}";}
    output<<"]}";if(!output)throw std::runtime_error("cannot write camera report");
}
} // namespace pal4::portable
