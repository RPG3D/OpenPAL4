#include "pal4/native_new_game_script_session.h"
#include "support/event_arena_runtime.h"
#include "pal4/native_script_timed_wait_runtime.h"
#include "pal4/native_script_module_loader.h"
#include "pal4/native_game_arena_manager_owner.h"
#include "pal4/script_music_transport.h"
#include "pal4/script_completed_handler_queue.h"
#include "pal4/world_map_buttons.h"
#include <cstring>

#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void VerifySceneStack() {
    using Status = pal4::NativeScriptFrameStatus;
    pal4::NativeAngelScriptEngine engine;
    constexpr char source[] =
        "void Outer() { global_mvar = 10; giWait(1.0); global_mvar += 100; } "
        "void Inner() { global_mvar += 1; }";
    Check(engine.Raw()->AddScriptSection("scene_stack", "stack.cs", source, sizeof(source)-1) >= 0 &&
              engine.Raw()->Build("scene_stack") >= 0, "compile nested SDK scene-stack probe");
    pal4::NativeSceneScriptStack stack(engine);
    auto* outer = &stack.Runner().Context();
    pal4::NativeScriptTimedWaitRuntime wait(*outer);
    pal4::NativeGameScriptServices services{};
    services.wait = &wait;
    stack.Callbacks().SetServices(services);
    stack.Queue("scene_stack", "Outer");
    Check(stack.Runner().TickUntilBoundary(0.0f, &wait, [] { return false; }) == Status::suspended &&
              engine.GlobalStoryVariable() == 10 && wait.Waiting(),
          "outer scene context suspends at its real timed callback");
    Check(!stack.Queue("scene_stack", "AbsentInit") &&
              stack.Depth() == 1u && &stack.Runner().Context() == outer &&
              stack.Runner().Status() == Status::suspended && wait.Waiting() &&
              outer->GetState() == asEXECUTION_SUSPENDED && engine.GlobalStoryVariable() == 10,
          "missing initializer preserves suspended SDK context, timed wait and story");
    wait.ClearHandlers();
    stack.DetachServices({});
    stack.Queue("scene_stack", "Inner");
    Check(stack.Depth() == 2u && &stack.Runner().Context() != outer &&
              outer->GetState() == asEXECUTION_SUSPENDED && engine.GlobalStoryVariable() == 10,
          "pushing prepares an independent top without executing or replacing suspended SDK state");
    auto* inner = &stack.Runner().Context();
    Check(!stack.Queue("SCENE_STACK", "oUtEr") && !stack.Queue("scene_stack", "Inner") &&
              stack.Depth() == 2u && &stack.Runner().Context() == inner &&
              engine.GlobalStoryVariable() == 10,
          "original case-insensitive full-function duplicate check covers top and suspended lower entries");
    bool rejected = false;
    try { (void)stack.PopFinished(); } catch (const std::logic_error&) { rejected = true; }
    Check(rejected && stack.Depth() == 2u, "prepared top cannot pop before completion");
    Check(stack.Runner().Tick(0.0f) == Status::finished && engine.GlobalStoryVariable() == 11 &&
              stack.PopFinished() && stack.Depth() == 1u && &stack.Runner().Context() == outer,
          "inner executes once then restores the exact previous SDK context");
    stack.Callbacks().SetServices(services);
    Check(stack.Runner().TickUntilBoundary(0.0f, &wait, [] { return false; }) == Status::finished &&
              engine.GlobalStoryVariable() == 111,
          "outer resumes after giWait with its prior instruction position, without restarting");
    stack.Queue("scene_stack", "Outer");
    Check(stack.Runner().TickUntilBoundary(0.0f, &wait, [] { return false; }) == Status::suspended,
          "repeat outer establishes populated nested teardown");
    wait.ClearHandlers();
    stack.Queue("scene_stack", "Inner");
    stack.ClearExecution();
    stack.DetachServices({});
    Check(stack.Runner().Status() == Status::idle && outer->GetState() != asEXECUTION_ACTIVE,
          "full stack cleanup stops every SDK context while host wait references remain valid");
    stack.Queue("scene_stack", "Inner");
    Check(stack.Depth() == 1u && &stack.Runner().Context() == outer &&
              stack.Runner().Tick(0.0f) == Status::finished,
          "new start reuses the base context after cleared nested stack teardown");
}

