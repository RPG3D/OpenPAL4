#include "pal4/portable/script.h"
#include "pal4/portable/script_stack.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
using namespace pal4::portable;
void Check(bool value) { if (!value) throw std::runtime_error("script contract failed"); }
std::string File(const char* path) { std::ifstream f(path,std::ios::binary); if(!f)throw std::runtime_error("missing script fixture"); return {std::istreambuf_iterator<char>(f),{}}; }
int main(int argc,char** argv) {
    try {
        if(argc==5 && std::string_view(argv[1])=="--verify-stack") {
            const auto result=VerifyChapterStackContract(File(argv[2]),File(argv[3]),File(argv[4]));
            std::cout<<"verified stack calls="<<result.calls<<" story="<<result.story<<" nested="<<result.scene_initializers<<'\n';return 0;
        }
        if(argc==6 && std::string_view(argv[1])=="--verify-opening") {
            const auto result=VerifyScriptSourceContract("Q01",File(argv[2]),argv[3],10100,File(argv[4]),
                {{287,std::int32_t(std::stoi(argv[5]))},{288,std::int32_t(std::stoi(argv[5]))},{133,std::int32_t(std::stoi(argv[5]))}});
            std::cout<<"verified opening calls="<<result.calls<<" story="<<result.story<<" suspensions="<<result.suspensions<<'\n';return 0;
        }
        if(argc>=3 && std::string_view(argv[1])=="--compile") {
            struct Unbound:IScriptServices { ScriptResponse Invoke(const ScriptCall&) override {throw std::runtime_error("compile-only services must not execute");} } unbound;
            ScriptGlobals globals{};ScriptSession session(globals,unbound);
            for(int i=2;i<argc;++i) {session.Compile("module"+std::to_string(i),File(argv[i]));std::cout<<"compiled "<<argv[i]<<'\n';}
            return 0;
        }
        if (argc==3) {
            const auto result=VerifyNewGameScriptContract(File(argv[1]),File(argv[2]));
            std::cout<<result.trace<<"verified suspensions="<<result.suspensions<<'\n'; return 0;
        }
        struct Services:IScriptServices {
            std::vector<ScriptCall> calls;
            ScriptResponse Invoke(const ScriptCall& call) override { calls.push_back(call); return {{},true}; }
        } services;
        ScriptGlobals globals{}; ScriptSession session(globals,services);
        session.Compile("test","void Main(){ maze_var1=2;giOpenMovieFlag(-3);global_mvar=42;giPlayMovie(\"x\");global_mvar++;} void Loop(){while(true){global_mvar++;}} ");
        session.Start("test","void Main()");
        Check(session.Execute()==ScriptState::suspended && session.PauseReason()==ScriptPause::service && globals[0]==0);
        Check(services.calls[0].system_id==110 && std::get<std::int32_t>(services.calls[0].arguments[0])==-3 && globals[1]==2);
        Check(session.Execute()==ScriptState::suspended && globals[0]==42 && services.calls.size()==2);
        Check(std::get<std::string>(services.calls[1].arguments[0])=="x");
        Check(session.Execute()==ScriptState::finished && globals[0]==43);
        session.Start("test","void Loop()");
        Check(session.Execute(10)==ScriptState::suspended && session.PauseReason()==ScriptPause::budget);
        bool rejected=false;try{session.Start("test","void Main()");}catch(const std::logic_error&){rejected=true;}Check(rejected);
        struct Missing:IScriptServices { ScriptResponse Invoke(const ScriptCall&) override {throw std::runtime_error("unbound game service");} } missing;
        ScriptGlobals other{};ScriptSession guarded(other,missing);guarded.Compile("guard","void Main(){giNewGame();global_mvar=1;}");guarded.Start("guard","void Main()");
        Check(guarded.Execute()==ScriptState::failed && other[0]==0 && guarded.Error()=="unbound game service");
        struct Results:IScriptServices {
            ScriptResponse Invoke(const ScriptCall& call) override {
                if(call.system_id==194)return {std::string("Q01"),false};
                if(call.system_id==61)return {12.5f,false};
                if(call.system_id==125)return {true,false};
                if(call.system_id==107)return {std::int32_t(7),false};
                throw std::runtime_error("unexpected typed result service");
            }
        } results;
        ScriptGlobals values{};ScriptSession returning(values,results);
        returning.Compile("returns","void Main(){if(giArenaGetName()==\"Q01\" && giCameraGetDist()==12.5 && giBGMConfigIsInArea(\"Q01\") && giPlayerGetLeader()==7 && abs(-7)==7 && sqrtf(4)==2)global_mvar=123;}");
        returning.Start("returns","void Main()");Check(returning.Execute()==ScriptState::finished && values[0]==123);
        const auto exact=VerifyScriptSourceContract("exact",
            "void Main(){giWait(pal4DoubleFromBits(0x3fd3333333333333));global_mvar=10201;}","void Main()",10100,
            "call 239 story 10100 d:3fd3333333333333\nfinal 10201\n",{});
        Check(exact.calls==1 && exact.suspensions==1 && exact.story==10201);
        bool mismatch_rejected=false;
        try {VerifyScriptSourceContract("wrong","void Main(){giWait(0.5);}","void Main()",0,"final 0\n",{});}
        catch(const std::runtime_error&) {mismatch_rejected=true;}
        Check(mismatch_rejected);
        std::cout<<"script globals, typed calls, suspension/resume, budget and missing-service contracts passed\n";
    } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
