#pragma once
#include <cstdint>

namespace pal4 {
// Shared value operations; State does not need an x86 object layout or texture.
template<class State>
void ConfigureScreenFade(State& state,float duration,std::uint32_t hold,
    std::uint8_t red,std::uint8_t green,std::uint8_t blue,
    float start,float end,float finish) {
    state.elapsed=0;state.duration=duration;
    state.red=red;state.green=green;state.blue=blue;
    state.alpha_start=start;state.alpha_end=end;state.alpha_on_finish=finish;
    state.phase=0;state.keep_active=hold;state.active=1;
}
template<class State,class Interpolate>
void AdvanceScreenFade(State& state,float delta,Interpolate interpolate) {
    if(state.active==0 || state.phase!=0)return;
    state.elapsed+=delta;
    if(state.elapsed>=state.duration) {
        state.elapsed=state.duration;state.alpha=state.alpha_on_finish;state.phase=1;
        if(state.keep_active==0)state.active=0;
    }
    state.alpha=interpolate(state.alpha_start,state.alpha_end,state.elapsed/state.duration);
}
} // namespace pal4
