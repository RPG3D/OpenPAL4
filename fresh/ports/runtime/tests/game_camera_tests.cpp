#include "pal4/portable/game_camera.h"
#include "pal4/portable/camera_follow.h"
#include "pal4/portable/game_services.h"
#include <sstream>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <source_location>
#include <string>
using namespace pal4::portable;
namespace {
void Check(bool value,std::source_location where=std::source_location::current()){if(!value)throw std::runtime_error("game camera contract failed at line "+std::to_string(where.line()));}
bool Near(float a,float b){return std::abs(a-b)<0.002f;}
template<class F> void Reject(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}Check(failed);}
std::array<float,3> Project(const std::array<float,16>& m,std::array<float,3> p){std::array<float,4> out{};for(unsigned row=0;row<4;++row)out[row]=m[row]*p[0]+m[4+row]*p[1]+m[8+row]*p[2]+m[12+row];return {out[0]/out[3],out[1]/out[3],out[2]/out[3]};}
Scene Wall(){Scene s;for(auto p:{std::array<float,3>{-100,-100,-50},{100,-100,-50},{100,100,-50},{-100,-100,-50},{100,100,-50},{-100,100,-50}}){Vertex v{};for(unsigned i=0;i<3;++i)v.position[i]=p[i];s.vertices.push_back(v);}return s;}
}
int main(){try {
    const GameCameraConfig config;
    GameCameraSystem cameras(config);Scene empty;
    cameras.SetParameters(0,{0,0,0},{0,0,0},100);
    auto pose=cameras.Pose();Check(Near(pose.eye[2],-100) && Near(pose.forward[2],1) && Near(pose.right[0],1));
    auto m=cameras.Matrix(1);auto center=Project(m,{0,0,0});Check(Near(center[0],0) && Near(center[1],0));
    // 原RwCamera公开viewMatrix测量：世界+X在屏幕左侧，+Y在上方。
    Check(Project(m,{10,0,0})[0]<0 && Project(m,{-10,0,0})[0]>0 && Project(m,{0,10,0})[1]>0);
    Check(Near(Project(m,{0,0,pose.eye[2]+config.near_clip})[2],-1) &&
          Near(Project(m,{0,0,pose.eye[2]+config.far_clip})[2],1));
    cameras.SetParameters(0,{0,0,0},{90,0,0},100);pose=cameras.Pose();Check(Near(pose.eye[0],-100) && Near(pose.forward[0],1));
    cameras.SetParameters(0,{0,0,0},{0,90,0},100);pose=cameras.Pose();Check(Near(pose.eye[1],100) && Near(pose.forward[1],-1));
    cameras.SetParameters(0,{0,0,0},{0,0,0},100);cameras.Update(0,Wall());
    pose=cameras.Pose();const auto probeDistance=std::sqrt(50.f*50+14.96f*14.96f);
    Check(Near(pose.distance,probeDistance) && Near(pose.eye[2],-probeDistance));
    cameras.SetCollision(false);Check(!cameras.Pose(0).collision && !cameras.Pose(1).collision && cameras.Pose(2).collision);
    cameras.SetParameters(0,{0,0,0},{0,0,0},100);cameras.Update(1,Wall());Check(Near(cameras.Pose().distance,100));
    cameras.SetCollision(true);cameras.Update(0,Wall());Check(cameras.Pose().distance<100);
    cameras.SetTarget(0,{300,0,0});cameras.Update(1,empty);Check(Near(cameras.Pose().distance,100));
    const auto before=cameras.Matrix(16.f/9);cameras.Update(0,empty);Check(cameras.Matrix(16.f/9)==before);
    Reject([&]{cameras.Update(-1,empty);});Reject([&]{cameras.Matrix(0);});
    Reject([&]{cameras.SetTarget(0,{std::numeric_limits<float>::infinity(),0,0});});
    for(float yaw:{0.f,90.f,174.f})for(float aspect:{1.f,16.f/9,.75f}){
        cameras.SetParameters(0,{10,20,30},{yaw,26,0},100);const auto p=cameras.Pose();const auto matrix=cameras.Matrix(aspect);
        auto right=p.target,left=p.target,up=p.target;for(unsigned axis=0;axis<3;++axis){right[axis]+=10*p.right[axis];left[axis]-=10*p.right[axis];up[axis]+=10*p.up[axis];}
        Check(Project(matrix,right)[0]<0 && Project(matrix,left)[0]>0 && Project(matrix,up)[1]>0);
    }
    std::optional<CameraFollowTarget> actor=CameraFollowTarget{{10,20,30},{0,174,0},10,false};
    CameraFollow follow(cameras,[&]{return actor;},[&](bool enabled){actor->auto_seek=enabled;});
    follow.SetMode(0,true);Check(cameras.Mode()==0 && actor->auto_seek && Near(cameras.Pose().angles[0],174));
    Check(Near(cameras.Pose().target[0],10) && Near(cameras.Pose().target[1],38) && Near(cameras.Pose().target[2],30));
    follow.SeekToPlayer();const auto listener=follow.Listener();
    Check(listener.updates==1 && Near(listener.position[0],10) && Near(listener.position[1],38) && Near(listener.position[2],-30));
    Check(Near(listener.forward[0],cameras.Pose().forward[0]) && Near(listener.forward[2],-cameras.Pose().forward[2]) && listener.up[1]==1 && std::signbit(listener.up[2]));
    actor->position={50,60,70};follow.Advance();Check(Near(cameras.Pose().target[0],50) && Near(cameras.Pose().target[1],78));
    follow.SetMode(1,false);Check(cameras.Mode()==1 && !actor->auto_seek && cameras.Pose().target==std::array<float,3>{});
    const auto fixed=cameras.Pose().target;actor->position={90,100,110};follow.Advance();Check(cameras.Pose().target==fixed);
    follow.SetMode(2,true);Check(cameras.Mode()==2 && actor->auto_seek && Near(cameras.Pose().target[1],118));
    for(float yaw:{0.f,90.f,174.f,180.f,225.f,359.f}){actor->rotation[1]=yaw;follow.SetMode(0,false);Check(Near(std::remainder(cameras.Pose().angles[0]-yaw,360.f),0));}
    for(unsigned mode=0;mode<6;++mode){follow.SetMode(mode,false);Check(cameras.Mode()==mode);}
    Check(cameras.SetMode(6)==false && cameras.Mode()==5);follow.SetMode(7,false);Check(cameras.Mode()==5);
    // 服务91独立调用：不重新应用模式，不移动目标或改玩家自动跟随。
    std::ostringstream trace;GameServices services(trace);unsigned flush_calls{};
    Reject([&]{services.Invoke({{1,1},ScriptCall{91,0,{}}});});
    services.flush_camera_tail_yaw=[&]{follow.FlushTailYaw();++flush_calls;};
    Reject([&]{services.Invoke({{1,1},ScriptCall{91,0,{std::int32_t(1)}}});});
    for(float yaw:{0.f,90.f,174.f,225.f,359.f}) {
        actor->rotation[1]=yaw;actor->auto_seek=false;
        cameras.SetParameters(0,{100,200,300},{45,26,7},123);
        const auto before_flush=cameras.Pose();const auto mode_before=cameras.Mode();
        ScriptGlobals globals{};ScriptStack script(globals);
        script.Compile("flush","void Probe(){giFlushTailYAngle();global_mvar=10300;}");
        Check(script.Push("flush","void Probe()",services).has_value());
        Check(script.Tick(0)==ScriptState::finished && globals[0]==10300);
        const auto after_flush=cameras.Pose();
        Check(Near(std::remainder(after_flush.angles[0]-yaw,360.f),0));
        Check(after_flush.target==before_flush.target && after_flush.angles[1]==26 && after_flush.angles[2]==7);
        Check(after_flush.requested_distance==123 && cameras.Mode()==mode_before && !actor->auto_seek);
        const auto stable=cameras.Matrix(1);const auto stable_yaw=cameras.Pose().angles[0];follow.FlushTailYaw();const auto again=cameras.Matrix(1);float max_error{};for(unsigned n=0;n<16;++n)max_error=std::max(max_error,std::abs(again[n]-stable[n]));Check(Near(cameras.Pose().angles[0],stable_yaw) && max_error<0.0002f);
    }
    Check(flush_calls==5);
    actor.reset();const auto without_actor=cameras.Matrix(1);follow.FlushTailYaw();Check(cameras.Matrix(1)==without_actor);
    const auto previous=cameras.Pose().target;follow.SeekToPlayer();Check(cameras.Pose().target==previous);
    std::cout<<"original camera matrix, projection, static collision, recovery and collision flags passed\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
