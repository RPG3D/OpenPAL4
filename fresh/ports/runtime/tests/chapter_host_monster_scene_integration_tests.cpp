#include "pal4/portable/chapter_host.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace pal4::portable;

namespace {
class PassThrough final:public IScriptContextServices {
public:
    ScriptResponse Invoke(const ScriptRequest&) override{return {};}
};

void Check(bool value,const char* message) {
    if(!value)throw std::runtime_error(message);
}

std::vector<SceneMonster> ReadMonsters(ArenaSource& source) {
    const auto bytes=source.ReadSceneMember("M01\\1\\mstInfo.mst");
    Check(bytes.has_value(),"original M01/1 mstInfo.mst is available");
    std::istringstream input(std::string(bytes->begin(),bytes->end()));
    return ReadSceneMonsters(input);
}

std::vector<ScenePath> ReadPaths(ArenaSource& source) {
    const auto bytes=source.ReadSceneMember("M01\\1\\pathway.pwy");
    Check(bytes.has_value(),"original M01/1 pathway.pwy is available");
    std::istringstream input(std::string(bytes->begin(),bytes->end()));
    return ReadScenePaths(input);
}

void ArenaRequestsReceiveMonotonicSceneGenerations() {
    PassThrough next;
    ArenaServices services(next);
    const ScriptCall load{65,0,{std::string("M01"),std::string("1"),std::string(""),false}};
    const auto request=[&](std::uint64_t sequence) {
        Check(services.Invoke({{1,sequence},load}).suspend,"arena request suspends for host loading");
        const auto command=services.TakeCommand();
        Check(command.has_value(),"arena command is dispatched once");
        Check(services.Finish(command->token),"arena command completes");
        return command->generation;
    };
    Check(request(1)==1 && request(2)==2,"scene generations advance across completed arena requests");
}

void ChapterHostStagesSupportedEmptyArenaOwner() {
    ChapterHost host;
    host.game_data="integration-test-data";
    host.arena_source=std::make_shared<ArenaSource>(host.game_data);
    ArenaResources arena;
    arena.monsters_verified_empty=true;
    PrepareArenaGameplayOwners(host,arena,1);
    Check(host.staged_monster_scene_runtime!=nullptr,"ChapterHost stages the owner after arena resources load");
    Check(!host.staged_monster_scene_runtime->Closed(),"staged runtime is live");
    Check(host.staged_monster_scene_runtime->Monsters().Size()==0,"empty arena remains empty");
    Check(host.staged_monster_scene_runtime->Resources().Paths()->Size()==0,"empty arena has no fabricated pathways");
    host.monster_scene_runtime=std::move(host.staged_monster_scene_runtime);
    Check(host.monster_scene_runtime->World().Stats().helpers==0,"no gameplay helper was fabricated");
    host.distance=std::make_unique<DistanceOptimization>();
    UploadMonsterGraphics(host);
    Check(host.monster_graphics && host.monster_graphics->Uploaded(),"committed empty scene owns its graphics lifecycle");
    host.monster_graphics->Abandon();
    UploadMonsterGraphics(host);
    Check(host.monster_graphics->Uploaded(),"empty scene restores graphics without fabricating a render instance");
    ReleaseMonsterGraphics(host);
    Check(!host.monster_graphics,"graphics retire before their borrowed scene resources");
    host.monster_scene_runtime->Close();
    host.monster_scene_runtime.reset();
}

void LoadingPresentationClosesOnLoadOrPresenterFailure() {
    for(const bool presenter_fails:{false,true}) {
        ChapterHost host;PassThrough next;
        host.arena_source=std::make_shared<ArenaSource>("unread-loading-test-data");
        host.game_arena=std::make_unique<ArenaServices>(next);
        host.game=std::make_unique<ScriptStack>(host.game_globals);
        // Source compilation is a test fixture only. Production keeps reading CSB.
        host.game->Compile("loading_test",R"(
            void Run(){global_mvar=7;giArenaLoad("Q01","N01","unsupported_variant",true);global_mvar=99;}
        )");
        Check(host.game->Push("loading_test","void Run()",*host.game_arena).has_value(),"loading fixture starts");
        Check(host.game->Tick(0)==ScriptState::suspended,"loading fixture waits at original service");
        const auto wait=host.game->Pending();
        std::vector<std::optional<float>> presentations;
        host.present_loading=[&](std::optional<float> progress){
            presentations.push_back(progress);
            Check(host.game_globals[0]==7 && host.game->Pending()==wait,"presentation advanced or replaced the script wait");
            if(progress && presenter_fails)throw std::runtime_error("injected loading presenter failure");
        };
        AdvanceArenaLoad(host);
        Check(presentations.size()==2 && presentations.front()==0.f && !presentations.back(),
              "loading presents before resource work and closes on every failure path without fake completion");
        Check(!host.loading_visible && host.loading_progress==0.f && host.loading_presentations==(presenter_fails?0u:1u),
              "loading diagnostics count only successful presentations");
        Check(!host.arena && !host.game_arena->Waiting(),"loading failure does not commit a scene or leave its command pending");
        Check(host.game->State()==ScriptState::failed && host.game_globals[0]==7,
              "loading failure completes the existing wait with an error without running the next statement");
    }
}

void ChapterHostStagesRealM01OwnerAndKeepsGameplayGate(const std::string& game_data) {
    ChapterHost host;
    host.game_data=game_data;
    host.arena_source=std::make_shared<ArenaSource>(host.game_data);
    ArenaResources arena;
    arena.catalog.monsters=ReadMonsters(*host.arena_source);
    arena.catalog.paths=ReadPaths(*host.arena_source);
    arena.monsters_verified_empty=arena.catalog.monsters.empty();
    Check(arena.catalog.monsters.size()==8,"M01/1 has its original eight monster records");
    Check(arena.catalog.paths.size()==8,"M01/1 has its original eight pathways");

    StageMonsterSceneRuntime(host,1,arena.catalog.monsters,arena.catalog.paths);
    Check(host.staged_monster_scene_runtime!=nullptr,"ChapterHost owns a staged monster scene runtime");
    Check(host.staged_monster_scene_runtime->Monsters().Size()==arena.catalog.monsters.size(),
          "runtime owns all original MST instances");
    Check(host.staged_monster_scene_runtime->Monsters().ModelCount()!=0,
          "runtime loads original monster models from palactor.cpk");
    Check(host.staged_monster_scene_runtime->Resources().Paths()->Size()==arena.catalog.paths.size(),
          "runtime owns all original scene pathways");
    host.staged_monster_scene_runtime->Close();
    host.staged_monster_scene_runtime.reset();

    const std::vector<SceneMonster> no_monsters;
    const std::vector<ScenePath> no_paths;
    StageMonsterSceneRuntime(host,1,no_monsters,no_paths);
    host.monster_scene_runtime=std::move(host.staged_monster_scene_runtime);
    Check(host.monster_scene_runtime->World().Stats().helpers==0,
          "previous test owner has not fabricated gameplay helpers");

    std::string rejection;
    try {
        PrepareArenaGameplayOwners(host,arena,2);
    } catch(const std::exception& error) {
        rejection=error.what();
    }
    Check(rejection=="populated monster scene owner is not implemented",
          "original arena gameplay gate rejects the populated scene after staging");
    Check(!host.staged_monster_scene_runtime,
          "rejected staged monster runtime is closed and released");
    Check(host.monster_scene_runtime && !host.monster_scene_runtime->Closed(),
          "pre-commit rejection preserves the previous active runtime");
    Check(host.monster_scene_runtime->Monsters().Size()==0,
          "previous test owner remains usable after pre-commit rejection");
    host.monster_scene_runtime->Close();
    host.monster_scene_runtime.reset();
}
}

int main(int argc,char** argv) {
    try {
        if(argc==1) {
            ArenaRequestsReceiveMonotonicSceneGenerations();
            ChapterHostStagesSupportedEmptyArenaOwner();
            LoadingPresentationClosesOnLoadOrPresenterFailure();
        }
        else if(argc==3 && std::string_view(argv[1])=="--original-gamedata")
            ChapterHostStagesRealM01OwnerAndKeepsGameplayGate(argv[2]);
        else throw std::invalid_argument("expected no arguments or --original-gamedata <path>");
        return 0;
    } catch(const std::exception& error) {
        std::cerr<<error.what()<<'\n';
        return 1;
    }
}
