#pragma once
#include "pal4/native_miles_sound_3d_runtime.h"
#include <windows.h>
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace pal4::test {
// Caller supplies an opened real digital driver/listener and mutes the process.
inline void VerifyNativeMiles3dSound(const std::filesystem::path& root,
    NativeMilesDigitalDriver& driver, NativeMiles3dListener& listener) {
    const auto check=[](bool ok,const char* text){if(!ok)throw std::runtime_error(text);};
    const auto close=[](float a,float b){return std::abs(a-b)<0.0001f;};
    NativeMilesSound3dRuntime sounds(root,driver,listener);
    const auto id=sounds.PlayNamedSound("WF036.wav",{4.f,-7.f,2.5f},1u,600.f,300.f);
    auto state=sounds.Inspect(id);
    check(state.status==4u && state.loop_count==1u,"real Miles mono3D started once");
    check(close(state.miles_position[0],4.f)&&close(state.miles_position[1],-7.f)&&
          close(state.miles_position[2],-2.5f),"real Miles3D original Z conversion");
    check(close(state.distances[0],600.f)&&close(state.distances[1],300.f),"real Miles3D max/min600/300");
    check(sounds.SetPosition(id,{1.f,2.f,-3.f}),"real Miles3D reposition");
    sounds.SetVolume(.25f);state=sounds.Inspect(id);
    check(close(state.miles_position[0],1.f)&&close(state.miles_position[1],2.f)&&
          close(state.miles_position[2],3.f)&&close(state.volume,.25f),"real Miles3D position/volume readback");
    check(sounds.Stop(id)&&!sounds.Stop(id)&&sounds.ActiveCount()==0u,"real Miles3D cancellation owns release");
    check(!sounds.SetPosition(id,{0.f,0.f,0.f}),"retired3D voice cannot reposition");
    bool missing=false;
    try{(void)sounds.PlayNamedSound("PAL4_missing_3d.wav",{},1u,600.f,300.f);}
    catch(const std::runtime_error&){missing=true;}
    check(missing&&sounds.ActiveCount()==0u,"missing3D WAV does not retain voice");
    for(unsigned n=0;n<2u;++n){
        const auto loop=sounds.PlayNamedSound(n?"gamedata\\palSound\\WF025.wav":"gamedata\\PALSound\\WF025.wav",{},0u,600.f,300.f);
        std::this_thread::sleep_for(std::chrono::milliseconds(150));sounds.Update();
        check(sounds.ActiveCount()==1u&&sounds.Inspect(loop).loop_count==0u,"real Miles3D indefinite loop survives update");
        check(sounds.Exists(loop)&&sounds.IsPlaying(loop),"loop handle active before suspension");
        check(sounds.Suspend(loop),"suspend loop through real Miles");
        sounds.Update();
        check(sounds.Exists(loop)&&!sounds.IsPlaying(loop),"suspended voice retained by update");
        check(sounds.Resume(loop)&&sounds.IsPlaying(loop),"resume retained real Miles loop");
        sounds.StopAll();check(sounds.ActiveCount()==0u,"repeated3D owner cleanup");
    }
    (void)sounds.PlayNamedSound("WF025.wav",{},1u,600.f,300.f);
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
    while(sounds.ActiveCount()&&std::chrono::steady_clock::now()<deadline){
        std::this_thread::sleep_for(std::chrono::milliseconds(10));sounds.Update();
    }
    check(sounds.ActiveCount()==0u,"real Miles3D completed sample automatically retires");
    sounds.StopAll();
    std::cout<<"native_miles_3d_sound_probe: real mono WAV start/readback/loop/cancel/expiry/cleanup passed\n";
}
}