void VerifyExternalMachineSourceSteps(const std::filesystem::path& root) {
    struct CompletedQueue final : pal4::IScriptCompletedHandlerQueue {
        unsigned count{};
        void AppendCompletedHandler(std::uint32_t, std::int32_t, std::string_view) override { ++count; }
    } primary_queue, selected_queue;
    struct Context final : pal4::INativeTimedScriptContext {
        CompletedQueue& queue;
        explicit Context(CompletedQueue& value) : queue(value) {}
        pal4::NativeGameScriptServices BindServices(pal4::NativeGameScriptServices s) override {
            s.completed_handlers=&queue;return s;
        }
        pal4::INativeScriptFrameWaitRuntime* Waits() noexcept override { return nullptr; }
        void ClearHandlers() override {}
    };
    struct MarkerMusic final : pal4::IScriptMusicTransportRuntime {
        unsigned frame{};
        std::vector<unsigned> callback_frames;
        std::vector<std::string> callbacks;
        std::uint32_t BackgroundMusicPlayer() override {return 0x1000u;}
        std::uint8_t StopSound(std::uint32_t slot) override {
            Check(slot==0x1030u,"external marker pause uses existing music slot");
            callback_frames.push_back(frame);callbacks.emplace_back("pause");return 1u;
        }
        std::uint8_t LoadAndPlay(std::uint32_t player) override {
            Check(player==0x1000u,"external marker resume uses existing music owner");
            callback_frames.push_back(frame);callbacks.emplace_back("resume");return 1u;
        }
    } music;
    pal4::NativeGameScriptServices services;services.music=&music;
    services.completed_handlers=&primary_queue;
    pal4::NativeNewGameScriptSession session(root,services);
    session.SetTimedContextFactory([&](asIScriptContext&){return std::make_unique<Context>(selected_queue);});
    Check(session.SelectedCompletedHandlers()==&primary_queue,"idle external operations retain the primary completed queue");
    // Use the session's real SDK engine and external push API, matching Machine
    // ScriptTask dispatch. Do not test the low-level step=true primitive alone.
    auto* sdk=session.Context().GetEngine();
    constexpr char module[]="palGameOMTask_script[session_source_steps]";
    constexpr char source[]=
        "void Machine(){\n"
        " giScriptMusicPause();\n"
        " giScriptMusicResume();\n"
        " giWait(0.034f);\n"
        " giScriptMusicPause();\n"
        " giScriptMusicResume();\n"
        "}\n";
    Check(sdk->AddScriptSection(module,"machine_source_steps.cs",source,sizeof(source)-1)>=0 &&
        sdk->Build(module)>=0,"compile real SDK machine external module on the session engine");
    unsigned cleanups{};
    session.PushExternalStackModule(module,"Machine",services,[&]{++cleanups;});
    Check(session.SelectedCompletedHandlers()==&selected_queue,"external Machine selects its own completed queue");
    session.SelectedCompletedHandlers()->AppendCompletedHandler(0x849544u,1,"player-Player-Movment");
    Check(selected_queue.count==1u && primary_queue.count==0u,
        "diagnostic position completion reaches selected Machine rather than unrelated primary waits");
    Check(session.TimedScriptsActive() && session.TimedScriptsStarted()==1u &&
        music.callbacks.empty() && cleanups==1u,"Machine external push prepares without executing source callbacks");
    while(session.TimedScriptsActive() && music.frame<64u){
        ++music.frame;
        const auto before=music.callbacks.size();
        Check(session.ProcessTimedScripts(.017f,true,services,[&]{++cleanups;},[]{return false;}),
            "selected Machine module owns its normal session frame");
        Check(music.callbacks.size()-before<=1u,
            "session does not collapse ordinary machine source callbacks before or after a real SDK wait");

    }
    Check(!session.TimedScriptsActive() && session.TimedScriptsFinished()==1u && cleanups==1u &&
        music.callbacks==std::vector<std::string>({"pause","resume","pause","resume"}),
        "Machine external module finishes once with all normal source callbacks in order");
    Check(session.SelectedCompletedHandlers()==&primary_queue,"retired Machine does not leave a dangling completed queue");
    Check(music.callback_frames.size()==4u &&
        music.callback_frames[0]<music.callback_frames[1] &&
        music.callback_frames[1]<music.callback_frames[2] &&
        music.callback_frames[2]<music.callback_frames[3],
        "each normal Machine callback occurs in a distinct outer session invocation");
}

