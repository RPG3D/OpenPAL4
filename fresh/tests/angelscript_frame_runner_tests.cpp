#include "pal4/native_script_frame_runner.h"
#include "pal4/native_game_script_callbacks.h"

#include <angelscript.h>

#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

using Status = pal4::NativeScriptFrameStatus;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Callbacks final : public pal4::IGameScriptCallbackRuntime {
public:
    bool suspend_first = true;
    bool fail = false;
    int calls = 0;

    void Invoke(int system_id, asIScriptGeneric&) override {
        Check(system_id == -57, "script callback ID");
        ++calls;
        if (fail) {
            (void)asGetActiveContext()->SetException("callback failed");
        } else if (suspend_first && calls == 1) {
            Check(asGetActiveContext()->Suspend() >= 0,
                  "suspend script context");
        }
    }
};

class Waits final : public pal4::INativeScriptFrameWaitRuntime {
public:
    bool ready = false;
    int ticks = 0;
    bool Tick(float elapsed) override {
        Check(elapsed == 0.25f, "wait receives frame time");
        ++ticks;
        return ready;
    }
};

void VerifyImmediateEntryPolicy() {
    pal4::NativeAngelScriptEngine engine;
    constexpr char source[] =
        "void EnterImm() {\n"
        " giIMMBegin();\n"
        " giNewGame();\n"
        " giNewGame();\n"
        " giIMMEnd();\n"
        " giNewGame();\n"
        "}\n";
    Check(engine.Raw()->AddScriptSection("imm_entry", "imm_entry.cs", source, sizeof(source)-1)>=0 &&
        engine.Raw()->Build("imm_entry")>=0,"compile actual SDK IMM entry regression");
    class ImmediateCallbacks final : public pal4::IGameScriptCallbackRuntime {
    public:
        pal4::NativeGameScriptCallbacks immediate{pal4::NativeGameScriptServices{}};
        std::vector<int> calls;
        void Invoke(int id,asIScriptGeneric& arguments) override {
            calls.push_back(id);
            if(id==-55 || id==-56)immediate.Invoke(id,arguments);
            else Check(id==-57,"IMM regression uses explicit marker callback");
        }
    } callbacks;
    engine.SetCallbacks(&callbacks);
    pal4::NativeScriptFrameRunner runner(engine);
    runner.SetImmediateModeProvider([&]{return callbacks.immediate.ImmediateMode()==1u;});
    runner.Queue("imm_entry","EnterImm");
    // SDK source stepping may first stop at a function/line boundary. Observe
    // the invocation that actually enters IMM rather than assuming its index.
    unsigned frames{};
    while(callbacks.calls.empty() && frames++<16u)
        Check(runner.TickOriginal(.017f)==Status::suspended,"normal SDK step reaches IMMBegin");
    Check(callbacks.calls==std::vector<int>{-55} && callbacks.immediate.ImmediateMode()==1u,
        "entry IMM0 executes IMMBegin only, without falling through into its block");
    Check(runner.TickOriginal(.017f)==Status::suspended &&
        callbacks.calls==std::vector<int>({-55,-57,-57,-56}) && callbacks.immediate.ImmediateMode()==0u,
        "next frame entry IMM1 executes the block and stops at IMMEnd before following marker");
    frames=0;
    while(runner.Status()!=Status::finished && frames++<16u) {
        const auto previous=callbacks.calls.size();
        Check(runner.TickOriginal(.017f)!=Status::failed && callbacks.calls.size()-previous<=1u,
            "after IMMEnd normal source stepping remains bounded");
    }
    Check(runner.Status()==Status::finished && callbacks.calls==std::vector<int>({-55,-57,-57,-56,-57}),
        "post IMMEnd marker executes only in a later normal frame");
    engine.SetCallbacks(nullptr);
}

