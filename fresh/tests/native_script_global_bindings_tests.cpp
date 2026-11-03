#include "pal4/native_angelscript_engine.h"
#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
namespace {
void Check(bool value,const char* why){if(!value)throw std::runtime_error(why);}
void Build(pal4::NativeAngelScriptEngine& e,const char* module,const char* source){Check(e.Raw()->AddScriptSection(module,"globals.cs",source,static_cast<int>(std::char_traits<char>::length(source)))>=0 && e.Raw()->Build(module)>=0,"compile real SDK global declarations");}
unsigned Run(pal4::NativeAngelScriptEngine& e,const char* module,const char* function){asIScriptContext* c{};Check(e.Raw()->CreateContext(&c)>=0 && c,"SDK context");const auto id=e.FunctionId(module,function);Check(id>=0 && c->Prepare(id)>=0 && c->Execute()==asEXECUTION_FINISHED,"SDK global function executes");const auto value=c->GetReturnDWord();c->Release();return value;}
}
int main()try{
    std::array<int,48> storage{};
    pal4::NativeAngelScriptEngine first(&storage[0],[&](unsigned address)->int*{Check(address>=0x90dec0u && address<=0x90df7cu && !(address%4u),"original global address range");return &storage.at((address-0x90dec0u)/4u);});
    Build(first,"global_writer","void Write(){global_mvar=12;maze_var1=13;maze_var10=14;var_shu=15;var_shop010=16;var_shop035=17;var_shop030=18;}");
    (void)Run(first,"global_writer","Write");
    Check(storage[0]==12 && storage[1]==13 && storage[10]==14 && storage[11]==15 && storage[30]==16 && storage[40]==17 && storage[47]==18,"public SDK writes original distinct global/maze/named host slots");
    Build(first,"global_reader","int Read(){maze_var1+=5;var_shop010+=2;return var_shop010;}");
    Check(Run(first,"global_reader","Read")==18u && storage[30]==18 && storage[1]==18,"independent SDK module shares same live globals");
    pal4::NativeAngelScriptEngine second;
    Build(second,"isolated_reader","int Read(){return global_mvar+maze_var1+maze_var10+var_shu+var_shop010+var_shop035+var_shop030;}");
    Check(Run(second,"isolated_reader","Read")==0u && storage[30]==18,"standalone SDK engine owns independent zero-initialized globals");
    bool missing{};try{pal4::NativeAngelScriptEngine invalid(nullptr,[](unsigned)->int*{return nullptr;});}catch(const std::invalid_argument&){missing=true;}Check(missing,"missing borrowed global storage rejects construction");
    int foreign{};bool mismatch{};try{pal4::NativeAngelScriptEngine invalid(&foreign,[&](unsigned address){return &storage.at((address-0x90dec0u)/4u);});}catch(const std::invalid_argument&){mismatch=true;}Check(mismatch,"story/global alias mismatch rejects construction");
    std::cout<<"PASS: SDK global/maze/named borrowed storage, distinct shop010/shop035 slots, cross-module persistence, engine isolation and invalid-owner rejection\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