void VerifySelectedPushOrder(const std::filesystem::path& root) {
    using Status = pal4::NativeScriptFrameStatus;
    pal4::NativeNewGameScriptSession session(root, {});
    // Seed normal world-module loading, then replace only this isolated test's
    // SDK module with a bounded probe. Production still restores original CSB.
    session.LoadSceneInit("M11", "1");
    auto* sdk = session.Context().GetEngine();
    constexpr char scene_module[] = "gamedata\\editData\\script\\M11.csb";
    Check(sdk->Discard(scene_module) >= 0, "replace isolated scene scheduling probe");
    constexpr char scene_source[] =
        "void M11_1_init(){ global_mvar += 10; }\n"
        "void M11_2_init(){ global_mvar += 100; }\n";
    Check(sdk->AddScriptSection(scene_module, "scene_order.cs", scene_source, sizeof(scene_source)-1) >= 0 &&
              sdk->Build(scene_module) >= 0, "compile bounded scene ordering probe on real SDK");
    const auto external = [&](const char* module, const char* source) {
        Check(sdk->AddScriptSection(module, "external_order.cs", source, std::strlen(source)) >= 0 &&
                  sdk->Build(module) >= 0, "compile bounded external selected-stack probe");
        session.PushExternalStackModule(module, "Machine", {}, []{});
    };
    external("older_selected", "void Machine(){ global_mvar += 1; }");
    Check(!session.SceneInitHasPriority(), "older selected child is initial top");
    Check(session.QueueConcurrentSceneInit("M11", "1") && session.SceneInitHasPriority(),
          "scene init pushed during Loading preempts older selected child");
    session.BindQueuedSceneServices({});
    Check(!session.ProcessTimedScripts(0.017f, false, {}, []{}, []{return false;}) &&
              session.GlobalStoryVariable() == 0,
          "older selected child cannot consume a newer scene-init frame");
    external("newer_selected", "void Machine(){ global_mvar += 1000; }");
    Check(!session.SceneInitHasPriority() && session.TimedScriptsActive(),
          "later UI or timer child preempts scene init regardless of category");
    Check(session.TickConcurrentSceneOriginal(0.017f, nullptr, []{return false;}) == Status::queued &&
              session.GlobalStoryVariable() == 0,
          "scene runner cannot advance while a newer external top is selected");
    for (unsigned i=0; i<16u && !session.SceneInitHasPriority(); ++i)
        Check(session.ProcessTimedScripts(0.017f, false, {}, []{}, []{return false;}),
              "newest selected context owns its ordinary SDK source steps");
    Check(session.SceneInitHasPriority() && session.TimedScriptsActive() &&
              session.GlobalStoryVariable() == 1000,
          "finishing newest child restores scene above the still-active older child");
    // A nested scene push must restore its original sequence on pop, rather
    // than earning a fresh category priority over intervening external work.
    external("between_scenes", "void Machine(){ global_mvar += 10000; }");
    Check(session.QueueConcurrentSceneInit("M11", "2") && session.SceneInitHasPriority(),
          "new nested scene init wins over intervening external child");
    session.BindQueuedSceneServices({});
    for (unsigned i=0; i<16u && session.ConcurrentSceneStatus()!=Status::finished; ++i)
        (void)session.TickConcurrentSceneOriginal(0.017f, nullptr, []{return false;});
    Check(session.ConcurrentSceneStatus()==Status::finished && session.SceneInitHasPriority() &&
              session.GlobalStoryVariable()==1100,
          "finished nested scene remains selected until normal stack pop");
    Check(session.PopFinishedSceneInit() && !session.SceneInitHasPriority(),
          "pop restores prior scene push order below intervening external child");
    for (unsigned i=0; i<16u && !session.SceneInitHasPriority(); ++i)
        Check(session.ProcessTimedScripts(0.017f, false, {}, []{}, []{return false;}),
              "intervening external context resumes before older scene");
    Check(session.SceneInitHasPriority() && session.GlobalStoryVariable()==11100,
          "scene regains selection after newer external work retires");
    for (unsigned i=0; i<16u && session.ConcurrentSceneStatus()!=Status::finished; ++i)
        (void)session.TickConcurrentSceneOriginal(0.017f, nullptr, []{return false;});
    Check(!session.SceneInitHasPriority() && session.GlobalStoryVariable()==11110,
          "finished base scene does not permanently block underlying selected child");
    for (unsigned i=0; i<16u && session.TimedScriptsActive(); ++i)
        Check(session.ProcessTimedScripts(0.017f, false, {}, []{}, []{return false;}),
              "oldest child resumes after all newer scene and external work");
    Check(!session.TimedScriptsActive() && session.GlobalStoryVariable()==11111,
          "single original push order preserves every context exactly once");
}

