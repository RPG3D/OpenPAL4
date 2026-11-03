#include "pal4/native_shop_task_script.h"
#include "pal4/native_angelscript_engine.h"
#include "pal4/native_timed_script_stack.h"
#include "pal4/native_pk_file_manager_runtime.h"
#include "pal4/shop_ui_actions.h"
#include <array>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
namespace {
void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
struct Actions final : pal4::IShopUiActionsRuntime {
    std::vector<std::string> calls;
    bool hidden{};
    void GetData(unsigned,unsigned,unsigned)override{throw std::logic_error("unexpected buy/sell");}
    unsigned ReadWord(unsigned p)override{
        if(p==0x1000u)return 0x2000u;
        Check(p==0x2004u,"task Close reads only visibility slot");return 0x3000u;
    }
    void CallVisibilitySlot(unsigned method,unsigned frame,unsigned visible,unsigned parent)override{
        Check(method==0x3000u && frame==0x1000u && !visible && !parent,"task original selector hide arguments");
        hidden=true;calls.emplace_back("hide");
    }
    unsigned TradeSystem()override{return 0x4000u;}
    unsigned SetTradeState(unsigned owner)override{
        Check(owner==0x4000u && hidden,"trade closes after selector hide");calls.emplace_back("scene");return 1;
    }
};
void Build(pal4::NativeAngelScriptEngine& engine,std::string_view module,const char* source){
    const std::string name(module);
    Check(engine.Raw()->AddScriptSection(name.c_str(),"shop task",source,static_cast<int>(std::strlen(source)))>=0,"add SDK section");
    Check(engine.Raw()->Build(name.c_str())>=0,"build SDK task module");
}
}
int main(int argc,char** argv)try{
    Check(argc==2,"provide original directory");const std::filesystem::path root=argv[1];
    const auto prior=std::filesystem::current_path();
    struct Restore{std::filesystem::path p;~Restore(){std::filesystem::current_path(p);}} restore{prior};
    std::filesystem::current_path(root);
    pal4::NativePkFileManagerRuntime packages;std::array<unsigned,20> filesystem{};filesystem[10]=0x1010u;
    const auto manager=packages.InitializePackFileManager();packages.ProcessPackFiles(manager,reinterpret_cast<unsigned>(filesystem.data()));
    Check(packages.LoadAllScripts()!=0u,"mount original global script package");
    const auto initial=packages.LoadedPackageCount();
    Actions actions;pal4::NativeAngelScriptEngine engine;
    unsigned loads{},cleared{},missing{};bool fail_load{};
    pal4::NativeTimedScriptStack stack(engine,root,[&](auto source,auto module){
        Check(source=="Q01.csb" && module=="shop-script[ShopTask]" && actions.hidden && actions.calls.back()=="scene",
            "trade closes before module loading with original shop key/source");
        ++loads;if(fail_load)throw std::runtime_error("load failed");
        Build(engine,module,"void ShopTask(){ global_mvar+=2; giWait(0.5f); global_mvar+=3; }");
    });
    const auto message=[&](auto message,auto title,auto flags){
        Check(message=="not found script files" && title=="no script" && !flags,"original missing task text");++missing;
    };
    const auto execute=[&](std::string_view task="ShopTask"){
        stack.DispatchUiScript({},[&]{++cleared;},[&](auto& selected){
            Check(pal4::NativeShopTaskScript::Execute(0x99u,{packages,selected,actions,0x5000u,0x1000u,"Q01.csb",task,message}),"task handled");
        });
    };
    execute();
    Check(stack.Depth()==1u && loads==1u && cleared==1u && !engine.GlobalStoryVariable() &&
        actions.calls==std::vector<std::string>{"hide","scene"} && packages.LoadedPackageCount()==initial,
        "task closes first and queues actual SDK context without synchronous execution or package leak");
    execute();Check(stack.Depth()==1u && loads==1u && cleared==1u,"active task duplicate suppression preserves stack");
    Check(stack.ProcessSelected(0.f) && stack.Depth()==1u && engine.GlobalStoryVariable()==2,"queued task starts and suspends through real giWait");
    Check(stack.ProcessSelected(0.25f) && engine.GlobalStoryVariable()==2,"task wait blocks");
    Check(stack.ProcessSelected(0.25f) && !stack.Active() && engine.GlobalStoryVariable()==5,"task resumes and retires actual context");
    fail_load=true;bool failed{};const auto close_count=actions.calls.size();
    try{execute();}catch(const std::runtime_error& e){failed=std::string(e.what())=="load failed";}
    Check(failed && actions.calls.size()==close_count+2u && !stack.Active() && packages.LoadedPackageCount()==initial,
        "module failure preserves original close-first ordering and unwinds package scope");
    fail_load=false;execute();Check(stack.Active(),"failed task can retry");stack.Clear();
    execute("");Check(!stack.Active() && missing==1u && packages.LoadedPackageCount()==initial,"empty task follows missing-script endpoint without queuing");
    // Verify the default original CSB loader, independent of synthetic giWait.
    // EarthBall is an existing M10 entry, used here to validate arbitrary task
    // entry loading; this does not claim it is an original NPC shop quest.
    pal4::NativeAngelScriptEngine original;pal4::NativeTimedScriptStack original_stack(original,root);
    original_stack.DispatchUiScript({},[]{},[&](auto& selected){
        (void)pal4::NativeShopTaskScript::Execute(0u,{packages,selected,actions,0x5000u,0x1000u,
            "gamedata\\editData\\script\\M10.csb","EarthBall",message});
    });
    Check(original_stack.Depth()==1u && original.FunctionId("shop-script[EarthBall]","EarthBall")>=0,
        "original M10 bytecode restores under original shop module naming");
    original_stack.Clear();Check(packages.LoadedPackageCount()==initial,"original bytecode task queue/retirement balances packages");
    std::cout<<"PASS: native shop task close-first ordering, actual SDK deferred queue/wait/resume, duplicate/failure/retry/missing and original CSB entry load; scene close endpoint recorded\n";
    return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
