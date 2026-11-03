#include <bit>
#include <array>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "pal4/native_angelscript_engine.h"
#include "pal4/native_game_script_callbacks.h"
#include "pal4/native_script_frame_runner.h"
#include "pal4/native_script_timer.h"
#include "pal4/native_timed_script_stack.h"
#include "pal4/native_ui_countdown_script.h"
#include "pal4/native_game_timer_queue.h"
#include "pal4/native_ui_countdown.h"
#include "pal4/native_script_timed_wait_runtime.h"
#include "pal4/native_script_flash_runtime.h"
#include "pal4/effect_render_parameters.h"
#include "pal4/pal_screen_effect.h"

namespace {
void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
void Start(pal4::NativeScriptTimer& timer, float delay, std::string text) {
    const std::uint32_t record[]{0u, reinterpret_cast<std::uint32_t>(text.c_str())};
    (void)pal4::ScriptTimedState::Start(std::bit_cast<std::uint32_t>(delay),
                                        reinterpret_cast<std::uint32_t>(record), timer);
}
struct Host final : pal4::INativeScriptTimerHost {
    bool actor{}, busy{}, duplicate{}, fail{};
    unsigned loads{}, pushes{};
    std::string module, function;
    bool ActorControlActive() override { return actor; }
    bool ScriptBusy() override { return busy; }
    bool HasStackModule(std::string_view name) override {
        Check(name == "TimeScript[pulse]", "original synthetic module name");
        return duplicate;
    }
    std::string_view ScriptSource() override { return "Q01.csb"; }
    void LoadTimerModule(std::string_view source, std::string_view name) override {
        Check(source == "Q01.csb", "timer loads current scene source");
        ++loads;
        module = name;
        if (fail) throw std::runtime_error("loader failure");
    }
    void PushTimerModule(std::string_view name, std::string_view entry) override {
        Check(name == module && entry == "pulse", "timer module/function routing");
        ++pushes;
        function = entry;
    }
};
void Policy() {
    pal4::NativeScriptTimer timer;
    Host host;
    timer.Process(100.f, host);
    Check(host.loads == 0, "disabled timer has no effects");
    Start(timer, 0.5f, "pulse");
    Check(timer.Enabled() && timer.Delay() == 0.5f && timer.Callback() == "pulse",
          "timer owns copied callback and original delay bits");
    timer.Process(0.25f, host);
    Check(host.loads == 0 && timer.Elapsed() == 0.25f, "no early timer execution");
    timer.Process(0.25f, host);
    Check(host.pushes == 1 && timer.Elapsed() == 0 && timer.Enabled(),
          "threshold repeats without disabling");
    timer.Process(1.25f, host);
    Check(host.pushes == 2 && timer.Elapsed() == 0,
          "overshoot discarded, one trigger per frame");
    host.busy = true;
    timer.Process(0.5f, host);
    Check(host.pushes == 2 && timer.Elapsed() == 0,
          "busy gate resets elapsed before deferral");
    host.actor = true;
    timer.Process(0.5f, host);
    Check(host.pushes == 3, "actor control allows interruption of busy script");
    host.duplicate = true;
    timer.Process(0.5f, host);
    Check(host.loads == 3, "active same module prevents duplicate loading");
    host.duplicate = false;
    host.fail = true;
    try {
        timer.Process(0.5f, host);
        Check(false, "loader error propagated");
    } catch (const std::runtime_error& e) {
        Check(std::string(e.what()) == "loader failure", "original error retained");
    }
    host.fail = false;
    timer.Process(0.5f, host);
    Check(host.pushes == 4, "timer remains usable after loader failure");
    timer.Reset();
    Check(!timer.Enabled() && timer.Callback().empty() && timer.Delay() == 0 &&
              timer.Elapsed() == 0,
          "terminate clears enabled, text, elapsed and delay");
    try {
        timer.Process(std::numeric_limits<float>::infinity(), host);
        Check(false, "invalid delta rejected");
    } catch (const std::invalid_argument&) {
    }
}
void Build(pal4::NativeAngelScriptEngine& engine, std::string_view module,
           const char* text) {
    const std::string name(module);
    Check(engine.Raw()->AddScriptSection(name.c_str(), "fixture", text,
                                         static_cast<int>(std::strlen(text))) >= 0,
          "SDK adds fixture source");
    Check(engine.Raw()->Build(name.c_str()) >= 0, "SDK builds fixture module");
}
void SceneRelease() {
    pal4::NativeScriptTimer timer;
    Host host;
    Start(timer,0.5f,"pulse"); timer.Process(0.25f,host);
    timer.DisableForSceneRelease(); timer.DisableForSceneRelease();
    Check(!timer.Enabled() && timer.Elapsed()==0.25f && timer.Delay()==0.5f &&
              timer.Callback()=="pulse","scene reset only disables scheduling");
    timer.Process(100.0f,host);
    Check(!host.loads && timer.Elapsed()==0.25f,"old scene timer cannot fire after release");
    pal4::NativeAngelScriptEngine engine;
    unsigned loads{};
    pal4::NativeTimedScriptStack stack(engine,{},[&](std::string_view,std::string_view module) {
        ++loads; Build(engine,module,"void pulse(){ global_mvar=1; giWait(0.5f); global_mvar+=10; }");
    });
    Start(stack.Timer(),0.0f,"pulse");
    (void)stack.Process(0.0f,true,false,"Q01");
    Check(stack.Depth()==1u && engine.GlobalStoryVariable()==1,"timed script is suspended before scene reset");
    stack.Timer().DisableForSceneRelease();
    stack.ClearHandlers(); stack.ClearHandlers();
    Check(stack.Depth()==1u && engine.GlobalStoryVariable()==1,"scene wait cleanup retains SDK execution");
    (void)stack.Process(0.0f,true,false,"Q01");
    (void)stack.Process(100.0f,true,false,"Q01");
    Check(!stack.Active() && engine.GlobalStoryVariable()==11 && loads==1u,
          "scene wait reset resumes the SDK stack and disabled timer cannot retrigger");
}
void Stack() {
    pal4::NativeAngelScriptEngine engine;
    std::string program =
        "void pulse(){ global_mvar += 1; giWait(0.5f); global_mvar += 10; }";
    unsigned loads{}, interrupts{};
    pal4::NativeTimedScriptStack stack(
        engine, {}, [&](std::string_view source, std::string_view module) {
            Check(source == "Q01.csb", "SDK stack receives current world CSB name");
            ++loads;
            Build(engine, module, program.c_str());
        });
    stack.SetBeforePush([&] { ++interrupts; });
    Start(stack.Timer(), 0.25f, "pulse");
    Check(!stack.Process(0.125f, true, false, "Q01"), "stack timer not early");
    Check(stack.Process(0.125f, true, false, "Q01") && stack.Depth() == 1 &&
              engine.GlobalStoryVariable() == 1,
          "real SDK callback executes and owns giWait");
    Check(stack.Process(0.25f, true, false, "Q01") && loads == 1 &&
              engine.GlobalStoryVariable() == 1,
          "active timer module is deduplicated while waiting");
    Check(stack.Process(0.25f, true, false, "Q01") && !stack.Active() &&
              engine.GlobalStoryVariable() == 11,
          "wait resumes and finished context/module retire");
    Check(stack.Process(0.25f, true, false, "Q01") && loads == 2 && interrupts == 2,
          "finished module can be loaded again");
    stack.Clear();
    Check(!stack.Active(), "clear retires suspended context and waits");
    stack.Timer().Reset();
    program =
        "void outer(){ global_mvar=100; giTimeScript(0.0f,\"inner\"); giWait(5.0f); "
        "global_mvar+=1; }"
        "void inner(){ global_mvar+=10; giTimeScriptTerminate(); giWait(0.5f); "
        "global_mvar+=10; }";
    Start(stack.Timer(), 0.0f, "outer");
    (void)stack.Process(0.0f, true, false, "Q01");
    Check(stack.Depth() == 1 && engine.GlobalStoryVariable() == 100,
          "outer timer replaces timer registration through real callback");
    (void)stack.Process(0.0f, true, true, "Q01");
    Check(stack.Depth() == 2 && engine.GlobalStoryVariable() == 110 &&
              !stack.Timer().Enabled(),
          "nested timer pushes separate context and terminate stops future triggers");
    (void)stack.Process(0.25f, true, false, "Q01");
    Check(stack.Depth() == 2 && engine.GlobalStoryVariable() == 110,
          "nested context has independent wait");
    (void)stack.Process(0.25f, true, false, "Q01");
    Check(stack.Depth() == 1 && engine.GlobalStoryVariable() == 120,
          "nested context returns to suspended parent");
    (void)stack.Process(0.0f, true, false, "Q01");
    Check(!stack.Active() && engine.GlobalStoryVariable() == 121,
          "preemption clears displaced wait and parent resumes");
    program = "void other(){}";
    Start(stack.Timer(), 0.0f, "missing");
    const auto prior_interrupts = interrupts;
    Check(!stack.Process(0.0f, true, false, "Q01") && !stack.Active() &&
              interrupts == prior_interrupts && stack.Timer().Enabled() &&
              engine.GlobalStoryVariable() == 121,
          "absent optional timer entry leaves story and prior services unchanged");
    Check(!stack.Process(0.0f, true, false, "Q01") && !stack.Active() &&
              interrupts == prior_interrupts,
          "absent timed modules retire so repeated dispatch can retry");
    program = "void missing(){ giTimeScriptTerminate(); global_mvar=123; }";
    (void)stack.Process(0.0f, true, false, "Q01");
    Check(!stack.Active() && engine.GlobalStoryVariable() == 123,
          "later available optional entry runs without stale module");
    program = "void other(){}";
    Start(stack.Timer(), 0.0f, "");
    bool invalid_name{};
    try { (void)stack.Process(0.0f, true, false, "Q01"); }
    catch (const std::invalid_argument&) { invalid_name = true; }
    Check(invalid_name && !stack.Active() &&
              engine.FunctionId("TimeScript[]", "other") < 0,
          "empty callback remains an error and retires its loaded module");
    program = "void ambiguous(){} void ambiguous(int value){}";
    Start(stack.Timer(), 0.0f, "ambiguous");
    bool ambiguous{};
    try { (void)stack.Process(0.0f, true, false, "Q01"); }
    catch (const std::runtime_error&) { ambiguous = true; }
    Check(ambiguous && !stack.Active() &&
              engine.FunctionId("TimeScript[ambiguous]", "ambiguous") < 0,
          "other SDK lookup errors remain errors and retire loaded modules");
}

void AbsentTimerPreservesParentWait() {
    pal4::NativeAngelScriptEngine engine;
    const char* program =
        "void outer(){global_mvar=1;giTimeScript(0.1f,\"missing\");"
        "giWait(5.0f);global_mvar+=10;}";
    unsigned loads{}, interrupts{};
    pal4::NativeTimedScriptStack stack(engine, {}, [&](auto, auto module) {
        ++loads; Build(engine, module, program);
    });
    stack.SetBeforePush([&]{ ++interrupts; });
    Start(stack.Timer(), 0.0f, "outer");
    Check(stack.Process(0.0f, true, false, "M05") && stack.Depth() == 1u &&
              engine.GlobalStoryVariable() == 1, "parent owns its real SDK wait");
    for (int i = 0; i < 3; ++i)
        Check(stack.Process(0.1f, true, true, "M05") && stack.Depth() == 1u &&
                  engine.GlobalStoryVariable() == 1 && interrupts == 1u,
              "missing timed callback neither preempts nor clears parent wait");
    Check(loads == 4u && stack.Timer().Enabled(), "missing callback retriggers normally");
    (void)stack.Process(5.0f, true, true, "M05");
    Check(!stack.Active() && engine.GlobalStoryVariable() == 11 && interrupts == 1u,
          "parent finishes its original wait after absent callbacks");
}
void Countdown() {
    pal4::NativeAngelScriptEngine engine;
    unsigned loads{}, ticks{};
    bool fail_load{};
    std::string program="void pulse(){ global_mvar+=2; giWait(0.5f); global_mvar+=3; }";
    pal4::NativeTimedScriptStack stack(engine,{},[&](std::string_view source,std::string_view module){
        Check(source=="Q01.csb" && module=="PFNTimerScript[TimerScript]",
            "countdown loads current scene under its original fixed module key");
        ++loads;
        if(fail_load)throw std::runtime_error("countdown load failure");
        Build(engine,module,program.c_str());
    });
    pal4::MinimapSceneUiState state;
    const auto owner=reinterpret_cast<std::uint32_t>(&state);
    const std::string identifier="SceneTimer", function="pulse";
    state.WriteWord(0xbcu,0x7000u);
    state.WriteWord(0xd4u,reinterpret_cast<std::uint32_t>(identifier.c_str()));
    state.WriteWord(0xd8u,static_cast<std::uint32_t>(identifier.size()));
    state.WriteWord(0xe4u,reinterpret_cast<std::uint32_t>(function.c_str()));
    state.WriteWord(0xe8u,static_cast<std::uint32_t>(function.size()));
    state.WriteWord(0xf0u,1u);
    std::function<void()> tick;
    pal4::NativeGameTimerQueue queue([&](std::uint32_t context,std::uint32_t method){
        Check(context==owner && method==0x47caa0u,"actual queue invokes countdown owner/method");
        ++ticks;tick();
    });
    std::array<std::uint32_t,0x224u/4u> manager{};
    manager[0x1a0u/4u]=queue.Construct();
    std::vector<std::string> displayed;
    pal4::NativeUiCountdownScriptServices services{
        reinterpret_cast<std::uint32_t>(manager.data()),0xc000u,queue,stack,"Q01.csb",
        [&](std::uint32_t window,std::string_view text){
            Check(window==0x7000u,"countdown text uses portrait window");displayed.emplace_back(text);
        }};
    tick=[&]{(void)pal4::NativeUiCountdownScript::Execute(state,owner,services);};
    queue.AddRepeating(identifier,1.0f,owner,0x47caa0u);
    tick();
    Check(state.ReadWord(0xf0u)==0u && displayed==std::vector<std::string>{"1"} &&
              !stack.Active() && !loads,"positive countdown changes live save field without queuing a module");
    queue.Update(1.0f);
    Check(!ticks && !stack.Active() && state.ReadWord(0xf0u)==0u,
        "countdown preserves original timer's strict threshold at equality");
    queue.Update(0.001f);
    Check(ticks==1u && state.ReadWord(0xf0u)==0xffffffffu &&
              displayed==std::vector<std::string>{"1","0",""} && stack.Depth()==1u &&
              stack.StartedCount()==1u && loads==1u && engine.GlobalStoryVariable()==0,
        "expiry clears display, removes real timer and prepares SDK context without synchronous execution");
    queue.Update(2.0f);
    Check(ticks==1u && queue.Count()==0u,"expired actual timer cannot invoke countdown again");
    state.WriteWord(0xf0u,0u);tick();
    Check(stack.Depth()==1u && loads==1u && stack.StartedCount()==1u,
        "already stacked PFNTimerScript suppresses duplicate registration");
    Check(stack.Process(0.0f,false,true,"Q01") && stack.Active() && engine.GlobalStoryVariable()==2,
        "queued countdown callback actually executes in SDK even when story is busy");
    (void)stack.Process(0.25f,false,false,"Q01");
    Check(stack.Active() && engine.GlobalStoryVariable()==2,"countdown callback owns an actual suspended wait");
    (void)stack.Process(0.25f,false,false,"Q01");
    Check(!stack.Active() && engine.GlobalStoryVariable()==5 && stack.FinishedCount()==1u &&
              engine.FunctionId("PFNTimerScript[TimerScript]","pulse")<0,
        "countdown callback resumes, finishes and retires SDK module/context");
    fail_load=true;state.WriteWord(0xf0u,0u);
    bool load_rejected{};
    try{tick();}catch(const std::runtime_error& error){load_rejected=std::string(error.what())=="countdown load failure";}
    Check(load_rejected && !stack.Active() && state.ReadWord(0xf0u)==0xffffffffu,
        "countdown loader failure propagates without claiming a queued callback");
    fail_load=false;program="void other(){}";state.WriteWord(0xf0u,0u);
    tick();
    Check(!stack.Active() && engine.FunctionId("PFNTimerScript[TimerScript]","other")<0 &&
              engine.GlobalStoryVariable()==5,
        "absent optional countdown function retires its module without changing story");
    program="void pulse(){ global_mvar=77; }";state.WriteWord(0xf0u,0u);tick();
    Check(stack.Active() && engine.GlobalStoryVariable()==5,"countdown can prepare a retry after failures");
    (void)stack.Process(0.0f,false,false,"Q01");
    Check(!stack.Active() && engine.GlobalStoryVariable()==77,"retried countdown executes actual callback");
    queue.Destroy();
    Check(!queue.OutstandingAllocations(),"countdown queue releases callbacks, names and nodes");
    std::cout<<"Native UI countdown: live state, actual queue expiry, deferred SDK execution/wait, duplicate guard and failure retry verified\n";
}
void CountdownControl() {
    pal4::NativeAngelScriptEngine engine;
    unsigned loads{},ticks{};
    pal4::NativeTimedScriptStack stack(engine,{},[&](std::string_view source,std::string_view module){
        Check(source=="Q01.csb" && module=="PFNTimerScript[TimerScript]","controlled countdown source routing");
        ++loads;
        Build(engine,module,"void pulse(){ global_mvar+=10; giClearUiTimer(); }");
    });
    pal4::MinimapSceneUiState state;state.WriteWord(0xbcu,0x7000u);
    const auto owner=reinterpret_cast<std::uint32_t>(&state);
    pal4::NativeUiCountdown* live{};
    pal4::NativeGameTimerQueue queue([&](std::uint32_t context,std::uint32_t method){
        Check(live && context==owner && method==0x47caa0u,"controlled countdown actual callback target");
        ++ticks;live->Tick();
    });
    std::array<std::uint32_t,0x224u/4u> manager{};
    manager[0x1a0u/4u]=queue.Construct();
    const auto manager_address=reinterpret_cast<std::uint32_t>(manager.data());
    std::vector<std::string> displayed;
    bool fail_text{};
    const auto set_text=[&](std::uint32_t window,std::string_view text){
        Check(window==0x7000u,"controlled countdown HUD text identity");
        if(fail_text)throw std::runtime_error("countdown text failure");
        displayed.emplace_back(text);
    };
    auto countdown=std::make_unique<pal4::NativeUiCountdown>(state,owner,manager_address,queue,
        [&]{(void)pal4::NativeUiCountdownScript::Execute(state,owner,
            {manager_address,0xc000u,queue,stack,"Q01.csb",set_text});},set_text);
    live=countdown.get();
    pal4::NativeGameScriptServices services;services.ui_countdown=live;
    stack.BindServices(services);
    pal4::NativeGameScriptCallbacks callbacks(services);engine.SetCallbacks(&callbacks);
    Build(engine,"controls",
        "void start(){ giStartUiTimer(3,\"pulse\"); }"
        "void restart(){ giStartUiTimer(5,\"pulse\"); }"
        "void pause(){ giPauseUiTimer(); }"
        "void resume(){ giResumeUiTimer(); }"
        "void clear(){ giClearUiTimer(); }"
        "void zero(){ giStartUiTimer(0,\"pulse\"); }"
        "void negative(){ giStartUiTimer(-1,\"pulse\"); }"
        "void empty(){ giStartUiTimer(3,\"\"); }");
    const auto run=[&](const char* name){
        pal4::NativeScriptFrameRunner runner(engine);runner.Queue("controls",name);
        Check(runner.Tick(0.0f)==pal4::NativeScriptFrameStatus::finished,
            "actual SDK countdown control entry completes");
    };
    run("start");
    Check(state.ReadWord(0xf0u)==2u && state.ReadWord(0xe8u)==5u &&
              std::string(reinterpret_cast<const char*>(state.ReadWord(0xe4u)))=="pulse" &&
              displayed.back()=="3" && queue.Count()==1u,"SDK start owns callback text and immediately displays/decrements live save state");
    run("pause");queue.Update(5.0f);
    Check(!ticks && !queue.Count() && state.ReadWord(0xf0u)==2u && state.ReadWord(0xe8u)==5u && displayed.back()=="3",
        "SDK pause retires actual task while preserving callback, duration and text");
    run("resume");queue.Update(1.0f);
    Check(!ticks && state.ReadWord(0xf0u)==2u,"SDK resume retains original strict one-second threshold");
    queue.Update(0.001f);
    Check(ticks==1u && state.ReadWord(0xf0u)==1u && displayed.back()=="2","resumed task reaches same live countdown state");
    run("pause");queue.Update(0.001f);
    run("resume");run("resume");
    Check(queue.Count()==2u,"repeated SDK Resume retains original duplicate-task policy");
    queue.Update(1.001f);
    Check(ticks==3u && stack.Active() && loads==1u && engine.GlobalStoryVariable()==0,
        "duplicate resumed tasks decrement then expire once without synchronous script execution");
    (void)stack.Process(0.0f,false,false,"Q01");queue.Update(0.001f);
    Check(!stack.Active() && !queue.Count() && state.ReadWord(0xf0u)==0u && !state.ReadWord(0xe8u) &&
              displayed.back().empty() && engine.GlobalStoryVariable()==10,
        "actual expiry script invokes giClearUiTimer through its context-bound service");
    run("resume");Check(!queue.Count(),"clear's empty callback blocks subsequent Resume");
    run("start");run("restart");queue.Update(0.001f);
    Check(queue.Count()==1u && state.ReadWord(0xf0u)==4u && displayed.back()=="5",
        "restart retires previous callback task and immediately displays new duration");
    run("clear");queue.Update(0.001f);
    fail_text=true;bool rejected{};
    try{countdown->Start(2u,"pulse");}catch(const std::runtime_error& error){rejected=std::string(error.what())=="countdown text failure";}
    Check(rejected && state.ReadWord(0xf0u)==2u && queue.Count()==1u && !stack.Active(),
        "start text failure preserves original earlier task/text assignment effects");
    fail_text=false;run("clear");queue.Update(0.001f);
    run("zero");
    Check(state.ReadWord(0xf0u)==0xffffffffu && stack.Active() && engine.GlobalStoryVariable()==10,
        "zero start expires immediately but only prepares its script");
    (void)stack.Process(0.0f,false,false,"Q01");queue.Update(0.001f);
    run("negative");
    Check(state.ReadWord(0xf0u)==0xfffffffeu && stack.Active() &&
              displayed[displayed.size()-2u]=="-1","negative SDK duration preserves signed display and 32-bit wraparound");
    (void)stack.Process(0.0f,false,false,"Q01");queue.Update(0.001f);
    run("empty");run("pause");queue.Update(0.001f);run("resume");
    Check(!queue.Count() && state.ReadWord(0xf0u)==2u && !state.ReadWord(0xe8u),
        "empty callback Start can pause but Resume's original nonempty gate prevents a new task");
    run("clear");run("start");
    const auto before=ticks;
    countdown.reset();live=nullptr;queue.Update(100.0f);
    Check(ticks==before && !queue.Count() && !state.ReadWord(0xd4u) && !state.ReadWord(0xe4u),
        "countdown retirement removes queued owner borrows before native text storage dies");
    stack.Clear();engine.SetCallbacks(nullptr);queue.Destroy();
    Check(!queue.OutstandingAllocations(),"controlled countdown native allocations return to zero");
    std::cout<<"Native UI countdown controls: real SDK start/pause/resume/clear, immediate tick, duplicate tasks, signed expiry and retirement verified\n";
}
struct ContextRender final : pal4::IEffectRenderRuntime {
    std::uint32_t DestroyTexture(std::uint32_t) override {return 1u;}
    std::uint32_t TextureDictionary() override {return 1u;}
    std::uint32_t CreateTexture(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) override {return 1u;}
    std::int32_t FloatToLong(float value) override {return static_cast<std::int32_t>(value);}
    std::uint32_t DrawTexturedRectangle(std::uint32_t,std::int32_t,std::int32_t,std::int32_t,std::int32_t,
        std::uint32_t,std::uint8_t,std::uint8_t,std::uint8_t,std::int32_t) override {return 1u;}
    float Interpolate(float first,float last,float fraction,std::uint32_t) override {return first+(last-first)*fraction;}
};
void OwnedTimedContexts() {
    pal4::NativeAngelScriptEngine engine;
    ContextRender render;pal4::PalScreenEffectState effect{};
    (void)pal4::EffectRenderControl::Initialize(effect.render,800,600);
    pal4::EffectRenderControl::InitializeTexture(effect.render,render);
    unsigned created{},destroyed{},cleared{};std::vector<asIScriptContext*> contexts;
    struct Owner final : pal4::INativeTimedScriptContext {
        pal4::NativeScriptFlashRuntime flash;unsigned& destroyed;unsigned& cleared;
        Owner(asIScriptContext& context,pal4::PalScreenEffectState& effect,ContextRender& render,
            unsigned& retired,unsigned& cleanups):flash(effect,render,context),destroyed(retired),cleared(cleanups){}
        ~Owner() override {++destroyed;}
        pal4::NativeGameScriptServices BindServices(pal4::NativeGameScriptServices services) override {
            services.flash=&flash.Flash();return services;
        }
        pal4::INativeScriptFrameWaitRuntime* Waits() noexcept override {return &flash;}
        pal4::INativeScriptFrameWaitRuntime* HandlerWaits() noexcept override {return &flash.Handlers();}
        void ClearHandlers() override {flash.ClearHandlers();++cleared;}
    };
    pal4::NativeTimedScriptStack stack(engine,{},[&](auto,auto module){
        Build(engine,module,"void pulse(){ global_mvar=1; giFlashOutBlack(0.1f,true,true); global_mvar+=10; giWait(0.25f); global_mvar+=100; }");
    });
    const auto factory=[&](asIScriptContext& context){
        ++created;contexts.push_back(&context);
        return std::make_unique<Owner>(context,effect,render,destroyed,cleared);
    };
    stack.SetContextFactory(factory);
    Start(stack.Timer(),0.0f,"pulse");(void)stack.Process(0.0f,false,false,"Q01");stack.Timer().DisableForSceneRelease();
    Check(stack.Active() && engine.GlobalStoryVariable()==1 && created==1u && !destroyed,
        "timed context owns an actual SDK flash wait service");
    bool active_factory_rejected{};
    try{stack.SetContextFactory({});}catch(const std::logic_error&){active_factory_rejected=true;}
    Check(active_factory_rejected,"live context factory cannot be replaced");
    Start(stack.Timer(),1.0f,"later");
    const auto fade_elapsed=effect.render.elapsed;
    (void)stack.ProcessSelected(0.05f,true);
    Check(engine.GlobalStoryVariable()==1 && effect.render.elapsed==fade_elapsed &&
              stack.Timer().Elapsed()==0.0f && stack.Timer().Enabled(),
        "video handler processing leaves screen and periodic clock to their state owner");
    (void)stack.ProcessSelected(0.05f);
    Check(effect.render.elapsed>fade_elapsed && engine.GlobalStoryVariable()==1,
        "ordinary selected context advances real flash once before observing its wait");
    stack.Timer().DisableForSceneRelease();
    (void)stack.Process(0.2f,false,false,"Q01");
    Check(engine.GlobalStoryVariable()==11 && stack.Active(),"flash completion resumes into existing per-entry giWait");
    (void)stack.Process(0.251f,false,false,"Q01");
    Check(engine.GlobalStoryVariable()==111 && !stack.Active() && destroyed==1u && cleared,
        "both frame wait owners finish before timed context retirement");
    Start(stack.Timer(),0.0f,"pulse");(void)stack.Process(0.0f,false,false,"Q01");stack.Timer().DisableForSceneRelease();
    auto* displaced=contexts.back();stack.DetachContextServices();
    Check(stack.Active() && destroyed==2u && engine.GlobalStoryVariable()==1,
        "scene detach retires context services but preserves suspended SDK instruction position");
    stack.BindServices({});
    Check(created==3u && contexts.back()==displaced,"rebind recreates wait owners on the same suspended SDK context");
    (void)stack.Process(0.0f,false,false,"Q01");
    Check(engine.GlobalStoryVariable()==11,"rebound script resumes after its cancelled old-scene wait");
    stack.Clear();Check(destroyed==created,"teardown retires every context service owner");
    stack.SetContextFactory([](auto&)->std::unique_ptr<pal4::INativeTimedScriptContext>{return {};});
    Start(stack.Timer(),0.0f,"pulse");bool empty_rejected{};
    try{(void)stack.Process(0.0f,false,false,"Q01");}catch(const std::logic_error&){empty_rejected=true;}
    Check(empty_rejected && !stack.Active(),"factory failure does not install a timed context");
    (void)pal4::EffectRenderControl::DestroyTexture(effect.render,render);
    std::cout<<"Timed context services: actual flash/giWait, detach/rebind, SDK position and owner teardown verified\n";
}
void CountdownPreemption() {
    pal4::NativeAngelScriptEngine engine;
    unsigned loads{},clears{},previous_hook{};
    bool fail_cleanup{};
    pal4::NativeTimedScriptStack stack(engine,{},[&](std::string_view source,std::string_view module){
        Check(source=="Q01.csb","preemption uses current scene source");++loads;
        if(module=="PFNTimerScript[TimerScript]")
            Build(engine,module,"void pulse(){ global_mvar+=10; giWait(0.25f); global_mvar+=20; }");
        else
            Build(engine,module,"void outer(){ global_mvar=4; giTimeScriptTerminate(); giStartUiTimer(0,\"pulse\"); global_mvar+=400; }");
    });
    Build(engine,"parent",
        "void story(){ global_mvar=1; giStartUiTimer(0,\"pulse\"); global_mvar+=100; }"
        "void waiting(){ global_mvar=2; giWait(10.0f); global_mvar+=200; }"
        "void missing(){ global_mvar=3; giStartUiTimer(0,\"missing\"); global_mvar+=300; }");
    pal4::NativeScriptFrameRunner parent(engine);
    pal4::NativeScriptTimedWaitRuntime parent_wait(parent.Context());
    class Wait final:public pal4::INativeScriptFrameWaitRuntime {
    public:
        explicit Wait(pal4::NativeScriptTimedWaitRuntime& value):value_(value){}
        bool BlocksScript()const override{return value_.Waiting();}
        bool Tick(float elapsed)override{return value_.Tick(elapsed);}
    private:pal4::NativeScriptTimedWaitRuntime& value_;
    } waits(parent_wait);
    pal4::MinimapSceneUiState state;state.WriteWord(0xbcu,0x7000u);
    const auto owner=reinterpret_cast<std::uint32_t>(&state);
    pal4::NativeUiCountdown* live{};
    pal4::NativeGameTimerQueue queue([&](auto context,auto method){
        Check(live && context==owner && method==0x47caa0u,"preemption countdown callback identity");live->Tick();
    });
    std::array<std::uint32_t,0x224u/4u> manager{};manager[0x1a0u/4u]=queue.Construct();
    const auto address=reinterpret_cast<std::uint32_t>(manager.data());
    pal4::NativeGameScriptServices services;services.wait=&parent_wait;
    const auto set_text=[](std::uint32_t window,std::string_view){Check(window==0x7000u,"preemption text identity");};
    auto control=std::make_unique<pal4::NativeUiCountdown>(state,owner,address,queue,[&]{
        stack.DispatchUiScript(services,[&]{
            Check(asGetActiveContext()==nullptr,"displaced handlers clear only after native callback unwinds");
            if(fail_cleanup)throw std::runtime_error("preemption cleanup failure");
            ++clears;parent_wait.ClearHandlers();
        },[&](auto& scripts){
            (void)pal4::NativeUiCountdownScript::Execute(state,owner,
                {address,0xc000u,queue,scripts,"Q01.csb",set_text});
        });
    },set_text);
    live=control.get();services.ui_countdown=live;
    pal4::NativeGameScriptCallbacks callbacks(services);engine.SetCallbacks(&callbacks);
    parent.Queue("parent","story");
    Check(parent.Tick(0.0f,&waits)==pal4::NativeScriptFrameStatus::suspended &&
              parent.Context().GetState()==asEXECUTION_SUSPENDED && engine.GlobalStoryVariable()==1 &&
              stack.Depth()==1u && !clears,"zero countdown suspends active parent before its next statement and defers handler cleanup");
    fail_cleanup=true;bool failed{};
    try{(void)stack.Process(0.0f,false,true,"Q01");}
    catch(const std::runtime_error& error){failed=std::string(error.what())=="preemption cleanup failure";}
    Check(failed && engine.GlobalStoryVariable()==1 && stack.Depth()==1u && !clears,
        "child cannot execute while displaced cleanup fails");
    fail_cleanup=false;
    (void)stack.Process(0.0f,false,true,"Q01");
    Check(clears==1u && engine.GlobalStoryVariable()==11 && stack.Active() &&
              parent.Context().GetState()==asEXECUTION_SUSPENDED,"cleanup retry precedes actual child SDK execution and keeps parent paused");
    (void)stack.Process(0.25f,false,true,"Q01");
    Check(!stack.Active() && engine.GlobalStoryVariable()==31,"preempting countdown finishes its own SDK wait");
    Check(parent.TickUntilBoundary(0.0f,&waits,[]{return false;})==pal4::NativeScriptFrameStatus::finished && engine.GlobalStoryVariable()==131,
        "parent resumes at original next statement after child retirement");
    queue.Update(0.001f);control->Clear();
    parent.Queue("parent","waiting");
    Check(parent.Tick(0.0f,&waits)==pal4::NativeScriptFrameStatus::suspended && parent_wait.Waiting(),
        "parent owns a real ten-second wait before asynchronous expiry");
    control->Start(0u,"pulse");
    Check(clears==2u && !parent_wait.Waiting() && engine.GlobalStoryVariable()==2 && stack.Active(),
        "expiry outside native execution retires paused parent waits immediately");
    (void)stack.Process(0.0f,false,true,"Q01");(void)stack.Process(0.25f,false,true,"Q01");
    Check(parent.TickUntilBoundary(0.0f,&waits,[]{return false;})==pal4::NativeScriptFrameStatus::finished && engine.GlobalStoryVariable()==232,
        "paused parent resumes without resurrecting its displaced ten-second wait");
    queue.Update(0.001f);control->Clear();
    const auto before=clears;
    parent.Queue("parent","missing");
    Check(parent.Tick(0.0f,&waits)==pal4::NativeScriptFrameStatus::finished && !stack.Active() &&
              clears==before && engine.GlobalStoryVariable()==303,
        "absent countdown child leaves parent running without clearing its handlers");
    parent.ClearExecution();parent_wait.ClearHandlers();queue.Update(0.001f);control->Clear();
    // A countdown can also preempt a currently running timed SDK context.
    Start(stack.Timer(),0.0f,"outer");
    (void)stack.Process(0.0f,true,false,"Q01");
    Check(stack.Depth()==2u && engine.GlobalStoryVariable()==4 && !stack.Timer().Enabled(),
        "UI countdown preempts actual timed parent while retaining both SDK contexts");
    (void)stack.Process(0.0f,false,true,"Q01");(void)stack.Process(0.25f,false,true,"Q01");
    Check(stack.Depth()==1u && engine.GlobalStoryVariable()==34,"nested countdown returns to paused timed parent");
    (void)stack.Process(0.0f,false,false,"Q01");
    Check(!stack.Active() && engine.GlobalStoryVariable()==434,"timed parent resumes after nested UI countdown");
    queue.Update(0.001f);control->Clear();
    stack.SetBeforePush([&]{++previous_hook;});
    bool dispatch_failed{};
    try{stack.DispatchUiScript(services,[]{},[](auto&){throw std::runtime_error("dispatch failure");});}
    catch(const std::runtime_error& error){dispatch_failed=std::string(error.what())=="dispatch failure";}
    Check(dispatch_failed,"dispatch error propagates");
    Start(stack.Timer(),0.0f,"outer");
    (void)stack.Process(0.0f,true,false,"Q01");
    Check(previous_hook==1u && stack.Depth()==2u,"enclosing timed-frame hook is restored after failed countdown dispatch");
    stack.Clear();stack.Timer().Reset();stack.SetBeforePush({});
    control.reset();live=nullptr;queue.Update(0.001f);queue.Destroy();engine.SetCallbacks(nullptr);
    Check(!queue.OutstandingAllocations(),"preempting countdown retires all native queue allocations");
    std::cout<<"Native UI countdown preemption: active/waiting/timed parents, deferred cleanup/retry, SDK resume and hook restoration verified\n";
}
}  // namespace
int main() {
    try {
        Policy();
        SceneRelease();
        Stack();
        AbsentTimerPreservesParentWait();
        Countdown();
        CountdownControl();
        CountdownPreemption();
        OwnedTimedContexts();
        std::cout << "Native timer: shared policy, real SDK callbacks, repeating/nested "
                     "contexts, waits and cleanup verified\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