void VerifyOriginalWorldMapScripts(const std::filesystem::path& root) {
    struct Buttons final : pal4::IWorldMapButtonStateRuntime,
                           pal4::IWorldMapExternalButtonRuntime {
        std::vector<std::string> names;
        std::vector<std::uint32_t> states;
        std::string selected;
        bool fail{};
        std::uint32_t FrameManager() override { return 0x100u; }
        std::uint32_t ReadWord(std::uint32_t address) override {
            if (address == 0x8b089cu) return 0x200u;
            if (address == 0x1040u) return 0x300u;
            std::uint32_t value{};
            std::memcpy(&value, reinterpret_cast<const void*>(address), sizeof(value));
            return value;
        }
        std::uint32_t FindFrame(std::uint32_t manager, std::uint32_t name) override {
            Check(manager == 0x100u && name == 0x200u, "original map callback finds actual map endpoint");
            if (fail) throw std::runtime_error("intentional WorldMap callback failure");
            return 0x1000u;
        }
        std::uint32_t MakeUppercaseName(std::uint32_t source) override {
            selected = reinterpret_cast<const char*>(source);
            names.push_back(selected);
            return 0x400u;
        }
        std::string_view NameText(std::uint32_t) override { return selected; }
        std::string_view RecordText(std::uint32_t) override { return selected; }
        void WriteWord(std::uint32_t address, std::uint32_t value) override {
            Check(address == 0x11e4u, "shared map setter writes matched record");
            states.push_back(value);
        }
        void SetVisible(std::uint32_t window, bool visible) override {
            Check(window == 0x300u && visible, "actual init enables its selected locations");
        }
        std::uint32_t CleanupName(std::uint32_t) override { return 0; }
    } buttons;
    std::int32_t story = 70500;
    pal4::NativeGameScriptServices services;
    services.world_map_buttons = &buttons;
    services.world_map_external = &buttons;
    pal4::NativeNewGameScriptSession session(root, services, &story);
    auto* sdk = session.Context().GetEngine();
    session.RunWorldMapInitializer(services);
    Check(buttons.names == std::vector<std::string>({"Q06", "M11", "Q05", "M09"}) &&
              buttons.states == std::vector<std::uint32_t>(4, 1u),
          "actual worldMap.csb init at70500 drives real SDK map visibility callbacks");
    Check(session.Status() == pal4::NativeScriptFrameStatus::idle && story == 70500 &&
              sdk->GetFunctionIDByName("init", "init") < 0,
          "synchronous map init retires temporary module without advancing story");
    buttons.fail = true;
    bool failed{};
    try { session.RunWorldMapInitializer(services); } catch (const std::runtime_error&) { failed = true; }
    Check(failed && sdk->GetFunctionIDByName("init", "init") < 0,
          "failing SDK map callback releases context and temporary module");
    buttons.fail = false;
    buttons.names.clear(); buttons.states.clear();
    session.RunWorldMapInitializer(services);
    Check(buttons.names.size() == 4u, "map initializer can run after failed callback cleanup");
    unsigned cleanups{};
    const auto missing = session.QueueWorldMapDestination("AbsentDestination", services, [&] { ++cleanups; });
    Check(!missing && cleanups == 0u && !session.TimedScriptsActive(),
          "absent original map destination does not preempt underlying story");
    const auto first = session.CreateWorldMapInterpreter("WorldMap", "M11");
    const auto duplicate = session.CreateWorldMapInterpreter("WorldMap", "M11");
    Check(session.AddWorldMapScriptModule(first) && !session.AddWorldMapScriptModule(duplicate),
          "second transient interpreter cannot replace a loaded map module");
    session.ReleaseWorldMapInterpreter(duplicate);
    Check(sdk->GetFunctionIDByName("M11", "M11") >= 0,
          "cleanup of AddModule-false interpreter preserves the loaded owner's module");
    session.ReleaseWorldMapInterpreter(first);
    Check(sdk->GetFunctionIDByName("M11", "M11") < 0,
          "cleanup releases unqueued successfully added map module");
    const auto failed_push = session.CreateWorldMapInterpreter("WorldMap", "M11");
    Check(session.AddWorldMapScriptModule(failed_push), "load map interpreter for failed preemption");
    bool push_failed{};
    try {
        (void)session.PushWorldMapInterpreter(failed_push, services, [] {
            throw std::runtime_error("intentional WorldMap preemption failure");
        });
    } catch (const std::runtime_error&) { push_failed = true; }
    session.ReleaseWorldMapInterpreter(failed_push);
    Check(push_failed && !session.TimedScriptsActive() &&
              sdk->GetFunctionIDByName("M11", "M11") < 0,
          "failed push cleanup releases interpreter without masking error or retaining SDK module");
    const auto interpreter = session.CreateWorldMapInterpreter("WorldMap", "M11");
    Check(session.WorldMapInterpreterFunctionName(interpreter) == "M11" &&
              session.WorldMapInterpreterFullPath(interpreter) == "M11" &&
              session.WorldMapScriptOwnerAddress() == reinterpret_cast<std::uintptr_t>(&session),
          "WorldMap interpreter owns original independently copied full-path/name and wrapper identity");
    Check(session.AddWorldMapScriptModule(interpreter) &&
              sdk->GetFunctionIDByName("M11", "M11") >= 0,
          "flight interpreter restores original worldMap.csb into actual destination module");
    Check(session.PushWorldMapInterpreter(interpreter, services, [&] { ++cleanups; }) &&
              cleanups == 1u && session.TimedScriptsStarted() == 1u &&
              story == 70500 && session.Status() == pal4::NativeScriptFrameStatus::idle,
          "flight pushes a prepared selected context without executing arrival callbacks");
    session.ReleaseWorldMapInterpreter(interpreter);
    Check(session.TimedScriptsActive() && sdk->GetFunctionIDByName("M11", "M11") >= 0,
          "interpreter release preserves SDK destination module owned by queued context");
    session.ReleaseServices();
    Check(!session.TimedScriptsActive() && sdk->GetFunctionIDByName("M11", "M11") < 0,
          "selected-stack teardown releases WorldMap arrival module");
}

