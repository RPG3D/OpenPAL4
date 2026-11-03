#include "native_combat_levelup_hint_probe.h"
#include "pal4/native_combat_ending_window.h"
#include "pal4/native_combat_hints.h"
#include "pal4/native_game_timer_queue.h"
#include "pal4/native_game_database.h"
#include "pal4/native_cegui_text_conversion.h"
#include <cstdio>
#include "../../native/ui/cegui_exports_native.h"
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
namespace pal4::test {
void VerifyNativeCombatLevelUpHints(NativeCombatEndingWindow& frame,NativeCombatHints& hints,NativeGameTimerQueue& timers,const NativeGameDatabase& db){
    const auto check=[](bool b,const char* m){if(!b)throw std::runtime_error(m);};
    const auto reject=[&](auto f){bool b{};try{f();}catch(const std::logic_error&){b=true;}check(b,"Combat hint rejects invalid identity/operands");};
    const auto module=GetModuleHandleW(L"CEGUIBase.dll");
    const auto visible=detail::Export<bool (__thiscall*)(const void*,bool)>(module,"?isVisible@Window@CEGUI@@QBE_N_N@Z");
    const auto text=detail::Export<const void* (__thiscall*)(const void*)>(module,"?getText@Window@CEGUI@@QBEABVString@2@XZ");
    const auto cstr=detail::Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ");
    const auto x=detail::Export<float (__thiscall*)(const void*)>(module,"?getXPosition@Window@CEGUI@@QBEMXZ");
    const auto y=detail::Export<float (__thiscall*)(const void*)>(module,"?getYPosition@Window@CEGUI@@QBEMXZ");
    const auto width=detail::Export<float (__thiscall*)(const void*)>(module,"?getWidth@Window@CEGUI@@QBEMXZ");
    const auto height=detail::Export<float (__thiscall*)(const void*)>(module,"?getHeight@Window@CEGUI@@QBEMXZ");
    const auto word=[](std::uint32_t a){return *reinterpret_cast<const std::uint32_t*>(a);};
    timers.Update(std::numeric_limits<float>::min());const auto base=timers.EventCount(),repeating=timers.Count();
    std::array<std::uint32_t,4> handles{};for(unsigned i=0;i<4u;++i)handles[i]=frame.Word(684u+4u*i);
    struct Cleanup{NativeCombatHints& h;std::array<std::uint32_t,4>& handles;~Cleanup(){for(auto a:handles)try{h.Cleanup(a);}catch(...){}}} cleanup{hints,handles};
    // Original DialogLayoutBase::IsVisible tests parent attachment, not the retained DLL flag.
    const auto shown=[&](unsigned i){return word(hints.LevelUpRoot(i)+68u)!=0u;};
    const auto centered=[&](unsigned i,const std::array<float,2>& p){const auto root=reinterpret_cast<void*>(hints.LevelUpRoot(i));return std::fabs(x(root)+width(root)*.5f-p[0])<.0001f && std::fabs(y(root)+height(root)*.5f-p[1])<.0001f;};
    const auto base_notice=db.FindTableTextById("GameString",202u),learn_notice=db.FindTableTextById("GameString",184u);
    check(base_notice.has_value() && learn_notice.has_value(),"original level and learned notice formats exist");
    const std::string format(*base_notice),learn_format(*learn_notice);std::array<char,4096> buffer{};
    const auto n=std::snprintf(buffer.data(),buffer.size(),format.c_str(),"Tianhe",7);check(n>0 && static_cast<std::size_t>(n)<buffer.size(),"original level notice formats within fixture bound");std::string notice(buffer.data(),n);
    const auto l=std::snprintf(buffer.data(),buffer.size(),learn_format.c_str(),"Stunt");check(l>0 && static_cast<std::size_t>(l)<buffer.size(),"original learned notice formats within fixture bound");notice+="<br/>";notice.append(buffer.data(),l);
    const auto expected=ConvertGameTextToUtf8(notice);check(expected.has_value(),"original level notice encoding");
    std::cout<<"Native Ending hint notice: "<<*expected<<"\n";
    const std::array<float,2> position{.25f,.35f};
    for(unsigned i=0;i<4u;++i){hints.ShowLevelUp(handles[i],notice,position,3.f);
        check(shown(i) && visible(reinterpret_cast<void*>(hints.LevelUpRoot(i)),false) && word(handles[i]+64u)==0u && word(handles[i]+68u)==0u && centered(i,position),"four real hint roots show and center on supplied row point with original flags");
        check(std::string(cstr(text(reinterpret_cast<void*>(word(handles[i]+40u)))))==*expected,"actual DLL hint retains original rich notice text");}
    check(timers.EventCount()==base+4u && timers.Count()==repeating,"four named hint one-shot callbacks preserve unrelated repeating timers");
    const auto edit=word(handles[0]+40u),panel=word(handles[0]+44u);const auto old_width=width(reinterpret_cast<void*>(panel)),old_height=height(reinterpret_cast<void*>(panel));
    const std::array<float,2> moved{.7f,.8f};hints.ShowLevelUp(handles[0],notice,moved,4.f);
    check(centered(0,moved) && std::fabs(width(reinterpret_cast<void*>(panel))-old_width)<.001f && std::fabs(height(reinterpret_cast<void*>(panel))-old_height)<.001f && word(handles[0]+40u)==edit,
        "repeated original hint reflow and recenter retain dimensions and actual edit identity");
    timers.Update(std::numeric_limits<float>::min());check(timers.EventCount()==base+4u,"refresh replaces only the same named hint clock");
    reject([&]{hints.ShowLevelUp(handles[0]+4u,"invalid",position,3.f);});
    reject([&]{hints.ShowLevelUp(handles[0],"invalid",{std::numeric_limits<float>::quiet_NaN(),0.f},3.f);});
    check(!hints.HandleTimer(0u,0x1234u),"unrelated timer method is retained by caller");
    timers.Update(3.f);for(unsigned i=0;i<4u;++i)check(shown(i),"hint lifetime preserves original strict duration threshold");
    timers.Update(.001f);check(shown(0) && !shown(1) && !shown(2) && !shown(3),"real close callbacks detach three expired roots and preserve refreshed hint");
    timers.Update(1.1f);timers.Update(.001f);check(!shown(0) && timers.EventCount()==base && timers.Count()==repeating,"last real close callback and all hint timer nodes retire");
    for(auto a:handles)hints.ShowLevelUp(a,notice,position,10.f);
    frame.SetVisible(false);timers.Update(std::numeric_limits<float>::min());
    for(unsigned i=0;i<4u;++i)check(!shown(i),"actual Ending hide detaches every active level-up hint");
    check(timers.EventCount()==base && timers.Count()==repeating,"actual Ending hide cancels pending hint callbacks before owner retirement");
    std::cout<<"Native Ending level-up hints: four actual DLL roots/text/row-position centers, stable reflow, distinct three-second clocks, refreshed fourth-second lifetime, strict timer threshold, callback cleanup and real Ending hide cancellation pass\n";
}
}
