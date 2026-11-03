#include "pal4/native_script_combat_start.h"
#include "pal4/native_script_combat_config.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_event_document_runtime.h"
#include "pal4/native_angelscript_engine.h"
#include "pal4/native_game_script_callbacks.h"
#include <array>
#include <iostream>
#include <stdexcept>

namespace {
void Check(bool ok) { if(!ok)throw std::runtime_error("native combat request test"); }
struct Commands final : pal4::IScriptCommandRuntime {
    pal4::ScriptCombatExecutor executor{0x1000u,{}};
    pal4::ScriptCommandQueue& ScriptExecutorQueue() override { return executor.commands; }
};
}
int main() {
    try {
        Commands commands;
        pal4::NativeBaseObjectMemory memory;
        pal4::NativeEventDocumentRuntime docs(memory);
        pal4::NativeScriptCombatConfig config;
        unsigned entries=0u;
        bool fail=false;
        std::uint32_t saved=0u;
        const auto integer=[&](std::uint32_t node,const char* name) {
            return docs.AttributeToInt(docs.FindNumericAttribute(node,name),-999);
        };
        const auto verify=[&](std::uint32_t document) {
            Check(docs.NodeName(document)=="CombatParam" && docs.ChildCount(document)==3u);
            Check(docs.AttributeToString(docs.FindNumericAttribute(document,"CombatScene"))=="CombatWorld");
            Check(docs.AttributeToString(docs.FindNumericAttribute(document,"CombatArea"))=="M01");
            Check(integer(document,"StartType")==0);
            constexpr std::array<int,3> ids{2579,2580,2580},positions{1,0,2};
            for(unsigned i=0;i<3u;++i) {
                const auto child=docs.ChildAt(document,i);
                Check(docs.NodeName(child)=="CbMonster" && integer(child,"MonsterId")==ids[i]);
                Check(integer(child,"CombatPosition")==positions[i] && integer(child,"MonsterAbState")==0);
            }
        };
        pal4::NativeScriptCombatStart request(commands.executor,docs,config,0x2000u,
            [&](std::uint32_t document) {
                ++entries;verify(document);
                Check(commands.executor.commands.Commands().size()==3u);
                Check(config.Snapshot().words==std::array<std::uint32_t,5>{4,2579,1,0,2});
                if(fail)throw std::runtime_error("intentional combat entry failure");
                saved=docs.CloneDocument(document);
            });
        Check(pal4::ScriptCombatStarter::Start("M01",request)==0x1000u && entries==0u && !request.Document());
        pal4::NativeAngelScriptEngine engine;
        pal4::NativeGameScriptServices services{};
        services.commands=&commands;services.combat_start=&request;
        services.combat_config=&config;services.combat_text=&config;
        pal4::NativeGameScriptCallbacks callbacks(services);engine.SetCallbacks(&callbacks);
        constexpr char code[]=R"(
            void Add() { giConfigCombatBgm("P39"); giConfigCombatCamera("CA7"); giConfigCombatGroundCamera("CA12");
                giConfigCombatParam(true,4,2579,2);
                giAddCombatMonster(2579,1); giAddCombatMonster(2580,0); giAddCombatMonster(2580,2); }
            void Start() { giStartCombat("M01"); }
        )";
        Check(engine.Raw()->AddScriptSection("combat","combat.cs",code,sizeof(code)-1)>=0);
        Check(engine.Raw()->Build("combat")>=0);
        asIScriptContext* context{};Check(engine.Raw()->CreateContext(&context)>=0 && context);
        const auto run=[&](const char* name) {
            Check(context->Prepare(engine.FunctionId("combat",name))>=0);
            return context->Execute();
        };
        try {
            Check(run("Add")==asEXECUTION_FINISHED);
            const auto capacity=commands.executor.commands.Capacity();
            fail=true;Check(run("Start")==asEXECUTION_EXCEPTION);
            Check(std::string_view(context->GetExceptionString())=="intentional combat entry failure");
            Check(entries==1u && commands.executor.commands.Commands().size()==3u && !request.Document());
            Check(memory.OutstandingAllocations()==0u && docs.DocumentCount()==0u);
            fail=false;Check(run("Start")==asEXECUTION_FINISHED);
            Check(entries==2u && commands.executor.commands.Commands().empty());
            Check(commands.executor.commands.Capacity()==capacity && !request.Document() && saved);
            verify(saved); // State-owned XML clone survives the callback's temporary document.
            Check(docs.DocumentCount()==4u);
            docs.DestroyDocument(saved);(void)docs.Free(saved);saved=0u;
            Check(memory.OutstandingAllocations()==0u && docs.DocumentCount()==0u);
            Check(run("Start")==asEXECUTION_FINISHED && entries==2u);
            Check(commands.executor.commands.Capacity()==capacity && !request.Document());
            bool rejected=false;
            try {request.EnterGameState(0x2000u,3u);}catch(const std::invalid_argument&){rejected=true;}
            Check(rejected);
            context->Release();
        } catch(...) {context->Release();throw;}
        std::cout<<"Compiled combat request: shared monster queue, exact native M01 XML, property110, entry failure retention, retry, copied state document, empty requests and zero final allocations verified\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
}