void VerifyNestedOriginalEvfEvents(const std::filesystem::path& root) {
    using Status=pal4::NativeScriptFrameStatus;
    pal4::NativeNewGameScriptSession session(root,{});
    pal4::test::EventArenaRuntime arena;
    pal4::NativeGameScriptServices services{};services.arena=&arena;
    session.BindEventServices(services);
    unsigned displaced_cleanups{};
    session.SetCacheHandlerCleanup([&]{++displaced_cleanups;});
    session.QueueEventFunction("M11","func1001");
    auto* parent=&session.EventContext();
    session.QueueEventFunction("M11","func7002");
    Check(session.TimedScriptsActive() && displaced_cleanups==1u &&
          session.EventStatus()==Status::queued && &session.EventContext()==parent &&
          parent->GetState()==asEXECUTION_PREPARED && arena.requests==0,
          "actual overlapping M11 EVF pushes empty invalid-area child without replacing or executing exit parent");
    session.QueueEventFunction("m11","FUNC7002");
    session.QueueEventFunction("m11","FUNC1001");
    Check(displaced_cleanups==1u && session.TimedScriptsActive() &&
          parent->GetState()==asEXECUTION_PREPARED,
          "full-function duplicate check covers selected child and underlying queued event");
    for(unsigned n=0;n<16u && session.TimedScriptsActive();++n)
        Check(session.ProcessTimedScriptWaits(0.017f,{},[]{return false;},true),
              "nested EVF executes original CSB on normal selected-stack source steps");
    Check(!session.TimedScriptsActive() && session.EventStatus()==Status::queued &&
          &session.EventContext()==parent && arena.requests==0,
          "empty child retires naturally and restores exact unexecuted parent");
    Check(session.TickEventUntilBoundary(0.017f,nullptr,[&]{return arena.loading;})==Status::suspended &&
          arena.requests==1 && arena.world=="Q07" && arena.area=="Q07",
          "restored original M11 exit parent requests Q07 once");
    session.ReleaseServices();
}

void VerifyFirstEvfPreemptsSceneInit(const std::filesystem::path& root) {
    using Status=pal4::NativeScriptFrameStatus;
    pal4::NativeNewGameScriptSession session(root,{});
    session.LoadSceneInit("M11","1");
    auto* sdk=session.Context().GetEngine();
    constexpr char module[]="gamedata\\editData\\script\\M11.csb";
    Check(sdk->Discard(module)>=0,"replace isolated initializer scheduling probe");
    constexpr char source[]="void M11_1_init(){ global_mvar += 10; }\n";
    Check(sdk->AddScriptSection(module,"evf_scene_order.cs",source,sizeof(source)-1)>=0 &&
          sdk->Build(module)>=0,"compile bounded initializer on the actual SDK");
    Check(session.QueueConcurrentSceneInit("M11","1"),"prepare displaced scene initializer");
    session.BindQueuedSceneServices({});
    auto* parent=&session.ConcurrentSceneContext();
    Check(session.TickConcurrentSceneOriginal(0.017f,nullptr,[]{return false;})==Status::suspended,
          "initializer is suspended at a real SDK source boundary");
    const auto line=parent->GetCurrentLineNumber();
    const auto before=session.GlobalStoryVariable();
    unsigned cleanups{};
    session.SetCacheHandlerCleanup([&]{++cleanups;});
    Check(session.QueueEventFunction("M11","func7002") && cleanups==1u &&
          session.EventStatus()==Status::idle && session.TimedScriptsActive() &&
          !session.SceneInitHasPriority(),
          "first original-CSB EVF preempts a running scene even with no primary event");
    Check(!session.QueueEventFunction("m11","FUNC7002") && cleanups==1u,
          "duplicate first EVF retains selected context and displaced owner");
    Check(session.TickConcurrentSceneOriginal(0.017f,nullptr,[]{return false;})==Status::suspended &&
          &session.ConcurrentSceneContext()==parent && parent->GetCurrentLineNumber()==line &&
          session.GlobalStoryVariable()==before,
          "older initializer cannot step through a newer EVF");
    for(unsigned i=0;i<16u && session.TimedScriptsActive();++i)
        Check(session.ProcessTimedScriptWaits(0.017f,{},[]{return false;},true),
              "original empty EVF runs through the shared selected stack");
    Check(!session.TimedScriptsActive() && session.SceneInitHasPriority() &&
          &session.ConcurrentSceneContext()==parent && parent->GetCurrentLineNumber()==line &&
          session.GlobalStoryVariable()==before,
          "EVF retirement restores the exact unfinished initializer");
    for(unsigned i=0;i<16u && session.ConcurrentSceneStatus()!=Status::finished;++i)
        (void)session.TickConcurrentSceneOriginal(0.017f,nullptr,[]{return false;});
    Check(session.ConcurrentSceneStatus()==Status::finished && session.GlobalStoryVariable()==10,
          "restored initializer completes its real SDK statement exactly once");
}

