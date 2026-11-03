#pragma once
#include "pal4/native_angelscript_engine.h"
#include "pal4/native_game_script_callbacks.h"
#include "pal4/native_script_frame_runner.h"
#include "pal4/native_script_talk_runtime.h"
#include "pal4/native_script_random_talk_pool.h"
#include "pal4/native_cegui_ui_session.h"
#include <stdexcept>
#include <iostream>

namespace pal4::test {
inline void ProbeNativeRandomTalk(NativeCeguiUiSession& ui) {
    const auto check = [](bool value, const char* message) {
        if (!value) throw std::runtime_error(message);
    };
    NativeAngelScriptEngine engine;
    NativeScriptFrameRunner runner(engine);
    NativeScriptRandomTalkPool pool;
    NativeScriptTalkRuntime talk(ui,runner.Context(),[] { return std::uint8_t{0}; });
    NativeGameScriptServices services{};
    services.random_talk = &pool;
    services.talk = &talk;
    NativeGameScriptCallbacks callbacks(services);
    engine.SetCallbacks(&callbacks);
    constexpr char source[] =
        "void Begin() { string text = \"Random dialogue candidate owned by PAL4\"; "
        "giRandTalkPush(text); text = \"changed source\"; giRandTalk(); giRandTalkRelease(); } "
        "void Empty() { giRandTalk(); }";
    check(engine.Raw()->AddScriptSection("randomtalk", "randomtalk.cs", source,sizeof(source)-1) >= 0 &&
          engine.Raw()->Build("randomtalk") >= 0,"compile random dialogue SDK fixture");
    runner.Queue("randomtalk","Begin");
    check(runner.Tick(0.0f) == NativeScriptFrameStatus::suspended &&
          runner.Error().empty() && pool.Size()==1u &&
          pool.Text(0)=="Random dialogue candidate owned by PAL4" && talk.Waiting() && ui.ScenarioDialogVisible(),
          "random dialogue callback suspends actual SDK context and shows original DLL layout");
    const auto capacity = pool.Capacity();
    check(ui.HideScenarioDialog() && talk.Waiting(),
          "random dialogue wait closes through existing native scenario handler");
    check(runner.Tick(0.0f,&talk)==NativeScriptFrameStatus::finished && !talk.Waiting() &&
          pool.Size()==0u && pool.Capacity()==capacity,
          "resumed random dialogue release clears candidates and retains storage capacity");
    runner.Queue("randomtalk","Empty");
    check(runner.Tick(0.0f)==NativeScriptFrameStatus::failed &&
          runner.Error().find("empty pool")!=std::string::npos,
          "empty random dialogue remains an explicit failure after wait registration");
    talk.ClearHandlers();
    pool.Clear();
    engine.SetCallbacks(nullptr);
    std::cout << "Native random dialogue: SDK callbacks, owned candidates, original DLL display, wait/resume/release and empty-pool failure verified\n";
}
} // namespace pal4::test
