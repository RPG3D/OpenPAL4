#pragma once
#include "pal4/pal_actor_control.h"
namespace pal4 {
// 原423FF0共享顺序；Runtime仅提供当前相机/角色和数学边界。
template<class Runtime>void FlushActorCameraTailYaw(PalActorControlState& state,Runtime& runtime){
    if(state.words[1]==0)return;
    runtime.InitializeCameraSubsystem();const auto camera=runtime.GetSceneCamera();if(camera==0)return;
    const auto internal=runtime.GetCameraInternal(camera);if(internal==0)return;
    float sign{};const auto correction=runtime.ComputeYawCorrection(state,internal,state.words[1],sign);
    const auto yaw=runtime.CurrentCameraYaw(internal);if(correction==0)return;
    if(sign<0)runtime.SetCameraYaw(internal,yaw+correction,1);
    else if(sign>0)runtime.SetCameraYaw(internal,yaw-correction,1);
}
}
