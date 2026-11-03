#include "pal4/portable/modern_gamepad.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace pal4::portable {
bool ModernGamepadEnabled() noexcept {return PAL4_MODERN_GAMEPAD!=0;}
std::array<float,2> ModernGamepadCameraDelta(float x,float y,float elapsed) {
    if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(elapsed)||elapsed<0)
        throw std::invalid_argument("invalid modern camera stick input");
    x=std::clamp(x,-1.f,1.f);y=std::clamp(y,-1.f,1.f);
    const float length=std::sqrt(x*x+y*y),raw=std::min(length,1.f);
    constexpr float deadzone=8689.f/32767.f;
    if(raw<=deadzone)return {};
    const float scale=(raw-deadzone)/(1-deadzone)/length;
    const float step=120.f*std::clamp(elapsed,0.f,.1f);
    return {-x*scale*step,-y*scale*step};
}
ModernGamepadPlan PlanModernGamepad(float x,float y,const ControlCameraBasis& camera,float yaw,float elapsed) {
    if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(yaw)||!std::isfinite(elapsed)||elapsed<0)
        throw std::invalid_argument("invalid modern gamepad input");
    for(const auto& basis:{camera.right,camera.forward})for(float value:basis)
        if(!std::isfinite(value))throw std::invalid_argument("invalid modern gamepad camera basis");
    x=std::clamp(x,-1.f,1.f);y=std::clamp(y,-1.f,1.f);
    const float length=std::sqrt(x*x+y*y),raw=std::min(length,1.f);
    constexpr float deadzone=7849.f/32767.f;
    ModernGamepadPlan plan;
    if(raw<=deadzone)return plan;
    plan.magnitude=std::clamp((raw-deadzone)/(1-deadzone),0.f,1.f);
    x=x/length*plan.magnitude;y=y/length*plan.magnitude;
    float dx=camera.forward[0]*y-camera.right[0]*x,dz=camera.forward[2]*y-camera.right[2]*x;
    const float size=std::sqrt(dx*dx+dz*dz);if(size<=.0001f)return plan;
    dx/=size;dz/=size;
    const float target=std::atan2(dx,dz)*57.29577951308232f;
    const float delta=std::fmod(target-yaw+540.f,360.f)-180.f;
    const float turn=360.f*std::clamp(elapsed,0.f,.1f);
    const float angle=(yaw+std::clamp(delta,-turn,turn))*.017453292519943295f;
    plan.direction={std::sin(angle),0,std::cos(angle)};plan.active=true;
    if(plan.magnitude>=.88f){plan.mode=2;plan.speed_multiplier=1.5f;plan.animation_multiplier=1.4f;}
    else if(plan.magnitude<.62f){plan.mode=0;plan.speed_multiplier=.4f;}
    else {const float blend=std::clamp((plan.magnitude-.62f)/(.88f-.62f),0.f,1.f);
        plan.mode=1;plan.speed_multiplier=1+.5f*blend;plan.animation_multiplier=1+.4f*blend;}
    plan.walk_turn=std::fabs(delta)>20.f;
    if(plan.walk_turn){plan.mode=0;plan.speed_multiplier=.4f;plan.animation_multiplier=1;}
    return plan;
}
}
