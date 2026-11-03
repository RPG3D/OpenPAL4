#pragma once
#include "pal4/native_system_setting_session.h"
#include "pal4/native_system_setting_frame.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/native_cegui_window_pointer_events.h"
#include "pal4/native_dialog_settings_snapshot.h"
#include "pal4/native_background_music_runtime.h"
#include "pal4/native_miles_sound_2d_runtime.h"
#include "pal4/native_effect_scene_session.h"
#include "pal4/native_event_handler_array.h"
#include "../../native/ui/cegui_exports_native.h"
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
inline void VerifyPersistentSettingPage(NativeSystemSettingSession& session,NativeCeguiUiSession& ui,
    NativeDialogSettingsSnapshot& settings,NativeBackgroundMusicRuntime& music,
    NativeMilesSound2dRuntime* script_sound,NativeMilesSound2dRuntime& hud_sound,
    NativeEffectSceneSession& effects,NativeEventHandlerArray& active,
    const std::function<void(std::uint32_t)>& key,const std::function<void()>& render){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto word=[](std::uint32_t address){return *reinterpret_cast<const std::uint32_t*>(address);};
    const auto module=GetModuleHandleA("CEGUIBase.dll");const auto frame=session.Frame().FrameAddress();
    const auto top=[&]{return active.GetLast();};
    check(top()==session.DataInput(),"persistent F7 opens original default data page");
    key(VK_ESCAPE);check(top()==session.MainInput(),"persistent data Escape restores main settings input");
    const std::array saved_audio{settings.AudioLevel(0),settings.AudioLevel(1),settings.AudioLevel(2)};
    const auto saved_clip=settings.State().Word(12u);
    key('3');check(top()==session.OptionsInput(),"persistent numeric3 opens actual options input");
    const auto slider_value=detail::Export<float (__thiscall*)(const void*)>(module,"?getCurrentValue@Slider@CEGUI@@QBEMXZ");
    const auto set_slider=detail::Export<void (__thiscall*)(void*,float)>(module,"?setCurrentValue@Slider@CEGUI@@QAEXM@Z");
    const auto bgm=word(frame+496u),sfx=word(frame+500u);
    check(std::fabs(slider_value(reinterpret_cast<const void*>(bgm))-(1.0f-saved_audio[0]))<0.0001f,
        "persistent BGM slider initializes from actual live settings");
    key(slider_value(reinterpret_cast<const void*>(bgm))<0.5f?VK_LEFT:VK_RIGHT);
    check(std::fabs(settings.AudioLevel(0)-saved_audio[0])>0.01f &&
        std::fabs(music.Volume()-settings.AudioLevel(0))<0.0001f,
        "persistent slider keyboard updates settings and actual music owner");
    key(VK_DOWN);key(slider_value(reinterpret_cast<const void*>(sfx))<0.5f?VK_LEFT:VK_RIGHT);
    check(std::fabs(settings.AudioLevel(1)-saved_audio[1])>0.01f &&
        (!script_sound || std::fabs(script_sound->Volume()-settings.AudioLevel(1))<0.0001f) &&
        std::fabs(hud_sound.Volume()-settings.AudioLevel(1))<0.0001f,
        "persistent SFX keyboard reaches both actual sample owners");
    NativeCeguiWindowPointerEvents pointer(module);
    (void)pointer.HandleMouseAtCenter(word(frame+(saved_clip?212u:216u)));session.RethrowError();
    const auto expected=saved_clip?1800.0f:1200.0f;
    check(settings.State().Word(12u)==(saved_clip?0u:1u) && effects.EffectDistanceSquared()==expected*expected,
        "persistent clip DLL click reaches real rendering distance service");
    render();
    (void)pointer.HandleMouseAtCenter(word(frame+(saved_clip?216u:212u)));
    set_slider(reinterpret_cast<void*>(bgm),1.0f-saved_audio[0]);
    set_slider(reinterpret_cast<void*>(sfx),1.0f-saved_audio[1]);session.RethrowError();
    for(unsigned i=0;i<3;++i)settings.SetAudioLevel(i,saved_audio[i]);
    music.SetVolume(saved_audio[0]);if(script_sound)script_sound->SetVolume(saved_audio[1]);hud_sound.SetVolume(saved_audio[1]);
    key(VK_ESCAPE);check(top()==session.MainInput(),"persistent options Escape restores main input");
    const auto modal=[&]{const auto root=reinterpret_cast<const void*>(session.Dialog().RootAddress());
        return detail::Export<void* (__thiscall*)(const void*)>(module,"?getParent@Window@CEGUI@@QBEPAV12@XZ")(root) &&
            detail::Export<bool (__thiscall*)(const void*)>(module,"?getModalState@Window@CEGUI@@QBE_NXZ")(root);};
    const auto cancel=[&]{(void)pointer.HandleMouseAtCenter(session.Dialog().Window("BtNo"));session.RethrowError();
        check(!modal() && session.Dialog().LastSelection()!=1u,"persistent real No closes confirmation without game action");};
    key('2');check(top()==session.DataInput(),"persistent numeric2 opens real load list");
    key(VK_DOWN);key(VK_RETURN);check(modal(),"persistent load list Enter opens actual common confirmation");
    cancel();key(VK_ESCAPE);check(top()==session.MainInput(),"persistent cancelled load restores main input");
    key('4');check(modal(),"persistent numeric4 opens actual quit confirmation");cancel();
    check(top()==session.MainInput(),"persistent cancelled quit retains settings activity");
    ui.RethrowSystemSettingMainClickError();
    std::cout<<"Persistent settings: real data/options, BGM/SFX owners="<<(script_sound?2:1)<<", clip render, load/quit modal cancellation and input unwind pass\n";
}
}
