#pragma once
#include <cctype>
#include <cstdint>
#include <string_view>
namespace pal4 {
struct ActorControlIdleState {float elapsed{};std::uint32_t action{};};
inline bool ActorControlAnimationEquals(std::string_view a,std::string_view b) {
    if(a.size()!=b.size())return false;
    for(std::size_t i=0;i<a.size();++i)
        if(std::tolower(static_cast<unsigned char>(a[i]))!=std::tolower(static_cast<unsigned char>(b[i])))return false;
    return true;
}
// 原424750尾部；平台提供当前动画与动作服务，不拥有剧情输入。
template<class Runtime>
void UpdateActorControlIdle(ActorControlIdleState& state,float delta,bool busy,Runtime& runtime) {
    if(busy){state={};runtime.ReleaseDialog();return;}
    if(state.action){if(runtime.Completed())state={};return;}
    if(!ActorControlAnimationEquals(runtime.AnimationName(),"C01")) {
        (void)runtime.SetAnimation("C01");runtime.SetRestart();
    }
    state.elapsed+=delta;
    if(state.elapsed>=6.f){
        if(!ActorControlAnimationEquals(runtime.AnimationName(),"C12")) {
            state.action=runtime.SetAnimation("C12");runtime.SetRestart();
        }
        state.elapsed=0;
    }
}
}
