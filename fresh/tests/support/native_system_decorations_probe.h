#pragma once
#include "pal4/native_system_menu_decorations.h"
#include "pal4/time_formatter.h"
#include "../../native/ui/cegui_exports_native.h"
#include <array>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeSystemDecorationText(NativeSystemMenuDecorations& decorations,
    std::uint32_t inventory,std::uint32_t game,const std::function<void()>& tick){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto word=[](std::uint32_t a)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(a);};
    struct Restore{std::array<std::uint32_t*,3> fields;std::array<std::uint32_t,3> values;
        ~Restore(){for(unsigned i=0;i<3u;++i)*fields[i]=values[i];}
    } restore{{&word(inventory+308u),&word(game),&word(game+4u)},
              {word(inventory+308u),word(game),word(game+4u)}};
    auto module=GetModuleHandleA("CEGUIBase.dll");
    const auto get=detail::Export<const void* (__thiscall*)(const void*)>(module,"?getText@Window@CEGUI@@QBEABVString@2@XZ");
    const auto utf8=detail::Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ");
    const auto text=[&](unsigned offset){return std::string(utf8(get(reinterpret_cast<void*>(word(decorations.InfoFrame()+offset)))));};
    word(inventory+308u)=123456u;word(game)=65u;word(game+4u)=3600u;
    // Refresh the real show handler, which obtains live inventory/captions and clock.
    const auto root=word(decorations.InfoFrame()+36u);
    const auto parent=word(root+68u);
    check(parent!=0u,"GameInfo has its actual System parent");
    check(decorations.HandleEvent(0x8b13a0u,1u,parent),"GameInfo show reaches original shared handler");
    check(text(44u)=="123456" && text(52u)==TimeFormatter::FormatSeconds(3665) &&
        text(60u)==TimeFormatter::FormatSeconds(65),"original DLL GameInfo reads live money, total and session clock");
    tick();
    check(word(game)==66u,"original GameTimer callback increments actual session counter once");
    check(text(52u)==TimeFormatter::FormatSeconds(3666) && text(60u)==TimeFormatter::FormatSeconds(66),
        "GameInfo clock refresh observes live game counter");
}
}
