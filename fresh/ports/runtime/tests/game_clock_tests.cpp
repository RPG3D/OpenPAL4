#include "pal4/portable/game_clock.h"
#include <iostream>
#include <limits>
#include <stdexcept>
using pal4::portable::GameClock;
int main(){try{
    auto check=[](bool ok){if(!ok)throw std::runtime_error("original game clock contract failed");};
    GameClock clock;check(clock.Elapsed()==0 && clock.Offset()==0);
    check(!clock.Advance(0) && !clock.Advance(1)); // Original strict remaining < delta, not <=.
    check(clock.Advance(.25f) && clock.Elapsed()==1);check(!clock.Advance(.75f));
    check(clock.Advance(.125f) && clock.Elapsed()==2);
    clock.Reset();check(clock.Elapsed()==0 && clock.Offset()==0);
    check(clock.Advance(3.5f) && clock.Elapsed()==1); // At most one invocation per frame.
    check(!clock.Advance(0));check(clock.Advance(.125f) && clock.Elapsed()==2);
    for(float delta:{-1.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){
        bool rejected=false;try{clock.Advance(delta);}catch(const std::invalid_argument&){rejected=true;}check(rejected && clock.Elapsed()==2);
    }
    std::cout<<"game timer strict boundary/overrun/reset/invalid input passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
