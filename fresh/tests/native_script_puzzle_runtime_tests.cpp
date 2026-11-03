#include "pal4/native_script_puzzle_runtime.h"
#include "pal4/native_angelscript_engine.h"
#include "pal4/native_game_script_callbacks.h"
#include "pal4/native_script_frame_runner.h"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <utility>

namespace {
void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
}
int main() {
    try {
        pal4::NativeAngelScriptEngine engine;
        const char source[] =
            "void Puzzle(){global_mvar=1;giStartPuzzleGame(0);global_mvar=10+giGetPuzzleGameResult();}"
            "void Jigsaw(){global_mvar=2;giStartJigsawGame(3);global_mvar=20+giGetPuzzleGameResult();}";
        Check(engine.Raw()->AddScriptSection("puzzle_wait", "puzzle_wait.cs", source, sizeof(source)-1u)>=0 &&
            engine.Raw()->Build("puzzle_wait")>=0, "compile actual puzzle callback signatures");
        pal4::NativeScriptFrameRunner first(engine), second(engine);
        std::uint32_t current = 1u;
        std::uint8_t result = 255u;
        unsigned clears{}, queries{};
        bool fail{};
        std::vector<std::pair<int,int>> entries;
        pal4::NativeScriptPuzzleServices endpoints{
            0x123400u,
            [&]{++clears; result=0u;},
            [&]{return result;},
            [&](int mode,int type){
                Check(result==0u,"previous result is cleared before state entry");
                if(fail)throw std::runtime_error("actual puzzle entry failed");
                entries.emplace_back(mode,type);current=8u;
            },
            [&]{return current;},
            [&](auto state){Check(state!=0u,"absent state must short circuit");++queries;return static_cast<int>(state);}
        };
        pal4::NativeScriptPuzzleRuntime wait(first.Context(), endpoints), other(second.Context(), endpoints);
        bool rejected{};
        try { pal4::NativeScriptPuzzleRuntime missing(first.Context(), {}); }
        catch(const std::invalid_argument&) { rejected=true; }
        Check(rejected,"reject missing live UI/state endpoints");
        rejected=false;
        try {(void)pal4::ScriptPuzzleGame(wait).StartPuzzle(0);}
        catch(const std::logic_error&) { rejected=true; }
        Check(rejected && clears==0u,"foreign context rejected before result mutation");
        pal4::NativeGameScriptServices services;
        services.puzzle=&wait;services.puzzle_result=&wait;
        pal4::NativeGameScriptCallbacks callbacks(services);engine.SetCallbacks(&callbacks);
        first.Queue("puzzle_wait","Puzzle");
        Check(first.Tick(0.f,&wait)==pal4::NativeScriptFrameStatus::suspended && wait.BlocksScript() &&
            !wait.HasDocument() && engine.GlobalStoryVariable()==1 &&
            entries==std::vector<std::pair<int,int>>{{0,0}},
            "-300 enters Mode0/Type0, destroys temporary document, suspends exact SDK context");
        result=1u;
        for(auto state:{8u,4u,5u,2u,0xffffffffu}) {
            current=state;
            Check(first.Tick(1.f,&wait)==pal4::NativeScriptFrameStatus::suspended &&
                engine.GlobalStoryVariable()==1,"success flag or foreign state does not complete puzzle wait");
        }
        second.Queue("puzzle_wait","Puzzle");
        const auto clear_count=clears;
        Check(second.Tick(0.f,&other)==pal4::NativeScriptFrameStatus::failed && clears==clear_count &&
            wait.BlocksScript(),"another SDK context cannot borrow active puzzle service or clear global result");
        second.ClearExecution();
        current=1u;
        Check(first.Tick(0.f,&wait)==pal4::NativeScriptFrameStatus::finished &&
            engine.GlobalStoryVariable()==11 && !wait.BlocksScript(),
            "Scene1 resumes script and -131 returns actual successful result byte");
        services.puzzle=&other;services.puzzle_result=&other;callbacks.SetServices(services);
        second.Queue("puzzle_wait","Jigsaw");
        Check(second.Tick(0.f,&other)==pal4::NativeScriptFrameStatus::suspended &&
            entries.back()==std::pair<int,int>{1,3} && result==0u && !other.HasDocument(),
            "-301 uses same shared entry with Mode1/Type3 and clears previous result");
        current=0u;const auto before=queries;
        Check(second.Tick(0.f,&other)==pal4::NativeScriptFrameStatus::finished &&
            engine.GlobalStoryVariable()==20 && queries==before,
            "absent state resumes without querying vtable and returns failure result0");
        second.Queue("puzzle_wait","Puzzle");
        Check(second.Tick(0.f,&other)==pal4::NativeScriptFrameStatus::suspended,"prepare cancellation");
        second.ClearExecution();other.ClearHandlers();
        Check(!other.BlocksScript() && !other.HasDocument() && current==8u && !other.Tick(0.f),
            "retiring context does not close unrelated global puzzle or leave stale wait/document");
        fail=true;second.Queue("puzzle_wait","Puzzle");
        Check(second.Tick(0.f,&other)==pal4::NativeScriptFrameStatus::failed &&
            !other.BlocksScript() && !other.HasDocument(),
            "entry exception cleans temporary document without publishing active wait");
        second.ClearExecution();other.ClearHandlers();fail=false;
        second.Queue("puzzle_wait","Puzzle");
        Check(second.Tick(0.f,&other)==pal4::NativeScriptFrameStatus::suspended,
            "same context can retry real puzzle entry after failure");
        second.ClearExecution();other.ClearHandlers();
        std::cout<<"PASS: puzzle/jigsaw SDK callbacks, exact Mode/Type, live result, Scene-only wait, owner rejection, cleanup and retry\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
}
