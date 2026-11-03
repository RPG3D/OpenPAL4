#include "native_combat_protect_hint_probe.h"
#include "pal4/native_combat_hints.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_game_timer_queue.h"
#include "pal4/native_game_database.h"
#include "pal4/native_cegui_text_conversion.h"
#include "../../native/ui/cegui_exports_native.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/native_combat_sound.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/hint_window.h"
#include "pal4/native_miles_sound_2d_runtime.h"
#include <bit>
#include <cstdio>
#include <array>
#include <limits>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
void VerifyNativeCombatAcquisitionHints(NativeCombatHints& hints,NativeCeguiUiSession& ui,NativeGameTimerQueue& timers,
    const NativeGameDatabase& database,const NativeInventorySession& inventory,NativeMilesSound2dRuntime& sounds){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto word=[](unsigned a){return *reinterpret_cast<const unsigned*>(a);};
    const auto module=GetModuleHandleW(L"CEGUIBase.dll");
    const auto get_text=detail::Export<const void* (__thiscall*)(const void*)>(module,"?getText@Window@CEGUI@@QBEABVString@2@XZ");
    const auto cstr=detail::Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ");
    const auto shown=[&]{return std::string(cstr(get_text(reinterpret_cast<void*>(word(ui.HintFrameAddress()+40u)))));};
    const auto record=inventory.PropertyAddress(3001u),name=record?word(record+20u):0u;
    check(name!=0u,"acquisition fixture actual Property name");
    const auto format=database.FindTableTextById("GameString",251u);check(format.has_value(),"actual acquired-property GameString251");
    NativeCombatSystemImage image{};unsigned acquisitions{},plays{},sample{};
    const auto acquire=[&]{++acquisitions;return reinterpret_cast<unsigned>(&image);};
    const auto play=[&](std::string_view resource,unsigned mode){check(resource=="WA06.WAV" && mode==1u,"property acquisition sound resource/mode");++plays;sample=PlayNativeCombatSound(image,sounds,resource,mode);check(sample!=0u,"actual WA06 acquisition sound");};
    timers.Update(std::numeric_limits<float>::min());const auto baseline=timers.EventCount();
    for(int count:{0,-2,3}){
        hints.ShowCombatText("prior-stunt",{.5f,.15f},2.f);
        const std::array<unsigned,2> event{3001u,std::bit_cast<unsigned>(count)};
        hints.ShowPickedProperty(reinterpret_cast<unsigned>(event.data()),inventory,database,acquire,play);
        char expected[1024]{};const std::string f(*format);
        const auto n=std::snprintf(expected,sizeof(expected),f.c_str(),reinterpret_cast<const char*>(name),count);
        check(n>=0 && n<1024,"original acquisition resource formats in fixture bound");
        const auto utf8=ConvertGameTextToUtf8(expected);
        check(utf8 && shown()==*utf8 && ui.HintVisible() && !word(hints.Root(2u)+68u),"property hint uses actual DLL text and replaces CombatHint2");
        check(acquisitions==(count>0?1u:0u) && plays==(count>0?1u:0u),"only positive acquired count plays sound/acquires Combat");
        timers.Update(std::numeric_limits<float>::min());check(timers.EventCount()==baseline+1u,"property hint owns one ordinary timer");
        timers.Update(2.001f);timers.Update(std::numeric_limits<float>::min());check(!ui.HintVisible() && timers.EventCount()==baseline,"property hint naturally closes and retires timer");
    }
    hints.ShowCombatText("prior-stunt",{.5f,.15f},2.f);hints.ShowPickUpFailed(database);
    const auto failed=database.FindTableTextById("GameString",268u);const auto utf8=failed?ConvertGameTextToUtf8(*failed):std::nullopt;
    check(utf8 && shown()==*utf8 && ui.HintVisible() && !word(hints.Root(2u)+68u) && plays==1u,"pickup failure uses original268 text and clears skill hint without sound");
    timers.Update(std::numeric_limits<float>::min());check(timers.EventCount()==baseline+1u,"pickup failure owns one ordinary timer");
    timers.Update(2.001f);timers.Update(std::numeric_limits<float>::min());check(!ui.HintVisible() && timers.EventCount()==baseline,"pickup failure timer cleanup");
    if(sample)(void)sounds.StopSample(sample);
    std::cout<<"Native acquisition hints: actual Property3001/GameString251/268, zero/negative/positive counts, DLL text, skill-hint replacement, timer retirement and positive-only WA06 pass\n";
}
void VerifyNativeCombatMoneyHints(NativeCombatHints& hints,NativeCeguiUiSession& ui,NativeGameTimerQueue& timers,const NativeGameDatabase& database,NativeMilesSound2dRuntime& sounds){
    const auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    const auto word=[](std::uint32_t a){return *reinterpret_cast<const std::uint32_t*>(a);};
    const auto module=GetModuleHandleW(L"CEGUIBase.dll");
    const auto get_text=detail::Export<const void* (__thiscall*)(const void*)>(module,"?getText@Window@CEGUI@@QBEABVString@2@XZ");
    const auto cstr=detail::Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ");
    NativeCombatSystemImage image{};unsigned acquisitions{},plays{},sample{};const auto baseline=timers.EventCount();
    const auto acquire=[&]{++acquisitions;return reinterpret_cast<unsigned>(&image);};
    const auto play=[&](std::string_view name,unsigned mode){check(name=="WA06.WAV" && mode==1u,"money sound preserves original resource/mode");++plays;sample=PlayNativeCombatSound(image,sounds,name,mode);check(sample!=0u,"money hint plays real original sound");};
    for(int amount:{0,-9999,13}){
        hints.ShowMoney(reinterpret_cast<unsigned>(&amount),database,acquire,play);
        if(!amount){check(!ui.HintVisible() && !acquisitions && !plays && timers.EventCount()==baseline,"zero money creates no hint/clock/sound");continue;}
        check(ui.HintVisible() && (!hints.Root(2u) || !word(hints.Root(2u)+68u)),"money uses original HintWindow and clears CombatHint2");
        const auto text=database.FindTableTextById("GameString",amount>0?0xc2u:0x10bu);check(text.has_value(),"actual money GameString exists");
        char expected[1024]{};const std::string format(*text);const auto n=std::snprintf(expected,sizeof(expected),format.c_str(),amount>0?amount:-amount);check(n>=0 && n<1024,"money resource formats within fixture bound");
        const auto utf8=ConvertGameTextToUtf8(expected);check(utf8.has_value() && std::string(cstr(get_text(reinterpret_cast<void*>(word(ui.HintFrameAddress()+40u)))))==*utf8,"money original DLL text preserves full signed integer magnitude");
        check(acquisitions==(amount>0?1u:0u) && plays==(amount>0?1u:0u),"only positive money acquires combat and plays sound");
        timers.Update(std::numeric_limits<float>::min());check(timers.EventCount()==baseline+1u,"money owns one original timed hint");
        timers.Update(2.001f);timers.Update(std::numeric_limits<float>::min());check(!ui.HintVisible() && timers.EventCount()==baseline,"money timer naturally closes and retires");
    }
    if(sample)(void)sounds.StopSample(sample);
    std::cout<<"Native money hint probe: zero/-9999/+13, actual GameString194/267 and DLL text, full DWORD magnitude, timed cleanup, positive-only acquisition/real WA06 sound pass\n";
}
void VerifyNativeCombatMonsterFleeHints(NativeCombatHints& hints,NativeCeguiUiSession& ui,NativeGameTimerQueue& timers,const NativeGameDatabase& database){
    const auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    const auto word=[](std::uint32_t a){return *reinterpret_cast<const std::uint32_t*>(a);};
    const auto module=GetModuleHandleW(L"CEGUIBase.dll");
    const auto get_text=detail::Export<const void* (__thiscall*)(const void*)>(module,"?getText@Window@CEGUI@@QBEABVString@2@XZ");
    const auto cstr=detail::Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ");
    std::array<std::uint32_t,202> actor{};const auto address=reinterpret_cast<std::uint32_t>(actor.data());
    // ArenaReady now owns a real scene-name hint. Retire only this shared
    // window's prior event before measuring unrelated fixture timers.
    ui.CloseOriginalHint();
    (void)timers.RemoveEvent(HintWindow::kClockName);
    timers.Update(std::numeric_limits<float>::min());const auto base=timers.EventCount();
    hints.ShowMonsterFlee(address,false,database);
    check(!ui.HintVisible() && timers.EventCount()==base && hints.Count()==0u,"Player Flee gate does not create UI or timers");
    actor[201]=2u;
    for(bool failed:{false,true}){
        check(ui.ShowOriginalHint("prior-scene",.5f,.15f,1.f,0u,1u),
              "Monster Flee fixture starts with an actual timed scene hint");
        hints.ShowCombatText("prior-skill",{0.5f,0.15f},2.f);
        hints.ShowMonsterFlee(address,failed,database);
        check(!word(hints.Root(2u)+68u) && ui.HintVisible(),"Monster Flee clears CombatHint2 and shows original HintWindow");
        const auto text=database.FindTableTextById("GameString",failed?0x10du:0x103u);
        check(text.has_value(),"actual Monster Flee GameString exists");
        const auto expected=ConvertGameTextToUtf8(*text);check(expected.has_value(),"Monster Flee game text converts");
        check(std::string(cstr(get_text(reinterpret_cast<void*>(word(ui.HintFrameAddress()+40u)))))==*expected,"actual Monster Flee DLL text matches original GameString");
        timers.Update(std::numeric_limits<float>::min());
        check(timers.EventCount()==base+1u,"Monster Flee replaces skill timer with one HintWindow timer");
        timers.Update(2.001f);timers.Update(std::numeric_limits<float>::min());
        check(!ui.HintVisible() && timers.EventCount()==base,"Monster Flee hint closes and retires its timer");
    }
    std::cout<<"Native Monster Flee hint probe: actor fixture, Player early gate, actual GameString259/269, original HintWindow DLL text, skill-hint cleanup and timers pass\n";
}
void VerifyNativeCombatProtectHint(NativeCombatHints& hints,NativeGameTimerQueue& timers,const NativeGameDatabase& database){
    const auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    const auto word=[](std::uint32_t a){return *reinterpret_cast<const std::uint32_t*>(a);};
    const auto module=GetModuleHandleW(L"CEGUIBase.dll");
    const auto visible=detail::Export<bool (__thiscall*)(const void*,bool)>(module,"?isVisible@Window@CEGUI@@QBE_N_N@Z");
    const auto get_text=detail::Export<const void* (__thiscall*)(const void*)>(module,"?getText@Window@CEGUI@@QBEABVString@2@XZ");
    const auto cstr=detail::Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ");
    const auto original=database.FindTableTextById("GameString",313u);check(original.has_value(),"actual Protect GameString313");
    const auto expected=ConvertGameTextToUtf8(*original);check(expected.has_value(),"Protect text encoding");
    timers.Update(std::numeric_limits<float>::min());const auto base=timers.EventCount(),repeating=timers.Count();
    hints.ShowProtect(database);const auto owner=hints.Ensure(2u),root=hints.Root(2u);
    check(root && word(root+68u) && visible(reinterpret_cast<void*>(root),false) && timers.EventCount()==base+1u && timers.Count()==repeating,
          "Protect displays actual second DLL hint with one original two-second event");
    check(std::string(cstr(get_text(reinterpret_cast<void*>(word(owner+40u)))))==*expected,
          "actual Protect DLL text matches original GameString313");
    check(hints.Clock(2u)=="CombatHint2Clock" && word(owner+64u)==0u && word(owner+68u)==0u,"Protect clock/flags retain original contract");
    timers.Update(2.f);check(word(root+68u)!=0u,"Protect lifetime retains original strict threshold");
    timers.Update(.001f);timers.Update(std::numeric_limits<float>::min());
    check(!word(root+68u) && timers.EventCount()==base && timers.Count()==repeating,
          "actual Protect close callback drains only its one-shot timer");
    std::cout<<"Native Protect hint probe: actual GameString313/DLL text, second hint root, flags0/0, two-second clock/strict threshold and callback cleanup pass\n";
}
}
