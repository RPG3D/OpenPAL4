#include "pal4/native_angelscript_engine.h"
#include "pal4/native_game_script_callbacks.h"
#include "pal4/native_script_frame_runner.h"
#include "pal4/native_script_timed_wait_runtime.h"

#include <iostream>
#include <stdexcept>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

}  // namespace

int main() {
    try {
        pal4::NativeAngelScriptEngine engine;
        constexpr char source[] =
            "void Wait() { giWait(1.5); }";
        Check(engine.Raw()->AddScriptSection(
                  "timed_wait", "timed_wait.cs", source,
                  sizeof(source) - 1u) >= 0 &&
                  engine.Raw()->Build("timed_wait") >= 0,
              "compile original giWait callback signature");
        pal4::NativeScriptFrameRunner runner(engine);
        pal4::NativeScriptTimedWaitRuntime wait(runner.Context());
        pal4::NativeGameScriptServices services{};
        services.wait = &wait;
        pal4::NativeGameScriptCallbacks callbacks(services);
        engine.SetCallbacks(&callbacks);
        runner.Queue("timed_wait", "Wait");
        Check(runner.Tick(0.0f, &wait) ==
                  pal4::NativeScriptFrameStatus::suspended &&
                  wait.Waiting() && wait.Remaining() == 1.5,
              "giWait registers and suspends its original 1.5-second handler");
        Check(runner.Tick(1.0f, &wait) ==
                  pal4::NativeScriptFrameStatus::suspended &&
                  wait.Waiting() && wait.Remaining() == 0.5,
              "giWait stays suspended before its countdown ends");
        Check(runner.Tick(0.5f, &wait) ==
                  pal4::NativeScriptFrameStatus::finished &&
                  !wait.Waiting() && wait.Remaining() == 0.0,
              "giWait closes and resumes the same script context");
        auto* original_context = &runner.Context();
        runner.Queue("timed_wait", "Wait");
        Check(runner.Tick(0.0f, &wait) == pal4::NativeScriptFrameStatus::suspended &&
                  wait.Waiting(), "prepare a live wait for load cleanup");
        runner.ClearExecution();
        wait.ClearHandlers();
        Check(&runner.Context() == original_context &&
                  runner.Status() == pal4::NativeScriptFrameStatus::idle &&
                  runner.Error().empty() && !wait.Waiting() &&
                  wait.Remaining() == 0.0 && !wait.Tick(10.0f) &&
                  runner.Tick(0.0f) == pal4::NativeScriptFrameStatus::idle,
              "cleanup retains the borrowed context and removes the old countdown");
        runner.Queue("timed_wait", "Wait");
        Check(runner.Tick(0.0f, &wait) == pal4::NativeScriptFrameStatus::suspended &&
                  wait.Remaining() == 1.5 &&
                  runner.Tick(1.5f, &wait) == pal4::NativeScriptFrameStatus::finished,
              "same loaded module and wait service execute afresh after cleanup");
        runner.ClearExecution();
        wait.ClearHandlers();
        runner.ClearExecution();
        Check(runner.Status() == pal4::NativeScriptFrameStatus::idle,
              "repeated cleanup also accepts finished and idle contexts");
        runner.Queue("timed_wait", "Wait");
        for (unsigned step = 0; step < 64u && !wait.Waiting(); ++step)
            (void)runner.TickOriginal(0.0f, &wait);
        Check(wait.BlocksScript() && wait.Remaining() == 1.5,
              "original single-step runner reaches blocking timed wait");
        for (unsigned step = 0; step < 64u; ++step)
            Check(runner.TickOriginal(0.0f, &wait) == pal4::NativeScriptFrameStatus::suspended &&
                      wait.Remaining() == 1.5, "single-step VM must not bypass an unexpired wait");
        (void)runner.TickOriginal(1.5f, &wait);
        for (unsigned step = 0; step < 64u &&
                runner.Status() != pal4::NativeScriptFrameStatus::finished; ++step)
            (void)runner.TickOriginal(0.0f, &wait);
        Check(!wait.BlocksScript() && runner.Status() == pal4::NativeScriptFrameStatus::finished,
              "original runner resumes only after timed wait expires");
        std::cout << "PASS: native giWait frame runtime\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
