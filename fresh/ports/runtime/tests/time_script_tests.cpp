#include "pal4/portable/game_services.h"
#include "pal4/portable/arena_skill_state.h"
#include "pal4/portable/gbk.h"
#include "pal4/portable/package.h"
#include "pal4/portable/arena_source.h"
#include "pal4/portable/dff_collision.h"
#include "pal4/portable/scene_objects.h"
#include "pal4/portable/minimap.h"
#include "pal4/portable/original_database.h"
#include "pal4/portable/script_stack.h"
#include "pal4/portable/time_script.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <vector>

using namespace pal4::portable;
namespace {
void Check(bool value,const char* message="time script contract failed") { if (!value) throw std::runtime_error(message); }
template<class F> void Reject(F action) {
    bool rejected = false;
    try { action(); } catch (const std::exception&) { rejected = true; }
    Check(rejected);
}
ScriptRequest Request(const ScriptCall& call) { return {{1, 1}, call}; }

void RunOriginalM01Timer(std::string_view path) {
    std::ifstream input(std::string(path),std::ios::binary);
    if(!input)throw std::runtime_error("cannot open original script.cpk");
    PackageReader package(input);
    const auto bytes=package.Read("M01.csb");
    ScriptGlobals globals{};globals[0]=10200;
    ScriptStack stack(globals);
    stack.LoadCsb("M01",bytes,DecodeGbk);
    std::ostringstream trace;
    GameServices game(trace);
    unsigned ready_calls{},visibility_queries{};
    game.ready_arena=[&](bool) { ++ready_calls; };
    struct ControlledServices final:IScriptContextServices {
        GameServices& game;
        unsigned& visibility_queries;
        ControlledServices(GameServices& value,unsigned& queries):game(value),visibility_queries(queries) {}
        void OnCall(const ScriptRequest& request) override { game.OnCall(request); }
        ScriptResponse Invoke(const ScriptRequest& request) override {
            if(request.call.system_id==154) {
                if(request.call.arguments.size()!=1)throw std::runtime_error("invalid controlled visibility query");
                ++visibility_queries;
                return {true,false};
            }
            return game.Invoke(request);
        }
    } services(game,visibility_queries);

        ScriptGlobals callback_globals{};ScriptStack callback_stack(callback_globals);
        callback_stack.LoadCsb("M01",bytes,DecodeGbk);
        Reject([&] { (void)PushTimeScriptCallback(callback_stack,"M01","void missing_callback()",services); });
        Check(callback_stack.Depth()==0,"missing timed callback changed script stack");
        Check(bool(PushTimeScriptCallback(callback_stack,"M01","void M01_1_init()",services)),
            "loaded timed callback was not pushed");
        Check(!PushTimeScriptCallback(callback_stack,"M01","void M01_1_init()",services),
            "active timed callback was pushed twice");
        callback_stack.Clear();

    constexpr std::string_view alias="TimeScript[func9001]";
    bool callback_loaded{},callback_executed{},callback_cleaned{};
    TimeScript timer([&](std::string_view callback) {
        if(callback!="func9001")throw std::runtime_error("unexpected original timer callback");
        if(!callback_loaded) {
            stack.LoadCsb(alias,bytes,DecodeGbk);
            callback_loaded=true;
        }
        (void)PushTimeScriptCallback(stack,alias,"void func9001()",services);
    });
    game.start_time_script=[&](float delay,std::string_view callback) { timer.Start(delay,callback); };
    game.terminate_time_script=[&] { timer.Terminate(); };

    if(!stack.Push("M01","void M01_1_init()",services))
        throw std::runtime_error("original M01_1_init entry missing");
    bool init_finished{};
    for(unsigned frame=0;frame<1000;++frame) {
        const auto state=stack.Tick(0,100000,ScriptRunMode::original_steps);
        if(state==ScriptState::failed)throw std::runtime_error(stack.Error());
        if(state==ScriptState::finished) {
            stack.PopFinished();init_finished=true;break;
        }
    }
    Check(init_finished && globals[0]==10200 && ready_calls==1,
        "original M01 initialization did not complete through GameServices");
    Check(timer.Active() && timer.Delay()==180.0f && timer.Callback()=="func9001",
        "original M01 did not arm the expected timer");

    constexpr float frame_delta=1.0f/60.0f;
    unsigned advanced_frames{};
    for(;advanced_frames<12000 && !callback_executed;++advanced_frames) {
        const auto state=stack.State();
        const bool busy=state==ScriptState::prepared || state==ScriptState::suspended;
        timer.Advance(frame_delta,false,busy);
        if(!stack.Depth())continue;
        const auto callback_state=stack.Tick(frame_delta,100000,ScriptRunMode::original_steps);
        if(callback_state==ScriptState::failed)throw std::runtime_error(stack.Error());
        if(callback_state==ScriptState::finished) {
            if(!stack.HasActiveModule(alias))callback_executed=true;
            stack.PopFinished();
            if(callback_loaded && !stack.HasActiveModule(alias)) {
                stack.UnloadModule(alias);callback_loaded=false;callback_cleaned=true;
            }
        }
    }
    Check(callback_executed && callback_cleaned && stack.Depth()==0,
        "original func9001 did not execute and clean up its synthetic module");
    if(visibility_queries!=100 || globals[0]!=10200)
        throw std::runtime_error("original func9001 query/story mismatch: queries="+
            std::to_string(visibility_queries)+" story="+std::to_string(globals[0]));
    Check(timer.Active(),"repeating timer stopped after its first callback");

    const ScriptCall terminate{217,globals[0],{}};
    Check(!game.Invoke(Request(terminate)).suspend && !timer.Active() && timer.Callback().empty(),
        "original timer termination did not clear state");
    std::cout<<"original M01_1_init -> giTimeScript -> M01 CSB func9001 passed; frames="
        <<advanced_frames<<" visibility_queries="<<visibility_queries
        <<" ready_service_scope=controlled fixture\n";
}

void RunOriginalM01PlayerWalk(std::string_view path) {
    std::ifstream input(std::string(path),std::ios::binary);
    if(!input)throw std::runtime_error("cannot open original script.cpk");
    PackageReader package(input);
    const auto bytes=package.Read("M01.csb");
    ScriptGlobals globals{};globals[0]=10300;
    ScriptStack stack(globals);
    stack.LoadCsb("M01",bytes,DecodeGbk);
    std::ostringstream trace;
    GameServices game(trace);
    ArenaSkillState arena_skills;
    game.hide_skill_object=[&] {(void)arena_skills.HideSkillObject();};
    game.lock_player=[] {};
    game.set_camera_collision=[](bool) {};
    game.set_auto_seek=[](bool) {};
    game.set_position=[](Point3) {};
    game.set_angle=[](float) {};
    game.ready_arena=[](bool) {};
    game.set_camera_mode=[](std::uint32_t,bool) {};
    game.prepare_camera=[](const ScriptRequest&) {return ScriptResponse{std::int32_t(1),false};};
    game.run_camera=[](const ScriptRequest&) {return ScriptResponse{std::int32_t(1),false};};
    game.player_action=[](const ScriptRequest&) {return ScriptResponse{};};
    game.player_turn=[](const ScriptRequest&) {return ScriptResponse{};};
    unsigned walks{};
    game.player_walk=[&](const ScriptRequest& request) {
        if(request.call.system_id!=257 || request.call.arguments.size()!=5)
            throw std::runtime_error("unexpected M01 player movement service");
        const auto& args=request.call.arguments;
        if(std::get<std::int32_t>(args[0])!=0 || std::get<float>(args[1])!=-353.0f ||
            std::get<float>(args[2])!=40.0f || std::get<float>(args[3])!=2282.0f ||
            std::get<bool>(args[4]))throw std::runtime_error("original M01 walk arguments changed");
        ++walks;return ScriptResponse{{},true};
    };
    struct ControlledServices final:IScriptContextServices {
        GameServices& game;
        explicit ControlledServices(GameServices& value):game(value) {}
        void OnCall(const ScriptRequest& request) override {game.OnCall(request);}
        ScriptResponse Invoke(const ScriptRequest& request) override {
            switch(request.call.system_id) {
            case 66:case 74:case 75:case 81:case 91:case 97:case 103:case 104:case 105:
            case 118:case 157:case 160:case 180:case 216:case 217:case 245:case 246:case 250:
            case 253:case 255:case 257:case 258:case 267:case 287:case 288:case 290:
                return game.Invoke(request);
            default:return {};
            }
        }
    } services(game);
    if(!stack.Push("M01","void func2001()",services))
        throw std::runtime_error("original M01 func2001 entry missing");
    auto state=ScriptState::prepared;
    for(unsigned frame=0;frame<10000 && !walks && state!=ScriptState::failed && state!=ScriptState::finished;++frame)
        state=stack.Tick(1.0/60.0,100000,ScriptRunMode::original_steps);
    if(state==ScriptState::failed)
        throw std::runtime_error("M01 func2001 failed before the player walk route: "+stack.Error());
    if(walks!=1 || game.last_service!=257 || stack.State()!=ScriptState::suspended ||
        stack.WaitKind()!=ScriptWaitKind::external || !stack.Pending() || globals[0]!=10300)
        throw std::runtime_error("M01 player walk request did not suspend with its original token");
    const auto request=*stack.Pending();Check(request.context==stack.TopId(),"M01 walk token lost its script context");
    arena_skills.Close();
    stack.Clear();
    std::cout<<"original M01 func2001 passed service 180 on an empty manager and reached giPlayerWalkTo(257) with a suspended token; story="
        <<globals[0]<<"\n";
}

void RunOriginalM01VisibleObjectQuery(std::string_view path) {
    const auto game_data=std::filesystem::path(path).parent_path();
    ArenaSource source(game_data);
    const auto database=ReadOriginalDatabasePackage(game_data);
    auto arena=LoadArenaResources("M01","1",[&](std::string_view member){return source.Read(member);},
        [&]{return source.ReadOriginalCollision("M01","1");},true,
        [&](const SceneGob& gob){
            const auto dff=source.ReadObjectMember(gob.resource_directory,gob.model+".dff");
            if(!dff)throw std::runtime_error("original M01 object collision DFF is missing");
            return ReadDffObjectCollision(*dff);
        },
        [&]{return source.ReadOriginalDescription("M01","1",database.scenes);},
        [&](std::string_view kind){return source.ReadOriginalEnvironment("M01","1",kind);},
        [&]{return source.ReadOriginalWorld("M01","1");},
        [&](const SceneNpc& npc){return source.ReadOriginalActorResource("gamedata\\PALActor\\"+npc.resource+"\\",npc.resource,{},true);},
        [&](const SceneGob& gob){return source.ReadOriginalActorResource(gob.resource_directory,gob.model,gob.variant,true);});
    std::ifstream ui_stream(game_data/"ui.cpk",std::ios::binary);
    if(!ui_stream)throw std::runtime_error("cannot open original ui.cpk");
    PackageReader ui_package(ui_stream);
    SceneMinimap minimap("M01","1",[&](std::string_view member){return source.ReadSceneMember(member);},
        [&](std::string_view member)->std::optional<std::vector<std::uint8_t>> {
            std::string key(member);std::replace(key.begin(),key.end(),'/','\\');
            if(!ui_package.Find(key))return {};
            return ui_package.Read(key);
        });
    SceneObjects objects(arena,minimap,[&](auto directory,auto member){return source.ReadObjectMember(directory,member);});
    const auto states=objects.States();
    std::ifstream input(std::string(path),std::ios::binary);
    if(!input)throw std::runtime_error("cannot open original script.cpk");
    PackageReader scripts(input);const auto bytes=scripts.Read("M01.csb");
    ScriptGlobals globals{};globals[0]=10200;ScriptStack stack(globals);stack.LoadCsb("M01",bytes,DecodeGbk);
    std::ostringstream trace;GameServices game(trace);unsigned queries{},resets{};bool result{},expected{},present{};std::string query_name;
    game.get_object_visible=[&](std::string_view name) {
        ++queries;query_name=std::string(name);
        const auto found=std::find_if(states.begin(),states.end(),[&](const auto& object){return object.name==name;});
        present=found!=states.end();expected=present && found->visible!=0;result=objects.IsVisible(name);
        if(result!=expected)throw std::runtime_error("SceneObjects visibility changed during the original query loop");
        return result;
    };
    game.reset_object=[&](std::string_view name){++resets;(void)objects.ResetConfiguration(name);};
    if(!stack.Push("M01","void func9001()",game))throw std::runtime_error("original func9001 entry missing");
    const auto state=stack.Tick(0,100000,ScriptRunMode::until_wait);
    if(!queries || result!=expected)
        throw std::runtime_error("original func9001 SceneObjects query mismatch: queries="+
            std::to_string(queries)+" name="+query_name+" result="+std::to_string(result)+
            " expected="+std::to_string(expected)+" present="+std::to_string(present)+
            " state="+std::to_string(static_cast<unsigned>(state)));
    if(expected) {
        Check(state==ScriptState::finished && queries==100,
            "visible original GOB did not follow the 100-query path");
        Check(stack.PopFinished(),"visible func9001 context was not retired");
    } else {
        Check(queries==1 && resets==1,
            "hidden or missing original GOB did not invoke the real GOB reset owner");
        stack.Clear();
    }
    std::cout<<"original M01 func9001 queried real M01/1 SceneObjects "<<query_name<<" visible="
        <<(result?1:0)<<" present="<<(present?1:0)<<" queries="<<queries
        <<" resets="<<resets<<" state="<<static_cast<unsigned>(state)
        <<(stack.Error().empty()?std::string{}:" error="+stack.Error())
        <<"; no scene was committed\n";
}
}

