#include "pal4/native_property_window_script.h"
#include "pal4/native_angelscript_engine.h"
#include "pal4/native_timed_script_stack.h"
#include "pal4/native_pk_file_manager_runtime.h"
#include <array>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
namespace {
void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void Build(pal4::NativeAngelScriptEngine& engine,std::string_view module,const char* source){
    const std::string name(module);Check(engine.Raw()->AddScriptSection(name.c_str(),"EarthBall test",source,static_cast<int>(std::strlen(source)))>=0,"add SDK section");
    Check(engine.Raw()->Build(name.c_str())>=0,"build SDK test module");}
}
int main(int argc,char** argv)try{
    Check(argc==2,"provide original game directory");
    const std::filesystem::path root=argv[1];
    const auto previous=std::filesystem::current_path();
    struct Restore{std::filesystem::path p;~Restore(){std::filesystem::current_path(p);}} restore{previous};
    std::filesystem::current_path(root);
    pal4::NativePkFileManagerRuntime packages;std::array<std::uint32_t,20> filesystem{};filesystem[10]=0x1010u;
    const auto manager=packages.InitializePackFileManager();packages.ProcessPackFiles(manager,reinterpret_cast<std::uint32_t>(filesystem.data()));
    Check(packages.LoadAllScripts()!=0u,"original global script package mount");
    const auto initial_packages=packages.LoadedPackageCount();
    pal4::NativeAngelScriptEngine engine;unsigned loads{},cleanup{},sounds{},closed{};bool fail_load{};
    pal4::NativeTimedScriptStack stack(engine,root,[&](auto source,auto module){
        Check(source=="Q01.csb" && module=="PFNSkipScript[EarthBall]","EarthBall source and full module key");++loads;
        if(fail_load)throw std::runtime_error("expected load failure");
        Build(engine,module,"void EarthBall(){ global_mvar+=2; giWait(0.5f); global_mvar+=3; }");});
    const auto execute=[&]{stack.DispatchUiScript({},[&]{++cleanup;},[&](auto& selected){
        pal4::NativePropertyWindowScript::Execute({packages,selected,0x1000u,0x2000u,"Q01.csb",
            [&](auto file,auto mode){Check(file=="WA43.wav" && mode==0u,"original EarthBall sound request");++sounds;},
            [&](auto key,auto a,auto b){Check(key==0x8a80b4u && !a && !b,"EarthBall closes System with original event");++closed;}});});};
    execute();Check(stack.Depth()==1u && loads==1u && cleanup==1u && sounds==1u && closed==1u && !engine.GlobalStoryVariable(),
        "EarthBall queues before UI close and executes no script synchronously");
    execute();Check(stack.Depth()==1u && loads==1u && cleanup==1u && sounds==2u && closed==2u,"duplicate active module is suppressed but UI closes");
    Check(packages.LoadedPackageCount()==initial_packages,"EarthBall balances original package scopes");
    Check(stack.ProcessSelected(0.0f) && stack.Depth()==1u && engine.GlobalStoryVariable()==2,"queued SDK EarthBall executes and owns real giWait");
    Check(stack.ProcessSelected(0.25f) && engine.GlobalStoryVariable()==2,"EarthBall wait blocks the script");
    Check(stack.ProcessSelected(0.25f) && !stack.Active() && engine.GlobalStoryVariable()==5,"EarthBall resumes and unloads completed module");
    fail_load=true;bool rejected{};const auto before=closed;
    try{execute();}catch(const std::runtime_error& e){rejected=std::string(e.what())=="expected load failure";}
    Check(rejected && !stack.Active() && closed==before && packages.LoadedPackageCount()==initial_packages,"failed module load unwinds strings/packages without closing System");
    fail_load=false;execute();Check(stack.Depth()==1u && cleanup==2u,"failed EarthBall load can retry");stack.Clear();
    std::cout<<"PASS: native EarthBall synthetic SDK queue/wait/resume and retry\n";
    pal4::NativeAngelScriptEngine original;pal4::NativeTimedScriptStack original_stack(original,root);
    original_stack.DispatchUiScript({},[]{},[&](auto& selected){pal4::NativePropertyWindowScript::Execute({packages,selected,0x1000u,0x2000u,
        "gamedata\\editData\\script\\M10.csb",[](auto,auto){},[](auto,auto,auto){}});});
    Check(original_stack.Depth()==1u && original.FunctionId("PFNSkipScript[EarthBall]","EarthBall")>=0,
        "actual original M10 bytecode restores and queues its EarthBall entry");
    original_stack.Clear();Check(packages.LoadedPackageCount()==initial_packages,"original EarthBall queue/retirement balances packages");
    std::cout<<"PASS: original EarthBall bytecode queue, shared473C90 policy, actual SDK wait/resume, duplicate/failure/retry and package cleanup\n";
    return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
