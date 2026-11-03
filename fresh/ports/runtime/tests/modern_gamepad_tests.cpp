#include "pal4/portable/modern_gamepad.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#ifdef PAL4_GAMEPAD_REFERENCE
#include "pal4inject/gamepad.h"
#endif
using namespace pal4::portable;
namespace {
void Check(bool value){if(!value)throw std::runtime_error("modern gamepad contract failed");}
bool Near(float a,float b){return std::fabs(a-b)<.00003f;}
}
int main(){try{
    const ControlCameraBasis camera{{1,0,0},{0,0,1}};
    Check(!PlanModernGamepad(.1f,.1f,camera,0,.05f).active);
    Check(ModernGamepadCameraDelta(.1f,.1f,.05f)==std::array<float,2>{});
    Check(Near(ModernGamepadCameraDelta(1,0,.05f)[0],-6));
    const auto forward=PlanModernGamepad(0,1,camera,0,.05f);
    Check(forward.active && forward.mode==2 && Near(forward.speed_multiplier,1.5f) && Near(forward.animation_multiplier,1.4f));
    const auto right=PlanModernGamepad(1,0,camera,270,.05f);
    Check(Near(right.direction[0],-1) && Near(right.direction[2],0));
    const auto reverse=PlanModernGamepad(0,-1,camera,0,.05f);
    Check(reverse.walk_turn && reverse.mode==0 && Near(reverse.speed_multiplier,.4f));
    Check(Near(std::atan2(reverse.direction[0],reverse.direction[2])*57.29577951308232f,-18));
    const ControlCameraBasis pitched{{1,0,0},{0,-.8f,.6f}};
    Check(Near(PlanModernGamepad(0,1,pitched,0,.05f).direction[2],1));
    bool bad=false;try{(void)PlanModernGamepad(std::numeric_limits<float>::quiet_NaN(),0,camera,0,.05f);}catch(const std::invalid_argument&){bad=true;}Check(bad);
#ifdef PAL4_GAMEPAD_REFERENCE
    unsigned samples{};
    for(int x:{-32768,-25000,-12000,-7849,0,7849,12000,25000,32767})
    for(int y:{-32768,-25000,-12000,-7849,0,7849,12000,25000,32767})
    for(float yaw:{-180.f,0.f,45.f,90.f,179.f,270.f,359.f})
    for(float dt:{0.f,1.f/60,.05f,.2f}){
        const auto stick=pal4::inject::BuildGamepadAnalogStick(x,y,7849);
        const auto camera_stick=pal4::inject::BuildGamepadAnalogStick(x,y,8689);
        const auto camera_delta=ModernGamepadCameraDelta(x/32767.f,y/32767.f,dt);
        Check(Near(camera_delta[0],-camera_stick.x*120.f*std::min(dt,.1f)) && Near(camera_delta[1],-camera_stick.y*120.f*std::min(dt,.1f)));
        const auto actual=PlanModernGamepad(x/32767.f,y/32767.f,camera,yaw,dt);
        Check(actual.active==(stick.magnitude>0));
        if(!actual.active)continue;
        const auto turn=pal4::inject::BuildGamepadTurnTuning(yaw,-stick.x,stick.y,dt,360,20);
        auto tuning=pal4::inject::BuildGamepadMovementTuning(stick.magnitude,.62f,.88f);
        if(turn.use_walk_animation)tuning={0,.4f,1};
        Check(Near(actual.magnitude,stick.magnitude) && Near(actual.direction[0],turn.direction_x) && Near(actual.direction[2],turn.direction_z));
        Check(actual.mode==static_cast<unsigned>(tuning.mode) && Near(actual.speed_multiplier,tuning.speed_multiplier) && Near(actual.animation_multiplier,tuning.animation_multiplier));
        ++samples;
    }
    std::cout<<samples<<" input/turn/speed samples match actual pal4_inject common source\n";
#endif
    std::cout<<"modern deadzone, camera direction, turn bound and tuning contracts passed; compiled="<<ModernGamepadEnabled()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
