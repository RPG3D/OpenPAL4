#include "pal4/portable/csb.h"
#include "pal4/portable/script_immediate.h"
#include "pal4/portable/script_stack.h"
#include <bit>
#include <iostream>
#include <stdexcept>
using namespace pal4::portable;
namespace {
void Check(bool value,const char* text){if(!value)throw std::runtime_error(text);}
void Word(std::vector<std::uint8_t>& out,std::uint32_t value){for(unsigned b=0;b<4;++b)out.push_back(static_cast<std::uint8_t>(value>>(b*8)));}
void Text(std::vector<std::uint8_t>& out,std::string_view value){Word(out,static_cast<std::uint32_t>(value.size()));out.insert(out.end(),value.begin(),value.end());}
using Op=std::pair<std::uint8_t,std::uint64_t>;
Op Call(int id){return {97,static_cast<std::uint32_t>(-id)};}
void Function(std::vector<std::uint8_t>& out,std::string_view name,int id,const std::vector<Op>& code) {
    Text(out,name);out.push_back(0);Word(out,69);Text(out,"");out.insert(out.end(),4,0);Word(out,0);Word(out,static_cast<std::uint32_t>(id));
    unsigned length{};for(const auto& op:code)length+=CsbInstructionBytes(op.first);Word(out,length);
    for(const auto& [op,value]:code){out.push_back(op);for(unsigned n=0;n<CsbInstructionBytes(op)-4;++n)out.push_back(static_cast<std::uint8_t>(value>>(n*8)));}
    Word(out,0);Word(out,0);Text(out,"");Word(out,0);
}
std::vector<std::uint8_t> Fixture(const std::vector<Op>& outer,const std::vector<Op>& inner={{13,0}}) {
    // Synthetic wire fixture only; no CS compiler and no game assets.
    std::vector<std::uint8_t> out(4);for(unsigned n=0;n<4;++n)Word(out,0);
    Function(out,"@init",-1,{{13,0}});Function(out,"@exit",-1,{{13,0}});Word(out,2);
    Function(out,"Outer",0,outer);Function(out,"Inner",1,inner);Word(out,0);Word(out,0);
    const auto bytes=static_cast<std::uint32_t>(out.size()-4);for(unsigned b=0;b<4;++b)out[b]=static_cast<std::uint8_t>(bytes>>(b*8));return out;
}
struct Host:IScriptContextServices {
    unsigned calls{};bool fail{};
    void OnCall(const ScriptRequest&) override{++calls;}
    ScriptResponse Invoke(const ScriptRequest& request) override {
        Check(request.call.system_id==237,"immediate callback escaped context owner");
        if(fail)throw std::runtime_error("fixture callback failed");return {{},true};
    }
};
ScriptState Tick(ScriptStack& stack,double seconds=0,unsigned budget=10000){return stack.Tick(seconds,budget,ScriptRunMode::original_steps);}
}
int main(){try{
    ScriptImmediateState state;state.Begin();Check(state.Current()==1 && state.Saved()==0,"begin did not save zero");
    state.Begin();state.End();Check(state.Current()==1 && state.Saved()==0,"original single previous slot lost");state.End();Check(state.Current()==0 && state.Saved()==0,"second end did not restore zero");
    ScriptGlobals globals{};Host host;ScriptStack stack(globals);
    stack.LoadCsb("fixture",Fixture({{108,0},{2,1},{100,0xffffffffu},Call(55),{108,0},{2,2},{100,0xffffffffu},{108,0},{2,3},{100,0xffffffffu},Call(56),{108,0},{2,4},{100,0xffffffffu},{13,0}}));
    Check(bool(stack.Push("fixture","void Outer()",host)),"fixture entry missing");
    Check(Tick(stack)==ScriptState::suspended && globals[0]==0 && stack.ImmediateMode()==0,"normal cue did not yield");
    Check(Tick(stack)==ScriptState::suspended && globals[0]==1 && stack.ImmediateMode()==1,"begin must end normal entry step");
    Check(Tick(stack)==ScriptState::suspended && globals[0]==3 && stack.ImmediateMode()==0,"IMM did not coalesce cues through end");
    Check(Tick(stack)==ScriptState::suspended && globals[0]==3 && stack.StepYields()==4,"end did not return to ordinary cue stepping");
    Check(Tick(stack)==ScriptState::finished && globals[0]==4 && host.calls==2,"ordinary tail did not finish");stack.Clear();
    const auto half=std::bit_cast<std::uint64_t>(.5);
    stack.LoadCsb("wait",Fixture({Call(55),{66,half},Call(239),{108,0},{2,7},{100,0xffffffffu},Call(56),{13,0}}));
    globals[0]=0;Check(bool(stack.Push("wait","void Outer()",host)),"wait entry missing");Tick(stack);Tick(stack);
    Check(stack.WaitKind()==ScriptWaitKind::delay && stack.ImmediateMode()==1,"IMM bypassed real countdown");
    Tick(stack,.25);Check(globals[0]==0,"countdown released early");Tick(stack,.25);Check(globals[0]==7 && stack.ImmediateMode()==0,"countdown did not resume IMM");stack.Clear();
    stack.LoadCsb("nested",Fixture({Call(55),Call(237),{2,21},{100,0xffffffffu},Call(56),{13,0}},{Call(55),Call(56),{13,0}}));
    globals[0]=0;stack.Push("nested","void Outer()",host);Tick(stack);Tick(stack);const auto caller=*stack.Pending();
    Check(stack.ImmediateMode()==1 && Tick(stack,100)==ScriptState::suspended && globals[0]==0,"external wait bypassed inside IMM");
    Check(bool(stack.Push("nested","void Inner()",host)) && stack.ImmediateMode()==0,"child inherited caller IMM");
    Check(stack.Complete(caller),"caller completion lost beneath child");Tick(stack);Check(stack.ImmediateMode()==1,"child begin failed");Tick(stack);Tick(stack);
    Check(stack.State()==ScriptState::finished && stack.PopFinished() && stack.ImmediateMode()==1,"caller mode did not survive child");
    Tick(stack);Check(globals[0]==21 && stack.ImmediateMode()==0,"caller did not resume own mode");stack.Clear();Check(!stack.Complete(caller),"cancelled token accepted");
    stack.LoadCsb("loop",Fixture({Call(55),{108,0},{14,static_cast<std::uint32_t>(-12)}}));stack.Push("loop","void Outer()",host);Tick(stack);
    Check(Tick(stack,0,10)==ScriptState::suspended && stack.ImmediateMode()==1 && stack.WaitKind()==ScriptWaitKind::none,"IMM escaped instruction budget");stack.Clear();Check(stack.ImmediateMode()==0 && stack.SavedImmediateMode()==0,"clear retained IMM state");
    stack.Push("nested","void Outer()",host);Tick(stack);host.fail=true;
    Check(Tick(stack)==ScriptState::failed && stack.Error().find("fixture callback failed")!=std::string::npos,"callback exception lost to step yield");stack.Clear();
    std::cout<<"original IMM stepping, single previous slot, waits, context isolation and budget passed\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