int main(int argc,char** argv) {
    try {
        if(argc==3 && std::string_view(argv[1])=="--m01-walk-cpk") {
            RunOriginalM01PlayerWalk(argv[2]);return 0;
        }
        if(argc==3 && std::string_view(argv[1])=="--m01-visible-object-cpk") {
            RunOriginalM01VisibleObjectQuery(argv[2]);return 0;
        }
        if(argc==3 && std::string_view(argv[1])=="--original-cpk") {
            RunOriginalM01Timer(argv[2]);return 0;
        }
        if(argc!=1)throw std::invalid_argument("usage: pal4_portable_time_script_tests [--original-cpk script.cpk|--m01-walk-cpk script.cpk|--m01-visible-object-cpk script.cpk]");
        std::vector<std::string> callbacks;
        TimeScript timer([&](std::string_view name) { callbacks.emplace_back(name); });
        std::ostringstream trace;
        GameServices services(trace);
        services.start_time_script = [&](float delay, std::string_view callback) {
            timer.Start(delay, callback);
        };
        services.terminate_time_script = [&] { timer.Terminate(); };

        const ScriptCall start{216, 10200, {180.0f, std::string("func9001")}};
        Check(!services.Invoke(Request(start)).suspend);
        Check(timer.Active() && timer.Delay() == 180.0f && timer.Elapsed() == 0.0f &&
            timer.Callback() == "func9001");

        Check(!timer.Advance(179.5f, true, false) && callbacks.empty());
        Check(timer.Advance(.5f, true, false) && callbacks == std::vector<std::string>{"func9001"});
        Check(timer.Elapsed() == 0.0f);
        Check(!timer.Advance(90.0f, true, false));
        Check(timer.Advance(270.0f, true, false) && callbacks.size() == 2);

        timer.Start(2.0f, "replaced");
        timer.Start(1.0f, "func_busy");
        Check(!timer.Advance(.5f, false, false));
        Check(timer.Advance(.5f, false, true) && callbacks.size() == 2 && timer.Elapsed() == 0.0f);
        Check(!timer.Advance(.5f, false, false));
        Check(timer.Advance(.5f, false, false) && callbacks.back() == "func_busy");

        const ScriptCall terminate{217, 10200, {}};
        Check(!services.Invoke(Request(terminate)).suspend && !timer.Active());
        Check(!timer.Advance(1000.0f, true, false) && callbacks.size() == 3);
        Reject([&] { timer.Start(std::numeric_limits<float>::infinity(), "bad"); });
        Reject([&] { timer.Start(1.0f, std::string_view("bad\0name", 8)); });
        Reject([&] { timer.Advance(-1.0f, true, false); });

        unsigned movement_calls{};services.player_walk=[&](const ScriptRequest&) {
            ++movement_calls;return ScriptResponse{{},true};
        };
        const ScriptCall walk{257,10200,{std::int32_t(0),-353.0f,40.0f,2282.0f,false}};
        const ScriptCall run{258,10200,{std::int32_t(0),-353.0f,40.0f,2282.0f,false}};
        const ScriptCall end_move{267,10200,{std::int32_t(0)}};
        Check(services.Invoke(Request(walk)).suspend && services.Invoke(Request(run)).suspend &&
            services.Invoke(Request(end_move)).suspend && movement_calls==3,
            "walk, run and end-move services did not share the player movement owner");

        const ScriptCall visible_query{154,10200,{std::string("item202")}};
        GameServices visibility_missing(trace);
        Reject([&] {(void)visibility_missing.Invoke(Request(visible_query));});
        services.get_object_visible=[](std::string_view name){return name=="item202";};
        Check(std::get<bool>(services.Invoke(Request(visible_query)).value) &&
            !std::get<bool>(services.Invoke(Request(ScriptCall{154,10200,{std::string("missing")}})).value),
            "object visibility query did not return its owner state or missing=false");

        GameServices missing(trace);
        Reject([&] { (void)missing.Invoke(Request(start)); });
        missing.start_time_script = [](float, std::string_view) {};
        const ScriptCall invalid_start{216, 0, {180.0f}};
        Reject([&] { (void)missing.Invoke(Request(invalid_start)); });
        missing.terminate_time_script = [] {};
        const ScriptCall invalid_terminate{217, 0, {true}};
        Reject([&] { (void)missing.Invoke(Request(invalid_terminate)); });

        std::cout << "M01 giTimeScript and terminate timer contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
