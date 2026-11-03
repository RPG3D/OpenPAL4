#include "pal4/portable/camera_follow.h"
#include "pal4/portable/world.h"
#include "pal4/script_gameplay_callbacks.h"
#include "pal4/script_camera_callbacks.h"
#include "pal4/base_object_scene_parameter.h"
#include "pal4/pal_actor_camera_tail_yaw.h"
#include "pal4/pal_actor_control_transform.h"
#define HANDMADE_MATH_NO_SSE
#include <HandmadeMath.h>
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <ostream>
#include <stdexcept>
namespace pal4::portable {
namespace {
void Require(bool v,const char* message){if(!v)throw std::logic_error(message);}
HMM_Mat4 Decode(const pal4::ActorControlMatrix& value){auto m=HMM_M4D(1);std::memcpy(m.Elements,value.words.data(),64);return m;}
pal4::ActorControlMatrix Encode(HMM_Mat4 value){pal4::ActorControlMatrix m;std::memcpy(m.words.data(),value.Elements,64);return m;}
}
struct CameraFollow::Impl final:pal4::IScriptCameraModeRuntime,pal4::IScriptCameraCallbackRuntime,
    pal4::IBaseObjectSceneParameterRuntime,pal4::IActorControlTransformRuntime {
    GameCameraSystem& cameras;Target target;std::function<void(bool)> sync;
    std::optional<CameraFollowTarget> actor;CameraListener listener;
    unsigned mode_calls{},seek_calls{},follow_updates{},flushes{};float correction{},axis_y{},yaw_before{},yaw_after{};
    static constexpr unsigned manager=0x1000,object=0x2000,internal=0x3000,player=0x4000,control=0x5000,audio=0x6000;
    Impl(GameCameraSystem& c,Target t,std::function<void(bool)> s):cameras(c),target(std::move(t)),sync(std::move(s)){Require(bool(target)&&bool(sync),"missing camera follow owner");}
    void Snapshot(){actor=target();if(actor){for(float f:actor->position)Require(std::isfinite(f),"invalid camera actor position");for(float f:actor->rotation)Require(std::isfinite(f),"invalid camera actor rotation");Require(std::isfinite(actor->radius)&&actor->radius>=0,"invalid camera actor radius");}}
    unsigned CameraSubsystem() override{return manager;}
    void SetModeByIndex(unsigned owner,unsigned mode) override{Require(owner==manager,"invalid camera manager");(void)cameras.SetMode(mode);}
    unsigned ReadWord(unsigned address) override{if(address==manager+0xc0)return cameras.Mode();if(address==audio+0x1c)return audio+0x100;throw std::logic_error("invalid camera script field");}
    unsigned ActiveCameraInternal(unsigned owner) override{Require(owner==manager,"invalid camera manager");return internal;}
    unsigned ActiveCameraInternalId(unsigned owner) override{return ActiveCameraInternal(owner);}
    unsigned ActiveCameraObject(unsigned owner) override{Require(owner==manager,"invalid camera manager");return object;}
    void SetPosition(unsigned camera,unsigned x,unsigned y,unsigned z,unsigned mode) override{Require(camera==internal&&mode==1,"invalid camera target mode");cameras.SetTarget(0,{std::bit_cast<float>(x),std::bit_cast<float>(y),std::bit_cast<float>(z)});}
    std::array<float,3> ClumpPosition(unsigned clump) override{Require(clump==player&&actor.has_value(),"camera actor disappeared");return actor->position;}
    void SetLookAt(unsigned camera,unsigned mode) override{
        Require(camera==object&&mode==1,"invalid camera look-at");if(!actor)return;
        pal4::BaseObjectState state;state.Write(0x14,player);state.Write(0xec,std::bit_cast<unsigned>(actor->radius));
        cameras.SetTarget(0,pal4::BaseObjectSceneParameter::Refresh(state,*this));++follow_updates;
    }
    unsigned ActorControl() override{return control;}
    unsigned PlayerManager() override{return player;}
    std::int32_t SetPlayerState(unsigned owner,std::uint8_t enabled) override{Require(owner==player,"invalid camera player manager");sync(enabled!=0);return 0;}
    // 原回调AutoSeek走独立玩家拥有者；本适配器只调用SeekToPlayer。
    pal4::PlayerManagerObjectState& PlayerManagerState() override{throw std::logic_error("camera auto-seek state is not exposed");}
    void FlushMainCameraTailYaw(unsigned owner) override{
        Require(owner==control,"invalid camera controller");pal4::PalActorControlState state;state.words[1]=actor?player:0;
        yaw_before=cameras.Pose().angles[0];correction=axis_y=0;pal4::FlushActorCameraTailYaw(state,*this);yaw_after=cameras.Pose().angles[0];++flushes;
    }
    // The callback base now supplies InitializeCameraSubsystem via CameraSubsystem.
    // This adapter is used for seek/follow, not incremental script rotation.
    void AdjustYpr(float,float,float,std::uint8_t) override{throw std::logic_error("camera follow incremental script rotation is not bound");}
    unsigned GetSceneCamera(){return object;}
    unsigned GetCameraInternal(unsigned camera){Require(camera==object,"invalid scene camera");return internal;}
    float ComputeYawCorrection(pal4::PalActorControlState& state,unsigned camera,unsigned source,float& sign){const auto result=pal4::PalActorControlTransform::Compute(state,camera,source,&sign,*this);correction=result;axis_y=sign;return result;}
    float CurrentCameraYaw(unsigned camera){Require(camera==internal,"invalid camera yaw");return cameras.Pose().angles[0];}
    void SetCameraYaw(unsigned camera,float yaw,unsigned mode){Require(camera==internal&&mode==1,"invalid camera yaw mode");cameras.SetYaw(yaw);}
    pal4::ActorControlMatrix CameraMatrix(unsigned camera) override{Require(camera==internal,"invalid camera matrix source");return Encode(HMM_Rotate_RH(HMM_AngleDeg(cameras.Pose().angles[0]),HMM_V3(0,1,0)));}
    pal4::ActorControlMatrix ActorTransform(unsigned source) override{Require(source==player&&actor.has_value(),"invalid actor transform source");return std::bit_cast<pal4::ActorControlMatrix>(InstanceMatrix(actor->position,actor->rotation));}
    void SetupTransformMatrix(pal4::ActorControlMatrix& out,const pal4::ActorControlMatrix& source) override{out=Encode(HMM_InvGeneralM4(Decode(source)));}
    void NormalizeBasis(pal4::ActorControlMatrix& value,unsigned offset) override{
        Require(offset==0||offset==16||offset==32,"invalid actor basis");auto m=Decode(value);const auto n=HMM_NormV3(m.Columns[offset/16].XYZ);m.Columns[offset/16].XYZ=n;value=Encode(m);
    }
    pal4::ActorControlMatrix Multiply(const pal4::ActorControlMatrix& camera,const pal4::ActorControlMatrix& inverse_actor) override{return Encode(HMM_MulM4(Decode(inverse_actor),Decode(camera)));}
    pal4::ActorControlRotation QueryRotation(const pal4::ActorControlMatrix& value) override{
        auto q=HMM_NormQ(HMM_M4ToQ_RH(Decode(value)));if(q.W<0)q=HMM_MulQF(q,-1);
        const auto length=HMM_LenV3(q.XYZ);if(length<1e-7f)return {};
        const auto axis=HMM_DivV3F(q.XYZ,length);const auto angle=2*std::atan2(length,q.W)*57.29577951308232f;
        return {{axis.X,axis.Y,axis.Z},angle};
    }
    pal4::ActorControlVector3 CameraVector(unsigned camera,unsigned offset) override{Require(camera==internal&&(offset==80||offset==116),"invalid camera transform vector");const auto p=cameras.Pose();const auto& v=offset==80?p.forward:p.target;return {v[0],v[1],v[2]};}
    unsigned CameraByName(unsigned owner,std::string_view name) override{Require(owner==manager,"invalid camera manager");if(name=="MainCamera")return object;throw std::logic_error("non-main camera callback is not bound");}
    unsigned CameraInternalId(unsigned camera) override{Require(camera==object,"invalid camera object");return internal;}
    void WriteWord(unsigned,unsigned) override{throw std::logic_error("camera callback write is not exposed");}
    float ReadFloat(unsigned address) override{Require(address>=internal+0x3c&&address<=internal+0x88&&address%4==0,"invalid camera listener field");return std::bit_cast<float>(cameras.TaskWord(0,address-internal));}
    unsigned AudioManager() override{return audio;}
    void SetListenerPosition(unsigned handle,float x,float y,float z) override{Require(handle==audio+0x100,"invalid listener handle");listener.position={x,y,z};}
    unsigned SetListenerOrientation(unsigned handle,float x,float y,float z,float ux,float uy,float uz) override{Require(handle==audio+0x100,"invalid listener handle");listener.forward={x,y,z};listener.up={ux,uy,uz};++listener.updates;return 1;}
};
CameraFollow::CameraFollow(GameCameraSystem& c,Target target,std::function<void(bool)> sync):impl_(std::make_unique<Impl>(c,std::move(target),std::move(sync))){}
CameraFollow::~CameraFollow()=default;
void CameraFollow::SetMode(unsigned mode,bool sync){auto& s=*impl_;s.Snapshot();++s.mode_calls;(void)pal4::ScriptCameraModeHandler::SetMode(mode,sync,s);}
void CameraFollow::SeekToPlayer(){auto& s=*impl_;s.Snapshot();++s.seek_calls;(void)pal4::ScriptCameraCallbacks::SeekToPlayer(s);}
void CameraFollow::FlushTailYaw(){auto& s=*impl_;s.Snapshot();s.FlushMainCameraTailYaw(Impl::control);}
void CameraFollow::FollowPlayer(){auto& s=*impl_;s.Snapshot();s.SetLookAt(Impl::object,1);}
void CameraFollow::Advance(){auto& s=*impl_;s.Snapshot();if(s.actor&&s.actor->auto_seek)s.SetLookAt(Impl::object,1);}
const CameraListener& CameraFollow::Listener()const{return impl_->listener;}
void CameraFollow::WriteReport(std::ostream& out) const{
    const auto& s=*impl_;out<<std::setprecision(9)<<"{\"mode_calls\":"<<s.mode_calls<<",\"seek_calls\":"<<s.seek_calls<<",\"mode\":"<<s.cameras.Mode()<<",\"follow_updates\":"<<s.follow_updates<<",\"flushes\":"<<s.flushes<<",\"correction\":"<<s.correction<<",\"axis_y\":"<<s.axis_y<<",\"yaw_before\":"<<s.yaw_before<<",\"yaw_after\":"<<s.yaw_after<<",\"listener_updates\":"<<s.listener.updates<<",\"spatial_output_bound\":false";
    const auto vector=[&](const char* name,const auto& v){out<<",\""<<name<<"\":["<<v[0]<<','<<v[1]<<','<<v[2]<<']';};
    if(s.actor){vector("actor_position",s.actor->position);vector("actor_rotation",s.actor->rotation);out<<",\"actor_radius\":"<<s.actor->radius;}
    vector("listener_position",s.listener.position);vector("listener_forward",s.listener.forward);vector("listener_up",s.listener.up);out<<'}';
}
}