void Run(const std::filesystem::path& root) {
    VerifySceneStack();
    VerifyNestedOriginalEvfEvents(root);
    VerifyFirstEvfPreemptsSceneInit(root);
    VerifySelectedPushOrder(root);
    VerifyOriginalWorldMapScripts(root);
    VerifyExternalMachineSourceSteps(root);
    {
        pal4::NativeNewGameScriptSession optional(root, {});
        optional.LoadSceneInit("M01", "2");
        Check(optional.SceneInitLoaded(), "real M01 scene 2 module loads without an init function");
        auto* context = &optional.Context();
        optional.StartSceneInit();
        Check(optional.Status() == pal4::NativeScriptFrameStatus::idle &&
                  &optional.Context() == context && optional.GlobalStoryVariable() == 0 &&
                  !optional.QueueConcurrentSceneInit("M01", "2") &&
                  optional.ConcurrentSceneStatus() == pal4::NativeScriptFrameStatus::idle &&
                  optional.ConcurrentSceneDepth() == 1u,
              "real missing M01_2_init neither starts nor pushes a fabricated script");
    }
    pal4::NativeNewGameScriptSession session(root, {});
    using Status = pal4::NativeScriptFrameStatus;
    Check(session.Status() == Status::idle &&
              session.Tick(0.016f) == Status::idle,
          "original NewGame module stays idle before menu action");
    bool rejected_unqueued_services = false;
    try { session.BindQueuedSceneServices({}); }
    catch (const std::logic_error&) { rejected_unqueued_services = true; }
    Check(rejected_unqueued_services,
          "scene owner binding requires an actual prepared function");
    session.BindServices({});
    session.Start();
    Check(session.Status() == Status::queued,
          "menu action queues original NewGame bytecode");
    bool rejected_scene_load = false;
    try {
        session.LoadSceneInit("Q01", "N01");
    } catch (const std::logic_error&) {
        rejected_scene_load = true;
    }
    Check(rejected_scene_load && !session.SceneInitLoaded(),
          "active NewGame keeps scene bytecode outside its execution context");
    session.QueueConcurrentSceneInit("Q01", "Q01");
    session.BindQueuedSceneServices({});
    Check(session.ConcurrentSceneStatus() == Status::queued &&
              session.Status() == Status::queued &&
              session.ConcurrentSceneContext().GetState() == asEXECUTION_PREPARED &&
              &session.ConcurrentSceneContext() != &session.Context(),
          "scene init queues beside an active NewGame context");
    // Isolate the primary-event API from the selected scene-stack probe.
    // First EVFs during scene initialization are covered above through the
    // actual shared-stack preemption path.
    pal4::NativeNewGameScriptSession primary_event(root, {});
    primary_event.Start();
    primary_event.QueueEventFunction("Q01", "func1002");
    auto* queued_event_context = &primary_event.EventContext();
    primary_event.QueueEventFunction("q01", "FUNC1002");
    Check(&primary_event.EventContext() == queued_event_context &&
              primary_event.EventStatus() == Status::queued &&
              queued_event_context->GetState() == asEXECUTION_PREPARED,
          "same full EVF function is ignored case-insensitively without replacing its prepared context");
    Check(primary_event.EventStatus() == Status::queued &&
              primary_event.Status() == Status::queued,
          "original Q01 event function queues beside NewGame");
    const auto event_status = primary_event.TickEvent(0.0f);
    Check(event_status == Status::failed &&
              primary_event.EventError() ==
                  "PAL4 game script service -65 is unavailable" &&
              primary_event.Status() == Status::queued,
          "Q01 event enters giArenaLoad in an independent script context");
    Check(session.TickConcurrentSceneInit(0.0f) == Status::failed &&
              session.ConcurrentSceneError() ==
                  "PAL4 game script service -97 is unavailable" &&
              session.Status() == Status::queued,
          "Q01/Q01 scene init reaches visibility without consuming NewGame");
    bool rejected_executed_services = false;
    try { session.BindQueuedSceneServices({}); }
    catch (const std::logic_error&) { rejected_executed_services = true; }
    Check(rejected_executed_services && session.Status() == Status::queued,
          "scene rebinding cannot restart an executed function or consume NewGame");
    pal4::NativeNewGameScriptSession event_switch(root, {});
    pal4::test::EventArenaRuntime event_arena;
    pal4::NativeGameScriptServices event_services{};
    event_services.arena = &event_arena;
    event_switch.BindEventServices(event_services);
    event_switch.QueueEventFunction("Q01", "func1002");
    const auto event_boundary = [&event_arena] { return event_arena.loading; };
    Check(event_switch.TickEventUntilBoundary(
              0.0f, nullptr, event_boundary) == Status::suspended &&
              event_switch.EventError().empty() &&
              event_arena.requests == 1 && event_arena.world == "Q01" &&
              event_arena.area == "Q01",
          "original event requests Q01/Q01 and yields at Loading");
    const auto suspended_line = event_switch.EventContext().GetCurrentLineNumber();
    auto* suspended_event = &event_switch.EventContext();
    event_switch.QueueEventFunction("q01", "FUNC1002");
    Check(event_switch.EventStatus() == Status::suspended &&
              &event_switch.EventContext() == suspended_event &&
              suspended_event->GetCurrentLineNumber() == suspended_line &&
              event_arena.requests == 1 && event_switch.EventError().empty(),
          "duplicate EVF contact preserves the suspended Loading statement and does not request a second load");
    event_switch.QueueConcurrentSceneInit("Q01", "Q01");
    event_switch.CancelConcurrentSceneInit();
    Check(event_switch.ConcurrentSceneStatus() == Status::failed &&
              event_switch.EventStatus() == Status::suspended &&
              event_switch.EventError().empty(),
          "cancelling scene init preserves the suspended event context");
    event_switch.DetachEventServices();
    event_switch.RebindSuspendedEventServices({});
    Check(event_switch.TickEventUntilBoundary(
              0.0f, nullptr, [] { return false; }) == Status::failed &&
              event_switch.EventError() !=
                  "PAL4 game script service -65 is unavailable" &&
              event_arena.requests == 1,
          "rebound event resumes after giArenaLoad without repeating it");
    pal4::NativeNewGameScriptSession discarded_event(root, {});
    discarded_event.QueueEventFunction("Q01", "func1002");
    discarded_event.CancelQueuedEvent();
    Check(discarded_event.EventStatus() == Status::failed &&
              discarded_event.EventError() ==
                  "PAL4 script execution cancelled" &&
              discarded_event.TickEvent(0.0f) == Status::failed,
          "scene switch discards an event queued by the old scene");
    bool rejected_rebind = false;
    try {
        session.BindServices({});
    } catch (const std::logic_error&) {
        rejected_rebind = true;
    }
    Check(rejected_rebind,
          "queued NewGame cannot replace callbacks before its frame tick");
    Check(session.Tick(0.016f) == Status::failed &&
              session.Error() == "PAL4 game script service -291 is unavailable",
          "first game frame reports the missing flash runtime");
    rejected_rebind = false;
    try {
        session.BindServices({});
    } catch (const std::logic_error&) {
        rejected_rebind = true;
    }
    Check(rejected_rebind,
          "failed NewGame cannot replace callbacks after execution");
    Check(session.Tick(0.016f) == Status::failed,
          "failed game script is not restarted on later frames");
    session.QueueEventFunction("Q01", "func1002");
    session.ReleaseServices();
    Check(session.Status() == Status::failed &&
              session.Error() == "PAL4 game script service -291 is unavailable" &&
              session.EventStatus() == Status::failed &&
              session.EventError() == "PAL4 script execution cancelled",
          "service release preserves story failure and cancels queued event");
    session.Start();
    session.ReleaseServices();
    Check(session.Status() == Status::failed &&
              session.Error() == "PAL4 script execution cancelled" &&
              session.Tick(0.016f) == Status::failed,
          "closing before the next frame cancels queued NewGame");
    Check(!session.SceneInitLoaded(),
          "scene init bytecode is absent before the arena request");
    Check(session.GlobalStoryVariable() == 0,
          "failed NewGame leaves the story variable at its initial value");
    session.LoadSceneInit("Q01", "N01");
    Check(session.SceneInitLoaded(),
          "original Q01.csb provides the Q01_N01_init entry");
    session.StartSceneInit();
    Check(session.Status() == Status::queued,
          "scene init queues original Q01/N01 bytecode");
    const auto scene_status = session.Tick(0.016f);
    if (scene_status != Status::failed ||
        session.Error() != "PAL4 game script service -67 is unavailable")
        std::cerr << "scene init status=" << static_cast<int>(scene_status)
                  << " error=" << session.Error() << '\n';
    Check(scene_status == Status::failed &&
              session.Error() ==
                  "PAL4 game script service -67 is unavailable",
          "zero story variable takes the original scene restore branch");

    std::int32_t restored_story = 70201;
    pal4::NativeNewGameScriptSession restored(root, {}, &restored_story);
    restored.LoadSceneInit("Q01", "N01");
    restored.Start();
    restored.BindEventServices(event_services);
    event_arena.loading = false;
    restored.QueueEventFunction("Q01", "func1002");
    Check(restored.TickEventUntilBoundary(0.0f, nullptr, event_boundary) ==
              Status::suspended, "save cleanup starts with a suspended old event");
    restored.QueueConcurrentSceneInit("Q01", "Q01");
    bool missing_cleanup_rejected = false;
    try { restored.ClearExecutionStack({}); }
    catch (const std::invalid_argument&) { missing_cleanup_rejected = true; }
    Check(missing_cleanup_rejected && restored.Status() == Status::queued &&
              restored.EventStatus() == Status::suspended,
          "handler cleanup must be supplied before changing any execution state");
    auto* original_story_context = &restored.Context();
    auto* original_event_context = &restored.EventContext();
    int handler_cleanups = 0;
    restored.ClearExecutionStack([&] {
        Check(restored.Status() == Status::idle &&
                  restored.ConcurrentSceneStatus() == Status::idle &&
                  restored.EventStatus() == Status::idle,
              "all execution stacks stop before handler cleanup");
        ++handler_cleanups;
    });
    Check(handler_cleanups == 1 && restored.SceneInitLoaded() &&
              restored.GlobalStoryVariable() == 70201 && restored_story == 70201 &&
              &restored.Context() == original_story_context &&
              &restored.EventContext() == original_event_context &&
              restored.Tick(0.0f) == Status::idle &&
              restored.TickEvent(0.0f) == Status::idle && restored.Error().empty(),
          "load cleanup retains modules, shared globals and borrowed context identities");
    restored.BindServices({});
    restored.StartSceneInit();
    Check(restored.Status() == Status::queued,
          "retained scene entry queues again after execution cleanup");
    restored.ClearExecutionStack([&] { ++handler_cleanups; });
    event_arena.loading = false;
    restored.QueueEventFunction("Q01", "func1002");
    const auto requests_before_reload = event_arena.requests;
    Check(restored.TickEventUntilBoundary(0.0f, nullptr, event_boundary) ==
              Status::suspended && event_arena.requests == requests_before_reload + 1,
          "retained world bytecode and callback services restart an event after cleanup");
    restored.ClearExecutionStack([&] { ++handler_cleanups; });

    pal4::NativeNewGameScriptSession stepped(root, {});
    stepped.Start();
    int steps = 0;
    auto stepped_status = Status::queued;
    while ((stepped_status == Status::queued ||
            stepped_status == Status::suspended) && steps < 256) {
        stepped_status = stepped.TickOriginal(0.016f);
        ++steps;
    }
    Check(steps > 1 && stepped_status == Status::failed &&
              stepped.Error() ==
                  "PAL4 game script service -291 is unavailable",
          "original NewGame yields by source line before the first callback");

    pal4::NativeNewGameScriptSession switching(root, {});
    switching.Start();
    Check(switching.TickOriginal(0.0f) == Status::suspended,
          "first source step leaves a live script context");
    (void)pal4::ScriptCommandDispatcher::Push(201u, 3u, switching);
    (void)pal4::ScriptCommandDispatcher::Push(202u, 4u, switching);
    switching.RandomTalkPool().Push(std::string(1024u,'x'));
    switching.RandomTalkPool().Push("next scene candidate");
    const auto dialogue_capacity = switching.RandomTalkPool().Capacity();
    const auto command_capacity = switching.ScriptExecutorQueue().Capacity();
    auto* suspended_context = &switching.Context();
    const auto story_before_reset = switching.GlobalStoryVariable();
    switching.ResetSceneQueues();
    switching.ResetSceneQueues();
    Check(switching.ScriptExecutorQueue().Commands().empty() &&
              switching.RandomTalkPool().Size()==0u &&
              switching.RandomTalkPool().Capacity()==dialogue_capacity &&
              switching.ScriptExecutorQueue().Capacity() == command_capacity &&
              &switching.Context() == suspended_context &&
              switching.Status() == Status::suspended &&
              switching.GlobalStoryVariable() == story_before_reset,
          "scene command reset retains capacity, suspended SDK context and story");
    (void)pal4::ScriptCommandDispatcher::Push(203u, 5u, switching);
    Check(switching.ScriptExecutorQueue().Commands().size() == 1u &&
              switching.ScriptExecutorQueue().Commands()[0].command == 203u &&
              switching.ScriptExecutorQueue().Capacity() == command_capacity,
          "scene command queue accepts new additions after repeated reset");
    switching.ResetSceneQueues();
    switching.DetachSceneServices();
    Check(switching.Status() == Status::suspended &&
              switching.Error().empty(),
          "scene release preserves the suspended script context");
    switching.RebindSuspendedServices({});
    auto resumed = Status::suspended;
    int resumed_steps = 0;
    while (resumed == Status::suspended && resumed_steps < 256) {
        resumed = switching.TickOriginal(0.0f);
        ++resumed_steps;
    }
    Check(resumed == Status::failed &&
              switching.Error() ==
                  "PAL4 game script service -291 is unavailable" &&
              resumed_steps > 0,
          "rebound script continues at its original callback");
}

}  // namespace

