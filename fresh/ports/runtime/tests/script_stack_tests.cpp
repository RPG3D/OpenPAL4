#include "pal4/portable/script_stack.h"
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace pal4::portable;
void Check(bool condition,const char* message) {if(!condition)throw std::runtime_error(message);}
void Reject(const std::function<void()>& operation) {
    bool rejected=false;try{operation();}catch(const std::logic_error&){rejected=true;}Check(rejected,"unsafe stack operation accepted");
}
struct Host:IScriptContextServices {
    std::vector<ScriptToken> requests;
    ScriptResponse Invoke(const ScriptRequest& request) override {requests.push_back(request.token);return {{},true};}
};
int main() {
    try {
        ScriptGlobals globals{};Host host;ScriptStack stack(globals);
        stack.Compile("test",R"(
            int runs=0;
            void Outer(){runs++;global_mvar=1;giArenaLoad("Q01","N01","",true);global_mvar+=runs*100;giPlayMovie("x");global_mvar+=1000;}
            void Inner(){runs++;global_mvar+=10;giWait(0.5);global_mvar+=20;}
            void Zero(){giWait(0);global_mvar++;giWait(-1);global_mvar++;}
            void Loop(){while(true){global_mvar++;}}
            void Fail(){int a=0;int b=1/a;global_mvar=b;}
        )");
        Check(!stack.Push("test","void Missing()",host) && stack.Depth()==0,"missing init changed stack");
        const auto outer=stack.Push("test","void Outer()",host);
        Check(outer && stack.Tick(0)==ScriptState::suspended && globals[0]==1,"outer did not suspend at load");
        Check(stack.ContainsActive("TEST","void OUTER()") && !stack.ContainsActive("test","void Missing()"),"active module query mismatch");
        const auto loading=*stack.Pending();
        Check(stack.Tick(100)==ScriptState::suspended && globals[0]==1,"external service resumed before completion");
        Reject([&]{stack.PopFinished();});
        Reject([&]{stack.Compile("test","void Other(){}");});
        stack.Compile("destination","void Init(){}");
        const auto inner=stack.Push("test","void Inner()",host);
        Check(inner && stack.Depth()==2 && stack.TopId()!=*outer,"nested context not created");
        Check(!stack.Push("TEST","void OUTER()",host) && stack.Depth()==2,"active lower duplicate accepted");
        Check(stack.Tick(0)==ScriptState::suspended && stack.WaitKind()==ScriptWaitKind::delay && globals[0]==11,"real giWait did not suspend");
        Check(!stack.Complete(*stack.Pending()),"external completion bypassed a time wait");
        Check(stack.Complete(loading),"lower scene-load completion lost");
        Check(stack.Tick(.25)==ScriptState::suspended && globals[0]==11,"delay finished early");
        Check(stack.Tick(.25)==ScriptState::finished && globals[0]==31,"delay did not finish at duration");
        Check(stack.PopFinished() && stack.TopId()==*outer,"caller context identity lost");
        Check(stack.Tick(0)==ScriptState::suspended && globals[0]==231,"caller restarted or module state was copied");
        const auto movie=*stack.Pending();
        Check(movie.context==*outer && movie.sequence>loading.sequence,"request token not advanced");
        Check(!stack.Complete(loading) && !stack.Complete(loading),"stale load completion released movie wait");
        Check(stack.Complete(movie) && !stack.Complete(movie),"completion was not consumed once");
        Check(stack.Tick(0)==ScriptState::finished && globals[0]==1231,"caller continuation failed");
        Check(stack.PopFinished() && stack.State()==ScriptState::idle,"base context not retired");
        stack.Compile("timed","void Callback(){}");
        const auto timed=stack.Push("timed","void Callback()",host);
        Check(timed && stack.HasActiveModule("TIMED"),"active callback module was not tracked");
        Reject([&]{stack.UnloadModule("timed");});
        Check(stack.Tick(0)==ScriptState::finished && stack.PopFinished(),"callback module did not finish");
        Check(!stack.HasActiveModule("timed"),"finished callback remained active");
        stack.UnloadModule("timed");
        Check(!stack.Push("timed","void Callback()",host),"retired callback module remained loaded");
        const auto zero=stack.Push("test","void Zero()",host);
        Check(zero && stack.Tick(0)==ScriptState::suspended && globals[0]==1231,"zero wait did not yield a frame");
        Check(stack.Tick(0)==ScriptState::suspended && globals[0]==1232,"negative wait did not yield a frame");
        Check(stack.Tick(0)==ScriptState::finished && globals[0]==1233,"nonpositive delays did not release");
        stack.PopFinished();
        stack.Push("test","void Loop()",host);
        Check(stack.Tick(0,10)==ScriptState::suspended && stack.WaitKind()==ScriptWaitKind::none,"budget pause treated as service wait");
        const auto count=globals[0];stack.Tick(0,10);Check(globals[0]>count,"budget continuation did not advance");
        Reject([&]{stack.Tick(-1);});Reject([&]{stack.Tick(std::numeric_limits<double>::infinity());});
        stack.Clear();Check(stack.Depth()==0 && !stack.Complete(movie),"clear left a stale operation");
        const auto failing=stack.Push("test","void Fail()",host);
        Check(failing && *failing>*outer && stack.Tick(0)==ScriptState::failed && !stack.Error().empty(),"script failure not retained");
        Reject([&]{stack.PopFinished();});Reject([&]{stack.Push("test","void Zero()",host);});stack.Clear();

        struct Reentrant:IScriptContextServices {
            ScriptStack& stack;
            explicit Reentrant(ScriptStack& value):stack(value) {}
            ScriptResponse Invoke(const ScriptRequest& request) override {
                Reject([&]{stack.Clear();});Reject([&]{stack.Complete(request.token);});
                Reject([&]{stack.Tick(0);});Reject([&]{stack.Push("test","void Zero()",*this);});return {};
            }
        } reentrant(stack);
        stack.Compile("callback","void Main(){giNewGame();global_mvar=7;}");
        stack.Push("callback","void Main()",reentrant);Check(stack.Tick(0)==ScriptState::finished && globals[0]==7,"reentrant guard corrupted stack");stack.Clear();

        struct Service:IScriptServices {ScriptResponse Invoke(const ScriptCall&) override {return {{},true};}} service;
        auto owner=std::make_unique<ScriptSession>(globals,service);
        owner->Compile("life","void Main(){giNewGame();global_mvar=9;}");
        auto child=owner->Fork(service);child->Start("life","void Main()");owner.reset();
        Check(child->Execute()==ScriptState::suspended,"shared SDK or bindings died with creator");
        bool invalid_source=false;try{child->Compile("invalid","void Bad( {");}catch(const std::runtime_error&){invalid_source=true;}
        Check(invalid_source,"shared engine diagnostics unavailable after creator destruction");
        bool invalid_initializer=false;
        try{child->Compile("global_call","int value=giCameraPrepare(\"MC001\");");}catch(const std::runtime_error&){invalid_initializer=true;}
        Check(invalid_initializer,"module initializer executed an unowned game service");
        Check(child->Execute()==ScriptState::finished && globals[0]==9,"child resume failed after creator destruction");
        child->Start("life","void Main()");child->Cancel();
        child->Compile("life","void Main(){global_mvar=10;}");child->Start("life","void Main()");
        Check(child->Execute()==ScriptState::finished && globals[0]==10,"cancel leaked active module ownership");
        std::cout<<"nested contexts, time waits, operation tokens, shared modules, cancellation and lifetime contracts passed\n";
    } catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
