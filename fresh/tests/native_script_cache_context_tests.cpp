#include "pal4/native_script_cache_context.h"
#include "pal4/native_game_script_callbacks.h"
#include "pal4/native_script_frame_runner.h"
#include "pal4/native_script_timed_wait_runtime.h"
#include "pal4/native_scene_script_stack.h"
#include "pal4/native_timed_script_stack.h"

#include <bit>

#include <iostream>
#include <stdexcept>

namespace {
void Check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
void VerifyTimedCache() {
    pal4::NativeAngelScriptEngine engine;
    pal4::NativeTimedScriptStack stack(engine, {}, [&](auto, auto module){
        constexpr char source[] = "void Outer(){global_mvar=10;giWait(10);global_mvar+=1000;}"
                                "void Inner(){giScriptClearCTXButCurrent();global_mvar+=1;}"
                                "void InnerWait(){giScriptClearCTXButCurrent();giWait(0.5);global_mvar+=1;}";
        const std::string name(module);
        Check(engine.Raw()->AddScriptSection(name.c_str(), "timed.cs", source, sizeof(source)-1) >= 0 &&
              engine.Raw()->Build(name.c_str()) >= 0, "compile timed cache fixture");
    });
    class FactoryOwner final : public pal4::INativeTimedScriptContext {
    public:
        pal4::NativeGameScriptServices BindServices(pal4::NativeGameScriptServices s) override {
            // Real application factories rebuild their context services. The
            // selected cache must survive independently of that reconstruction.
            s.cache_context = nullptr; return s;
        }
        pal4::INativeScriptFrameWaitRuntime* Waits() noexcept override { return nullptr; }
        void ClearHandlers() override {}
    };
    stack.SetContextFactory([](auto&){return std::make_unique<FactoryOwner>();});
    unsigned cleanup{};
    pal4::NativeScriptCacheContext cache([&]{
        std::vector<pal4::NativeScriptCacheEntry> entries;
        stack.AppendCacheEntries(entries); return entries;
    }, [&]{stack.RetireClearedEntries();});
    cache.SetHandlerCleanup([&]{++cleanup;});
    pal4::NativeGameScriptServices services{}; services.cache_context = &cache;
    stack.BindServices(services);
    const auto schedule=[&](const char* name){
        const std::uint32_t record[]{0u,reinterpret_cast<std::uint32_t>(name)};
        (void)pal4::ScriptTimedState::Start(std::bit_cast<std::uint32_t>(.1f),
            reinterpret_cast<std::uint32_t>(record),stack.Timer());
    };
    schedule("Outer");
    Check(stack.Process(.1f,true,false,"Q01") && stack.Depth()==1 && engine.GlobalStoryVariable()==10,
          "outer timed SDK context owns suspended execution");
    schedule("Inner");
    Check(stack.Process(.1f,true,false,"Q01") && !stack.Active() && cleanup==1 &&
          stack.StartedCount()==2 && stack.FinishedCount()==1 && engine.GlobalStoryVariable()==11,
          "nested timed callback retains session cache across factory replacement and retires cancelled module");
    Check(!stack.ProcessSelected(20) && engine.GlobalStoryVariable()==11,
          "retired timed parent never resumes after current completion");
    schedule("Outer");
    Check(stack.Process(.1f,true,false,"Q01") && engine.GlobalStoryVariable()==10,
          "new parent starts for callback/wait ordering fixture");
    schedule("InnerWait");
    Check(stack.Process(.1f,true,false,"Q01") && stack.Depth()==1 && cleanup==2 && engine.GlobalStoryVariable()==10,
          "current's following wait survives callback-boundary cleanup");
    Check(stack.ProcessSelected(.2f) && stack.Active() && engine.GlobalStoryVariable()==10,
          "new wait still blocks after cancellation cleanup");
    Check(stack.ProcessSelected(.3f) && !stack.Active() && engine.GlobalStoryVariable()==11,
          "current resumes only after its new wait duration");
}
void VerifySceneCache() {
    using Status = pal4::NativeScriptFrameStatus;
    pal4::NativeAngelScriptEngine engine;
    constexpr char source[] = "void Outer(){giWait(20);global_mvar=999;}"
                            "void Inner(){giScriptClearCTXButCurrent();global_mvar=7;}";
    Check(engine.Raw()->AddScriptSection("nested", "nested.cs", source, sizeof(source)-1) >= 0 &&
          engine.Raw()->Build("nested") >= 0, "compile nested cache fixture");
    pal4::NativeSceneScriptStack stack(engine);
    pal4::NativeScriptTimedWaitRuntime wait(stack.Runner().Context());
    unsigned cleanup{};
    pal4::NativeScriptCacheContext cache([&]{
        std::vector<pal4::NativeScriptCacheEntry> entries;
        stack.AppendCacheEntries(entries); return entries;
    }, []{});
    cache.SetHandlerCleanup([&]{++cleanup; wait.ClearHandlers();});
    pal4::NativeGameScriptServices services{};
    services.wait = &wait; services.cache_context = &cache;
    stack.Callbacks().SetServices(services); stack.Queue("nested", "Outer");
    auto* outer = &stack.Runner().Context();
    Check(stack.Runner().TickUntilBoundary(0, &wait, []{return false;}) == Status::suspended,
          "outer scene waits before nested cancellation");
    wait.ClearHandlers(); stack.Queue("nested", "Inner");
    stack.Callbacks().SetServices(services);
    Check(stack.Runner().Tick(0) == Status::finished && engine.GlobalStoryVariable() == 7 &&
          cleanup == 1 && !stack.HasPendingEntries() && stack.PopFinished() && stack.Depth() == 1 &&
          &stack.Runner().Context() == outer && stack.Runner().Status() == Status::idle,
          "nested clear preserves current through return and cancelled lower cannot block future timers");
    Check(stack.Runner().Tick(30, &wait) == Status::idle && engine.GlobalStoryVariable() == 7,
          "cancelled parent cannot execute its old continuation after nested pop");
}
void Run() {
    using Status = pal4::NativeScriptFrameStatus;
    pal4::NativeAngelScriptEngine engine;
    constexpr char source[] =
        "void Outer() { global_mvar=10; giWait(10.0); global_mvar+=1000; }"
        "void Clear() { giScriptClearCTXButCurrent(); global_mvar+=1; }"
        "void cLeAr() { giScriptClearCTXButCurrent(); global_mvar+=3; }";
    Check(engine.Raw()->AddScriptSection("cache", "cache.cs", source, sizeof(source)-1) >= 0 &&
          engine.Raw()->Build("cache") >= 0, "compile SDK cache cancellation fixture");
    Check(engine.Raw()->AddScriptSection("external[Clear]", "cache.cs", source, sizeof(source)-1) >= 0 &&
          engine.Raw()->Build("external[Clear]") >= 0, "compile original full-key module fixture");
    pal4::NativeScriptFrameRunner outer(engine), current(engine);
    pal4::NativeScriptTimedWaitRuntime wait(outer.Context());
    unsigned cleanups{}, retired{}, other_handlers{};
    pal4::NativeScriptCacheContext cache([&] {
        std::vector<pal4::NativeScriptCacheEntry> entries;
        for (auto* runner : {&outer, &current}) {
            if (runner->Status() == Status::queued || runner->Status() == Status::suspended)
                entries.push_back({runner, runner == &outer ? std::function<void()>([&]{++other_handlers;wait.ClearHandlers();}) : std::function<void()>{}});
        }
        return entries;
    }, [&]{++retired;});
    cache.SetHandlerCleanup([&]{++cleanups;});
    pal4::NativeGameScriptServices services{};
    services.cache_context = &cache; services.wait = &wait;
    pal4::NativeGameScriptCallbacks callbacks(services);
    engine.SetCallbacks(&callbacks);
    outer.Queue("cache", "Outer");
    Check(outer.TickUntilBoundary(0, &wait, []{return false;}) == Status::suspended && wait.Waiting(),
          "real SDK outer suspends with live timer");
    auto* stable_outer = &outer.Context();
    current.Queue("cache", "Clear");
    Check(current.Tick(0) == Status::finished && engine.GlobalStoryVariable() == 11 &&
          outer.Status() == Status::idle && !wait.Waiting() && cleanups == 1 && retired == 1 && other_handlers == 1,
          "callback prunes different named execution and waits while current continues");
    Check(&outer.Context() == stable_outer && outer.Tick(20, &wait) == Status::idle && engine.GlobalStoryVariable() == 11,
          "borrowed context stays valid and cancelled instruction cannot resume");
    current.Queue("cache", "Clear");
    Check(current.Tick(0) == Status::finished && engine.GlobalStoryVariable() == 12 && cleanups == 1 && retired == 1,
          "single current entry follows original count gate without handler cleanup");
    outer.Queue("cache", "cLeAr"); current.Queue("cache", "Clear");
    Check(current.Tick(0) == Status::finished && outer.Status() == Status::queued && cleanups == 2 && retired == 2,
          "shared original case-insensitive cache-name policy preserves equal names");
    current.Queue("cache", "Outer");
    bool rejected{};
    try { (void)pal4::ScriptCacheContext::ClearExceptCurrent(cache); }
    catch (const std::logic_error&) { rejected = true; }
    Check(rejected && outer.Status() == Status::queued && current.Status() == Status::queued,
          "missing active context rejects before cancelling another execution");
    outer.ClearExecution(); current.ClearExecution();
    current.Queue("external[Clear]", "Clear");
    Check(current.CacheName() == "external[Clear]" && current.Tick(0) == Status::finished,
          "original fully qualified external keys retain one function suffix");
    engine.SetCallbacks(nullptr);
}
}
int main() {
    try { Run(); VerifySceneCache(); VerifyTimedCache(); std::cout << "PASS: real SDK current-cache preservation, cancellation, waits, names and count gate\n"; }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
