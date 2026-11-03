#include "pal4/native_script_player_skill_acquisition.h"
#include "pal4/native_script_inventory_change.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_game_database.h"
#include "pal4/native_game_script_callbacks.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/player_manager_object.h"
#include "pal4/game_text_format.h"

#include <cstring>
#include <iostream>
#include <stdexcept>

namespace { void Check(bool value,const char* message) {
    if (!value) throw std::runtime_error(message);
} }

int main(int argc,char** argv) try {
    Check(argc==2,"original game root required");
    pal4::NativeGameDatabase database;
    Check(database.Load(argv[1]),"load original GameDB");
    pal4::NativeInventorySession inventory;
    inventory.BindDatabase(database); (void)inventory.Start(); inventory.Reset();
    pal4::NativeBaseObjectMemory memory;
    pal4::PlayerManagerObjectState players{};
    const auto name=memory.Allocate(5u);std::memcpy(reinterpret_cast<void*>(name),"hero",5u);
    players.Word(0x9fcu/4u)=name;
    unsigned creates{},shows{},sounds{};bool allow_hint=true,throw_show=false;
    std::string shown;
    pal4::NativeScriptInventoryChange hints(inventory,database,
        [&]{++creates;return allow_hint?7u:0u;},
        [&](unsigned window,std::string_view text,float x,float y,float scale,unsigned mode,unsigned enabled){
            Check(window==7u && x==.5f && y==.15f && scale==1.0f && mode==0u && enabled==1u,
                  "original learning hint arguments");
            if (throw_show) throw std::runtime_error("learning hint failure");
            ++shows;shown=text;
        },
        [&](std::string_view path,unsigned mode){
            Check(path=="gamedata\\PALSound\\WA17.wav" && mode==1u,"original learning sound");++sounds;
        });
    pal4::NativeScriptPlayerSkillAcquisition learning(players,memory,database,hints);
    pal4::NativeGameScriptServices services;services.player_skill_acquisition=&learning;
    pal4::NativeGameScriptCallbacks callbacks(services);
    pal4::NativeAngelScriptEngine engine;engine.SetCallbacks(&callbacks);
    constexpr char script[] =
        "void Learn(){giPlayerAddSkill(0,5502,true);} "
        "void Silent(){giPlayerAddSkill(0,5502,false);} "
        "void Missing(){giPlayerAddSkill(0,2147483647,true);} "
        "void MissingPlayer(){giPlayerAddSkill(4,5502,true);} "
        "void Other(){giPlayerAddSkill(0,5501,true);}";
    Check(engine.Raw()->AddScriptSection("learn","learn.cs",script,sizeof(script)-1)>=0 &&
          engine.Raw()->Build("learn")>=0,"compile actual SDK learning calls");
    asIScriptContext* context{};
    Check(engine.Raw()->CreateContext(&context)>=0 && context,"create SDK context");
    struct Release { asIScriptContext* p;~Release(){(void)p->Release();} } release{context};
    const auto run=[&](const char* function){
        Check(context->Prepare(engine.FunctionId("learn",function))>=0,"prepare learning call");
        return context->Execute();
    };
    const auto keys=[&]{
        const auto first=players.Word((0x938u+0xa4u)/4u);
        const auto last=players.Word((0x938u+0xa8u)/4u);
        std::vector<unsigned> result;
        for(auto p=first;p<last;p+=4u)result.push_back(memory.ReadDword(p));
        return result;
    };
    Check(run("Learn")==asEXECUTION_FINISHED && keys()==std::vector<unsigned>{5502u} &&
          creates==1u && shows==1u && sounds==1u,"learned Stunt reaches the actual player collection");
    const auto record=learning.FindSkill(learning.GameDatabase()+0xcu,5502u);
    Check(record && shown==pal4::FormatGameTextArguments(*database.FindTableTextById("GameString",0x118u),
          "hero",learning.SkillName(record)),"original GameString118 and Stunt/player name order");
    Check(run("Learn")==asEXECUTION_FINISHED && keys()==std::vector<unsigned>{5502u} &&
          shows==2u && sounds==2u,"duplicate acquisition remains unique and retains original hint behavior");
    Check(run("Silent")==asEXECUTION_FINISHED && creates==3u && shows==2u && sounds==2u,
          "silent learning still creates the original hint object, skips display and sound");
    Check(run("Missing")==asEXECUTION_FINISHED && creates==4u && keys()==std::vector<unsigned>{5502u},
          "unknown Stunt cannot mutate the learned collection");
    Check(run("MissingPlayer")==asEXECUTION_FINISHED && creates==4u,"missing player skips UI and learning");
    allow_hint=false;
    Check(run("Other")==asEXECUTION_FINISHED && creates==5u && keys()==std::vector<unsigned>{5502u},
          "missing hint window suppresses learning per original early gate");
    allow_hint=true;throw_show=true;
    Check(run("Other")==asEXECUTION_EXCEPTION && keys()==std::vector<unsigned>({5501u,5502u}) &&
          learning.HintText().empty() && sounds==2u,
          "presentation exception cleans text and preserves the already inserted Stunt");
    bool rejected=false;
    try{(void)pal4::FormatGameTextArguments("%s%d","hero","skill");}
    catch(const std::invalid_argument&){rejected=true;}
    Check(rejected && pal4::FormatGameTextArguments("", "hero", "skill").empty(),
          "host CRT wrapper validates argument types and preserves empty fallback");
    inventory.Close();Check(!inventory.OutstandingAllocations(),"inventory released");
    std::cout<<"PASS: actual SDK learning, original GameDB/Stunt, sorted player collection, hints/audio/gates/cleanup\n";
    return 0;
} catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<'\n';return 1;}
