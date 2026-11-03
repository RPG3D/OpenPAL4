#include "pal4/combat_system_start.h"
#include "pal4/combat_monster_creation.h"
#include "pal4/combat_system_start_ready.h"

namespace pal4 {
namespace {
class LegacyStart final:public ICombatSystemStartServices {
    std::uint32_t object;ICombatSystemStartRuntime& runtime;bool players_ready{};
public:
    LegacyStart(std::uint32_t o,ICombatSystemStartRuntime& r):object(o),runtime(r){}
    void InitializeRequest(std::uint32_t mode) override{
        runtime.GetSkillSystem();
        runtime.ResetTaskFlags(runtime.GetActorManager()+96u);
        runtime.CleanupTaskQueue(runtime.GetActorManager()+96u);
        CombatSystemStart::InitializeRequestFields(object,mode,runtime);
    }
    std::int32_t ConfigChildCount(std::uint32_t c) override{return static_cast<std::int32_t>(runtime.ReadWord(c+56u));}
    std::optional<std::string> FindAttributeString(std::uint32_t c,std::string_view k) override{return runtime.FindAttributeString(c,k);}
    std::int32_t GetConfigInt(std::uint32_t c,std::string_view k,std::int32_t d) override{return runtime.GetConfigInt(c,k,d);}
    bool ColdInitialized() override{return runtime.ReadByte(object+517u)!=0u;}
    void InitializeCold() override{runtime.InitializeCold(object);}
    void ResetCollections() override{CombatSystemStart::ResetCollections(object,runtime);}
    void ClearRosters() override{runtime.ClearCombatRosters(runtime.GetActorManager());}
    void ResetConsole() override{CombatSystemStart::ResetConsoleServices(object,runtime);}
    bool Inactive() override{return runtime.ReadByte(runtime.GetCombatSystemInstance()+516u)!=0u;}
    void LoadWorld(std::string_view scene,std::string_view area) override{runtime.LoadWorld(object,scene,area);}
    void InitializeGameManager() override{runtime.InitializeGameManager(runtime.ReadWord(object+424u));}
    void ShowUi() override{runtime.ShowCombatUi(object);}
    void CreateMonsters(std::uint32_t c,std::int32_t count) override{
        class Monsters final:public ICombatMonsterCreationRuntime {
            ICombatSystemStartRuntime& r;
        public:
            explicit Monsters(ICombatSystemStartRuntime& r):r(r){}
            std::uint32_t GetChildByIndex(std::uint32_t c,std::uint32_t i) override{return r.GetChildByIndex(c,i);}
            std::int32_t GetConfigInt(std::uint32_t c,std::string_view k,std::int32_t d) override{return r.GetConfigInt(c,k,d);}
            void CreateMonster(std::int32_t id,std::uint32_t slot,std::int32_t ab) override{r.CreateMonster(r.GetActorManager(),id,slot,ab);}
        } monsters(runtime);
        CombatMonsterCreation::Create(c,count,monsters);
    }
    void CreatePlayers() override{
        std::vector<std::uint32_t> players;
        for(std::uint32_t index=0u;index!=4u;++index){
            const auto player=runtime.GetPlayer(index);
            if(player && runtime.ReadWord(player+2824u))players.push_back(player);
        }
        runtime.SortPlayers(players);
        const auto position=players.size()==1u?1u:6u;
        for(const auto player:players){runtime.PreparePlayer(player);runtime.CreatePlayerActor(runtime.GetActorManager()+32u,player,position);}
    }
    bool HasPlayers() override{
        const auto manager=runtime.GetActorManager(),begin=runtime.ReadWord(manager+52u),end=runtime.ReadWord(manager+56u);
        players_ready=begin!=0u && begin!=end;return players_ready;
    }
    bool CommitReady(std::int32_t type) override{
        class Ready final:public ICombatSystemStartReadyRuntime {
            std::uint32_t object;ICombatSystemStartRuntime& runtime;bool players_ready;
        public:
            Ready(std::uint32_t o,ICombatSystemStartRuntime& r,bool ready):object(o),runtime(r),players_ready(ready){}
            bool HasPlayers() override{return players_ready;} // Exact gate already evaluated by StartWithServices.
            void InstallInputCallback() override{CombatSystemStartRender::Install(object,runtime);}
            void PublishStateOne() override{runtime.WriteWord(object+508u,1u);}
            void DispatchPrepare(std::int32_t t) override{runtime.DispatchScriptEvent("combat_prepare",t);}
            void ResetAudioPosition() override{runtime.ResetAudioPosition();}
        } ready(object,runtime,players_ready);
        return CombatSystemStartReady::Commit(type,ready);
    }
};
}
bool CombatSystemStart::StartWithServices(std::uint32_t config,std::uint32_t mode,ICombatSystemStartServices& runtime){
    runtime.InitializeRequest(mode);
    if(!config)return false;
    const auto count=runtime.ConfigChildCount(config);if(count==0)return false;
    const auto scene=runtime.FindAttributeString(config,"CombatScene");if(!scene)return false;
    const auto area=runtime.FindAttributeString(config,"CombatArea");if(!area)return false;
    const auto type=runtime.GetConfigInt(config,"StartType",0);
    if(!runtime.ColdInitialized())runtime.InitializeCold();
    runtime.ResetCollections();runtime.ClearRosters();runtime.ResetConsole();
    if(!runtime.Inactive()){runtime.LoadWorld(*scene,*area);runtime.InitializeGameManager();}
    runtime.ShowUi();runtime.CreateMonsters(config,count);runtime.CreatePlayers();
    if(!runtime.HasPlayers())return false;
    return runtime.CommitReady(type);
}
bool CombatSystemStart::Start(std::uint32_t object,std::uint32_t config,std::uint32_t mode,ICombatSystemStartRuntime& runtime){
    LegacyStart services(object,runtime);return StartWithServices(config,mode,services);
}
}
