#include "pal4/native_script_world_map_runtime.h"
#include "pal4/native_world_map_state.h"
#include "pal4/native_game_script_callbacks.h"
#include "pal4/native_script_frame_runner.h"
#include "pal4/game_arena_load_handler.h"

#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
struct Xml final : pal4::IGameArenaLoadRuntime {
    unsigned document{}, created{}, destroyed{}, entered{};
    bool fail_entry{};
    unsigned InitializeXmlDocument() override { Check(!document, "XML reentry"); ++created; return document=0x1234u; }
    unsigned GameInstance() override { return 0x2345u; }
    void InitializeStateAndUi(unsigned game, unsigned state, unsigned doc) override {
        Check(game==GameInstance() && state==11u && doc==document, "real callback enters state11 with owned XML");
        ++entered;
        if (fail_entry) throw std::runtime_error("entry failure");
    }
    int DestroyXmlDocument(unsigned doc) override {
        Check(doc==document && doc, "XML destruction identity"); document=0u; ++destroyed; return 0;
    }
    unsigned ArenaManager() override { throw std::logic_error("unexpected arena"); }
    unsigned ReadWord(unsigned) override { throw std::logic_error("unexpected read"); }
    void WriteWord(unsigned,unsigned) override { throw std::logic_error("unexpected write"); }
    std::optional<unsigned> FindAttribute(unsigned,std::string_view) override { throw std::logic_error("unexpected config"); }
    void SetInt(unsigned,int) override { throw std::logic_error("unexpected integer"); }
    void SetString(unsigned,std::string_view) override { throw std::logic_error("unexpected string"); }
    unsigned InitializeIntAttribute(std::string_view,int) override { throw std::logic_error("unexpected attribute"); }
    unsigned InitializeStringAttribute(std::string_view,std::string_view) override { throw std::logic_error("unexpected attribute"); }
    void AddAttribute(unsigned,unsigned) override { throw std::logic_error("unexpected attribute"); }
    void DestroyAttribute(unsigned) override { throw std::logic_error("unexpected attribute"); }
    unsigned ActorControl() override { throw std::logic_error("unexpected actor"); }
    void ProcessActorInput(unsigned) override { throw std::logic_error("unexpected actor"); }
    void ProcessActorInputAndClearVector(unsigned) override { throw std::logic_error("unexpected actor"); }
    unsigned InputManager() override { throw std::logic_error("unexpected input"); }
    void InitializeInputState(unsigned) override { throw std::logic_error("unexpected input"); }
    unsigned Milliseconds() override { throw std::logic_error("unexpected clock"); }
    unsigned GameState() override { throw std::logic_error("unexpected game state"); }
};
void ScriptProtocol() {
    pal4::NativeAngelScriptEngine engine;
    constexpr char source[]="void Open(){giShowWorldMap();global_mvar=1;}";
    Check(engine.Raw()->AddScriptSection("map_protocol","map.cs",source,sizeof(source)-1)>=0 &&
          engine.Raw()->Build("map_protocol")>=0,"compile actual WorldMap callback signature");
    pal4::NativeScriptFrameRunner runner(engine);
    Xml xml;
    bool visible{};
    unsigned shows{};
    pal4::NativeScriptWorldMapRuntime wait(runner.Context(),xml,{
        [&](unsigned key){Check(key==0u,"original show event argument");visible=true;++shows;},
        []{return 0x3456u;},
        [&](unsigned frame){Check(frame==0x3456u,"visibility uses creating map frame");return visible;}});
    pal4::NativeGameScriptServices services; services.world_map=&wait;
    pal4::NativeGameScriptCallbacks callbacks(services); engine.SetCallbacks(&callbacks);
    runner.Queue("map_protocol","Open");
    Check(runner.Tick(0.f,&wait)==pal4::NativeScriptFrameStatus::suspended &&
          wait.BlocksScript() && shows==1u && xml.created==1u && xml.entered==1u &&
          xml.destroyed==1u && !xml.document && engine.GlobalStoryVariable()==0,
          "actual SDK callback shows map, suspends before continuation, and retires entry XML");
    Check(runner.Tick(.017f,&wait)==pal4::NativeScriptFrameStatus::suspended &&
          engine.GlobalStoryVariable()==0,"visible WorldMap retains script wait");
    visible=false;
    Check(runner.Tick(.017f,&wait)==pal4::NativeScriptFrameStatus::finished &&
          !wait.BlocksScript() && engine.GlobalStoryVariable()==1,
          "actual map disappearance resumes invoking SDK exactly once");
    runner.ClearExecution();wait.ClearHandlers();xml.fail_entry=true;
    runner.Queue("map_protocol","Open");
    Check(runner.Tick(0.f,&wait)==pal4::NativeScriptFrameStatus::failed &&
          xml.created==2u && xml.destroyed==2u && !xml.document,
          "state entry failure retains original SDK exception and releases XML");
    runner.ClearExecution();wait.ClearHandlers();engine.SetCallbacks(nullptr);
}
void StateProtocol() {
    std::array<std::uint8_t,0x224> manager{};
    unsigned console=1u;
    std::vector<std::string> calls;
    pal4::NativeWorldMapStateServices services;
    services.manager=reinterpret_cast<unsigned>(manager.data());
    services.console=reinterpret_cast<unsigned>(&console);
    services.music=0x4567u;services.script_executor=0x5678u;
    services.play_music=[&](std::string_view name,float seconds,unsigned flags){
        Check(name=="P01-1.mp3" && seconds==3.f && !flags,"original state11 music request");calls.emplace_back("music");};
    services.update_ui=[&](double elapsed){Check(elapsed==static_cast<double>(.017f),"UI receives original float-to-double delta");calls.emplace_back("ui");};
    services.process_scripts=[&](double){calls.emplace_back("scripts");return 7u;};
    services.render_ui=[&](unsigned frame){Check(frame==0x6789u,"original UI-only frame passed through");calls.emplace_back("render");return 8u;};
    pal4::NativeWorldMapStateOwner owner;
    Check(owner.Construct()!=0u && *reinterpret_cast<const unsigned*>(owner.Address())==0x848664u,
          "native WorldMap owner retains original state11 identity");
    pal4::NativeWorldMapStateRuntime state(std::move(services));
    Check(state.Enter(0u) && !console && manager[0x221u]==1u,"entry sets actual UI flag and clears console");
    Check(state.Update(.017f)==7u && state.Render(0x6789u)==8u &&
          calls==std::vector<std::string>{"music","ui","scripts","render"},
          "original entry/UI/script ordering and UI-only render dispatch");
    state.Leave();owner.Destroy();Check(!owner.Address(),"state owner retires independently of borrowed UI");
}
}
int main() {
    try { ScriptProtocol();StateProtocol();std::cout<<"PASS: WorldMap SDK wait/XML and state11 protocol\n";return 0; }
    catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
