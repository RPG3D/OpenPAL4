#pragma once
#include "pal4/native_script_hint_display.h"
#include "pal4/native_game_script_callbacks.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/native_game_timer_queue.h"
#include "pal4/native_cegui_text_conversion.h"
#include "pal4/native_arena_ready_hints.h"
#include "pal4/native_game_database.h"
#include "../../native/ui/cegui_exports_native.h"
#include <cstring>
#include <limits>
#include <stdexcept>

namespace pal4::testing {
inline void VerifyNativeScriptHints(NativeCeguiUiSession& ui, NativeGameTimerQueue& timers) {
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    check(timers.EventCount()==0u,"script hint fixture starts without unrelated events");
    NativeScriptHintDisplay hints(ui);
    NativeGameScriptServices services; services.hint_display=&hints;
    NativeGameScriptCallbacks callbacks(services);
    NativeAngelScriptEngine engine;engine.SetCallbacks(&callbacks);
    const std::string source="void Empty(){giShowHint(\"\",0.5,0.15);} "
        "void Long(){giShowHint(\"<dc0>0123456789012345678901234567890123456789\xb2\xe2\xca\xd4</dc0>\",0.5,0.15);} ";
    check(engine.Raw()->AddScriptSection("hint","hint.cs",source.data(),static_cast<int>(source.size()))>=0 &&
        engine.Raw()->Build("hint")>=0,"actual SDK hint script compilation");
    asIScriptContext* context{};check(engine.Raw()->CreateContext(&context)>=0 && context,"hint SDK context");
    struct Release {asIScriptContext* p;~Release(){(void)p->Release();}} release{context};
    const auto module=GetModuleHandleW(L"CEGUIBase.dll");
    const auto get_text=detail::Export<const void* (__thiscall*)(const void*)>(module,"?getText@Window@CEGUI@@QBEABVString@2@XZ");
    const auto length=detail::Export<unsigned (__thiscall*)(const void*)>(module,"?length@String@CEGUI@@QBEIXZ");
    const auto ptr=detail::Export<const char32_t* (__thiscall*)(const void*)>(module,"?ptr@String@CEGUI@@ABEPBIXZ");
    const auto run=[&](const char* name,std::string_view expected){
        check(context->Prepare(engine.FunctionId("hint",name))>=0 && context->Execute()==asEXECUTION_FINISHED,"SDK hint executes");
        const auto frame=ui.HintFrameAddress();
        const auto edit=*reinterpret_cast<const std::uint32_t*>(frame+40u);
        const auto* actual=get_text(reinterpret_cast<const void*>(edit));
        const auto utf8=ConvertGameTextToUtf8(expected);check(bool(utf8),"expected hint conversion");
        const detail::NativeString text(module,*utf8,detail::NativeStringEncoding::Utf8);
        const auto size=length(text.Data());
        check(length(actual)==size && std::memcmp(ptr(actual),ptr(text.Data()),size*4u)==0,
            "actual HintWindow text matches installed CEGUI codec including markup and long GBK input");
        // Original queue retires cancelled nodes on the next nonzero update.
        timers.Update(std::numeric_limits<float>::min());
        check(ui.HintVisible() && !*reinterpret_cast<const std::uint8_t*>(frame+64u) && timers.EventCount()==1u,
            "script hint flag0 visible and exactly one live close event");
    };
    run("Empty","");
    run("Long","<dc0>0123456789012345678901234567890123456789\xb2\xe2\xca\xd4</dc0>");
    timers.Update(2.49f);check(ui.HintVisible(),"hint remains before fixed 2.5 second deadline");
    timers.Update(0.02f);check(!ui.HintVisible(),"real timer closes script hint after 2.5 seconds");
    timers.Update(0.001f);check(!timers.EventCount(),"script hint timer node retires");
}

inline void VerifyNativeArenaReadyHints(NativeCeguiUiSession& ui,
    NativeGameTimerQueue& timers, const NativeGameDatabase& database) {
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    check(timers.EventCount()==0u,"Scene hint fixture has no unrelated timers");
    NativeArenaReadyHints hints(ui);
    hints.BindDatabase(database);
    check(hints.Records().size()==database.Tables().at("Scene").rows.size(),
          "Scene hint adapter exposes every original database row");
    const ArenaReadyHintRecord* record{};
    for(const auto& row:hints.Records())if(row.arena=="M08" && row.area=="1"){record=&row;break;}
    check(record && !record->text.empty(),"actual M08/1 scene-name record exists");
    const auto markup="<dc0><df2>"+std::string(record->text)+"</df2></dc0><br/>";
    const auto frame=hints.CreateHint();
    check(frame && frame==ui.HintFrameAddress(),"Scene hints borrow the persistent HintWindow");
    hints.Show(frame,markup,0.5f,0.15f,1.f,0u,1u);
    timers.Update(std::numeric_limits<float>::min());
    const auto module=GetModuleHandleW(L"CEGUIBase.dll");
    const auto get_text=detail::Export<const void* (__thiscall*)(const void*)>(module,"?getText@Window@CEGUI@@QBEABVString@2@XZ");
    const auto length=detail::Export<unsigned (__thiscall*)(const void*)>(module,"?length@String@CEGUI@@QBEIXZ");
    const auto ptr=detail::Export<const char32_t* (__thiscall*)(const void*)>(module,"?ptr@String@CEGUI@@ABEPBIXZ");
    const auto utf8=ConvertGameTextToUtf8(markup);check(bool(utf8),"Scene markup uses game text codec");
    const detail::NativeString expected(module,*utf8,detail::NativeStringEncoding::Utf8);
    const auto edit=*reinterpret_cast<const std::uint32_t*>(frame+40u);
    const auto actual=get_text(reinterpret_cast<const void*>(edit));
    check(length(actual)==length(expected.Data()) &&
          std::memcmp(ptr(actual),ptr(expected.Data()),length(actual)*4u)==0,
          "actual Scene HintWindow contains original GBK database text and markup");
    check(ui.HintVisible() && *reinterpret_cast<const std::uint8_t*>(frame+64u)==1u && timers.EventCount()==1u,
          "ArenaReady displays flag1 with exactly one real close timer");
    bool rejected{};try{hints.Show(frame+4u,"wrong owner",0.5f,0.15f,1.f,0u,1u);}
    catch(const std::invalid_argument&){rejected=true;}
    check(rejected && hints.ShownMarkup()==markup,"invalid Scene hint owner cannot replace displayed text");
    timers.Update(.99f);check(ui.HintVisible(),"Scene hint remains before original one-second deadline");
    timers.Update(.02f);check(!ui.HintVisible(),"Scene hint closes through real one-second timer");
    timers.Update(.001f);check(!timers.EventCount(),"Scene hint timer is reclaimed");
}
}
