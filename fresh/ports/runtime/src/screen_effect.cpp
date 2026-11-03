#include "pal4/portable/screen_effect.h"
#include "pal4/screen_fade_state.h"
#include "pal4/float_interpolator.h"
#include <cmath>
#include <stdexcept>
#include <utility>

namespace pal4::portable {
ScriptResponse ScreenEffectServices::Invoke(const ScriptRequest& request) {
    const auto& call=request.call;const auto id=call.system_id;
    if(id<291 || id>296)return next_.Invoke(request);
    if(state_.phase==0)return {};
    const bool in=id==292 || id==294 || id==296;
    const auto count=in && id!=296?2u:3u;
    if(call.arguments.size()!=count)throw std::invalid_argument("invalid flash arguments");
    const float duration=std::get<float>(call.arguments[0]);
    if(!std::isfinite(duration))throw std::invalid_argument("nonfinite flash duration");
    const bool hold=in?false:std::get<bool>(call.arguments[1]);
    const bool wait=std::get<bool>(call.arguments.back());
    const std::uint8_t red=id>=295?229:id>=293?255:0;
    const std::uint8_t other=id>=295?10:id>=293?255:0;
    pal4::ConfigureScreenFade(state_,duration,hold?1u:0u,red,other,other,
        in?255.f:0.f,in?0.f:255.f,in?0.f:255.f);
    // As in the existing native adapter: avoid division by zero, but a
    // blocking command still yields until the next service/frame update.
    if(duration<=0) {
        state_.alpha=state_.alpha_on_finish;state_.phase=1;
        if(!hold)state_.active=0;
    }
    if(wait)pending_.push_back(request.token);
    return {{},wait};
}
std::vector<ScriptToken> ScreenEffectServices::Advance(float elapsed) {
    if(!std::isfinite(elapsed) || elapsed<0)throw std::invalid_argument("invalid flash frame time");
    struct Math:pal4::IFloatInterpolationRuntime {
        double CosineApproximation(float) override {throw std::logic_error("fade requires linear interpolation");}
    } math;
    pal4::AdvanceScreenFade(state_,elapsed,[&](float start,float end,float fraction) {
        return static_cast<float>(pal4::FloatInterpolator::Interpolate(start,end,fraction,0,math));
    });
    if(state_.phase==0)return {};
    return std::exchange(pending_,{});
}
void ScreenEffectServices::Reset() {state_=ScreenFadeState{};pending_.clear();}
} // namespace pal4::portable
