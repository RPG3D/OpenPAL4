#include "pal4/native_script_trade_runtime.h"
#include "pal4/native_angelscript_engine.h"
#include "pal4/native_game_script_callbacks.h"
#include "pal4/native_script_frame_runner.h"
#include <iostream>
#include <stdexcept>
#include <string>
namespace {void Check(bool v,const char* why){if(!v)throw std::runtime_error(why);}}
int main(){try{
    pal4::NativeAngelScriptEngine engine;
    const char source[]="void Trade(){global_mvar+=1;giStartTradeSystem(\"ShopA\",\"TaskB\");global_mvar+=10;}";
    Check(engine.Raw()->AddScriptSection("trade_wait","trade_wait.cs",source,sizeof(source)-1u)>=0 && engine.Raw()->Build("trade_wait")>=0,"compile original giStartTradeSystem signature");
    pal4::NativeScriptFrameRunner first(engine),second(engine);unsigned current=4u,starts{},queries{};bool fail{};
    std::string shop,task;
    pal4::NativeScriptTradeServices endpoints{
        [&](auto a,auto b){if(fail)throw std::runtime_error("actual shop entry failed");++starts;shop=a;task=b;current=4u;},
        [&]{return current;},[&](auto state){Check(state!=0u,"absent current state must not be queried");++queries;return state;}};
    pal4::NativeScriptTradeRuntime wait(first.Context(),endpoints),other(second.Context(),endpoints);
    bool missing{};try{pal4::NativeScriptTradeRuntime invalid(first.Context(),{});}catch(const std::invalid_argument&){missing=true;}Check(missing,"missing UI/state services reject construction");
    bool foreign{};try{(void)pal4::ScriptTradeWait(wait).Start("x","y");}catch(const std::logic_error&){foreign=true;}Check(foreign && !starts,"foreign SDK context rejected before global shop mutation");
    pal4::NativeGameScriptServices services{};services.trade=&wait;pal4::NativeGameScriptCallbacks callbacks(services);engine.SetCallbacks(&callbacks);
    first.Queue("trade_wait","Trade");Check(first.Tick(0.f,&wait)==pal4::NativeScriptFrameStatus::suspended && wait.BlocksScript() && engine.GlobalStoryVariable()==1 && starts==1u && shop=="ShopA" && task=="TaskB","original callback opens exact shop/task and suspends same context before continuation");
    for(auto state:{4u,5u,2u,0xffffffffu}){current=state;Check(first.Tick(1.f,&wait)==pal4::NativeScriptFrameStatus::suspended && engine.GlobalStoryVariable()==1,"page/selector/Foundry/non-Scene states retain actual SDK wait");}
    current=4u;second.Queue("trade_wait","Trade");Check(second.Tick(0.f,&other)==pal4::NativeScriptFrameStatus::failed && starts==1u && wait.BlocksScript(),"another context cannot borrow first wait service");second.ClearExecution();
    services.trade=&other;callbacks.SetServices(services);second.Queue("trade_wait","Trade");Check(second.Tick(0.f,&other)==pal4::NativeScriptFrameStatus::suspended && other.BlocksScript() && wait.BlocksScript() && starts==2u,"two contexts retain independent wait owners");
    current=1u;Check(second.Tick(0.f,&other)==pal4::NativeScriptFrameStatus::finished && !other.BlocksScript() && wait.BlocksScript(),"Scene1 resumes only the selected SDK context");
    // The rejected second-context call also executed its prefix increment.
    services.trade=&wait;callbacks.SetServices(services);Check(first.Tick(0.f,&wait)==pal4::NativeScriptFrameStatus::finished && !wait.BlocksScript() && engine.GlobalStoryVariable()==23,"first context resumes exact continuation once Scene is active");
    first.Queue("trade_wait","Trade");Check(first.Tick(0.f,&wait)==pal4::NativeScriptFrameStatus::suspended,"prepare absent-state completion");const auto before_queries=queries;current=0u;
    Check(first.Tick(0.f,&wait)==pal4::NativeScriptFrameStatus::finished && queries==before_queries,"absent current state closes without state virtual query");
    first.Queue("trade_wait","Trade");Check(first.Tick(0.f,&wait)==pal4::NativeScriptFrameStatus::suspended,"prepare cancellation");auto* identity=&first.Context();first.ClearExecution();wait.ClearHandlers();
    Check(&first.Context()==identity && !wait.BlocksScript() && !wait.Tick(0.f) && current==4u,"context cleanup preserves SDK owner and global shop without stale wait");
    fail=true;first.Queue("trade_wait","Trade");Check(first.Tick(0.f,&wait)==pal4::NativeScriptFrameStatus::failed && !wait.BlocksScript(),"shop-entry exception does not publish an active handler");fail=false;first.ClearExecution();wait.ClearHandlers();
    first.Queue("trade_wait","Trade");Check(first.Tick(0.f,&wait)==pal4::NativeScriptFrameStatus::suspended && wait.BlocksScript(),"same wait owner can retry after failed entry");first.ClearExecution();wait.ClearHandlers();
    std::cout<<"PASS: native giStartTradeSystem SDK suspend/Scene-only resume, original strings, independent contexts, absent state, cancellation, failure/retry and owner rejection\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
