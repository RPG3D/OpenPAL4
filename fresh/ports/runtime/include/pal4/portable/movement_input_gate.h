#pragma once
#include <cmath>
namespace pal4::portable {
// 切场时若方向仍被按住，必须释放后才能再次移动。
class MovementInputGate {
public:
    void Arm(float x,float y){if(std::abs(x)>.25f || std::abs(y)>.25f)waiting_=true;}
    void Apply(float x,float y,float& output_x,float& output_y){
        if(waiting_){if(std::abs(x)<=.25f && std::abs(y)<=.25f)waiting_=false;output_x=output_y=0;return;}
        output_x=x;output_y=y;
    }
    void Reset(){waiting_=false;}
    bool Waiting() const{return waiting_;}
private:
    bool waiting_{};
};
}