void Run() {
    pal4::NativeAngelScriptEngine engine;
    constexpr char source[] =
        "void NewGame() {\n giNewGame();\n giNewGame();\n}\n";
    Check(engine.Raw()->AddScriptSection("new_game", "new_game.cs",
                                         source, sizeof(source) - 1) >= 0 &&
              engine.Raw()->Build("new_game") >= 0,
          "compile frame-driven script");
    Callbacks callbacks;
    engine.SetCallbacks(&callbacks);
    pal4::NativeScriptFrameRunner runner(engine);
    Waits waits;
    Check(runner.Status() == Status::idle && callbacks.calls == 0,
          "new runner is idle");
    runner.Queue("new_game", "NewGame");
    Check(runner.Status() == Status::queued && callbacks.calls == 0,
          "queue does not execute script synchronously");
    Check(runner.Tick(0.25f, &waits) == Status::suspended &&
              callbacks.calls == 1 && waits.ticks == 1,
          "first frame executes until callback wait");
    Check(runner.Tick(0.25f, &waits) == Status::suspended &&
              callbacks.calls == 1 && waits.ticks == 2,
          "pending wait does not reenter script");
    waits.ready = true;
    Check(runner.Tick(0.25f, &waits) == Status::finished &&
              callbacks.calls == 2 && waits.ticks == 3,
          "closed wait resumes script on a later frame");
    Check(runner.Tick(0.25f, &waits) == Status::finished &&
              waits.ticks == 4,
          "wait handlers continue ticking after the script finishes");

    callbacks.fail = true;
    runner.Queue("new_game", "NewGame");
    Check(runner.Tick(0.25f) == Status::failed &&
              runner.Error() == "callback failed" && callbacks.calls == 3,
          "script callback failure is reported");
    Check(runner.Tick(0.25f) == Status::failed && callbacks.calls == 3,
          "failed script is not retried on every frame");
    callbacks.fail = false;
    callbacks.calls = 0;
    runner.Queue("new_game", "NewGame");
    Check(runner.Tick(0.25f, &waits) == Status::suspended &&
              callbacks.calls == 1,
          "new script may suspend after a failed invocation");
    runner.Cancel();
    Check(runner.Status() == Status::failed &&
              runner.Error() == "PAL4 script execution cancelled" &&
              runner.Tick(0.25f) == Status::failed && callbacks.calls == 1,
          "cancelled suspended script does not invoke callbacks again");
    try {
        runner.Queue("new_game", "MissingFunction");
        throw std::runtime_error("missing function was accepted");
    } catch (const std::runtime_error& error) {
        Check(std::string_view(error.what()) ==
                  "PAL4 script function is unavailable",
              "missing function remains explicit");
    }
    callbacks.fail = false;
    callbacks.suspend_first = false;
    callbacks.calls = 0;
    runner.SetImmediateModeProvider([] { return true; });
    runner.Queue("new_game", "NewGame");
    const auto ticks_before_boundary = waits.ticks;
    Check(runner.TickUntilBoundary(0.25f, &waits,
                                  [&] { return callbacks.calls == 1; }) ==
                  Status::suspended &&
              callbacks.calls == 1 &&
              waits.ticks == ticks_before_boundary + 1,
          "immediate script stops at a state boundary before next callback");
    Check(runner.TickUntilBoundary(0.25f, nullptr,
                                  [] { return false; }) == Status::finished &&
              callbacks.calls == 2,
          "script resumes after the state boundary is handled");
    callbacks.calls = 0;
    runner.SetImmediateModeProvider([] { return false; });
    runner.Queue("new_game", "NewGame");
    int frames = 0;
    while (runner.Status() != Status::finished && frames < 16) {
        const auto previous_calls = callbacks.calls;
        Check(runner.TickOriginal(0.25f) != Status::failed &&
                  callbacks.calls - previous_calls <= 1,
              "normal frame cannot collapse distinct source callbacks into one update");
        ++frames;
    }
    Check(runner.Status() == Status::finished && callbacks.calls == 2 && frames >= 2,
          "normal source steps finish on separate frames without artificial waits");
    callbacks.calls = 0;
    runner.SetImmediateModeProvider([] { return true; });
    runner.Queue("new_game", "NewGame");
    Check(runner.TickOriginal(0.25f) == Status::finished && callbacks.calls == 2,
          "same SDK script completes its source steps in one original IMM update");
    engine.SetCallbacks(nullptr);
}

}  // namespace

int main() {
    try {
        Run();
        VerifyImmediateEntryPolicy();
        std::cout << "PASS: PAL4 script frame scheduling\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
