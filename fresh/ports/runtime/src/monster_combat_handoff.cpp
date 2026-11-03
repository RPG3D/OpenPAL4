#include "pal4/portable/monster_combat_handoff.h"
#include "pal4/game_combat_handoff.h"
#include <limits>
#include <stdexcept>

namespace pal4::portable {
namespace {void Require(bool value,const char* message){if(!value)throw std::logic_error(message);}}
struct MonsterCombatHandoff::Impl final:IGameCombatHandoffRuntime {
    IMonsterCombatHandoffServices& services;MonsterSceneActor* actor{};std::string arena;std::optional<std::int32_t> state;
    explicit Impl(IMonsterCombatHandoffServices& s):services(s){}
    std::uint32_t StateController() override{state=services.CurrentStateId();return state?1u:0u;}
    std::int32_t ControllerReady(std::uint32_t id) override{Require(id==1 && state.has_value(),"invalid combat state controller");return *state;}
    void CreateDocument(std::string_view root) override{services.CreateDocument(root);}
    std::string ArenaName() override{return arena;}
    std::string ResolveArenaPath(std::string_view name,std::uint8_t) override{return services.ResolveArenaPath(name);}
    void ClearStateText() override{services.ClearStateNames();}
    void AddStateText(std::uint32_t token) override{Require(actor,"combat handoff has no actor");services.AddStateName(token==EmptyTextAddress()?std::string{}:actor->Encounter().Name(token));}
    std::uint32_t ReadDword(std::uint32_t address) override{Require(actor,"combat handoff has no actor");return actor->Encounter().ReadWord(address);}
    std::uint32_t EmptyTextAddress() override{return std::numeric_limits<std::uint32_t>::max();}
    void FinalizeManagerText(MonsterBehaviorManagerState& image,std::uint32_t begin,std::uint32_t end) override{
        Require(actor && &image==&actor->State() && begin==image.words[0x724/4] && end==image.words[0x728/4],"invalid combat history owner");actor->Encounter().ConsumeNames();
    }
    std::uint32_t StateMonsterBegin() override{return 4;}
    std::uint32_t StateMonsterEnd() override{const auto count=services.StateNameCount();Require(count<=static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()/12),"too many combat state names");return 4+static_cast<std::uint32_t>(count)*12;}
    void SetCombatWorld(std::string_view node,std::string_view path,std::uint32_t mode,std::int32_t count) override{services.SetCombatWorld(node,path,mode,count);}
    void SetCombatMonster(std::uint32_t id,std::uint32_t value,std::uint32_t group) override{services.SetCombatMonster(id,value,group);}
    void InitializeCombatState(std::uint32_t id) override{services.EnterState(id);}
    std::int32_t DestroyDocument() override{return services.DestroyDocument();}
};
MonsterCombatHandoff::MonsterCombatHandoff(IMonsterCombatHandoffServices& services):impl_(std::make_unique<Impl>(services)){}
MonsterCombatHandoff::~MonsterCombatHandoff()=default;
std::int32_t MonsterCombatHandoff::Process(MonsterSceneActor& actor,std::int32_t mode,std::string_view arena){
    auto& s=*impl_;Require(!s.actor,"combat handoff cannot reenter");
    (void)actor.Encounter().Records();(void)actor.Encounter().Names();
    s.arena=arena;s.actor=&actor;struct Guard{Impl& s;~Guard(){s.actor=nullptr;s.arena.clear();s.state.reset();}} guard{s};
    return GameCombatHandoff::Process(actor.State(),mode,s);
}
}