int main(int argc, char** argv) {
    try {
        Check(argc == 2, "provide original PAL4 installation path");
        {
            pal4::NativeAngelScriptEngine engine;
            pal4::NativeGameArenaManagerOwner arena;
            const auto image=arena.Construct();
            char world[]="Q01";
            auto* words=reinterpret_cast<std::uint32_t*>(image);
            words[2]=reinterpret_cast<std::uint32_t>(world);
            arena.SetSceneName("N01");
            pal4::NativeScriptFrameRunner runner(engine);
            Check(arena.QueueSceneScript([&](const auto& request){
                Check(arena.SceneScriptSource()==request.file_path && words[236u/4u]==0u,
                    "arena publishes original virtual source before real SDK preparation");
                pal4::LoadNativeScriptModule(engine,argv[1],request.file_path,request.function_path);
                Check(engine.FunctionId(request.function_path.c_str(),request.function_name.c_str())>=0,
                    "arena virtual source resolves real original scene bytecode and function");
                runner.Queue(request.function_path.c_str(),request.function_name.c_str());
                return true;
            }) && words[236u/4u]==1u && runner.Status()==pal4::NativeScriptFrameStatus::queued &&
                engine.GlobalStoryVariable()==0,
                "original scene script is prepared without executing inside Loading");
            words[2]=0u;
            std::cout<<"PASS: owned arena virtual source prepares original Q01 SDK context without execution\n";
        }
        Run(argv[1]);
        std::cout << "PASS: original NewGame session starts on a frame\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
