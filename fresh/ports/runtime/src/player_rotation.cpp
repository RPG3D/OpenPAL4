#include "pal4/portable/player_rotation.h"
#include "pal4/script_player_rotation.h"
#include <cmath>
#include <iomanip>
#include <ostream>
#include <stdexcept>
namespace pal4::portable {
namespace {[[noreturn]] void Missing(){throw std::logic_error("unbound indexed/NPC player rotation service");}}
struct PlayerRotation::Impl final:pal4::IScriptPlayerRotationRuntime {
    PlayerRoster& players;PlayerNotifications& notifications;
    unsigned calls{};float requested{};bool completed{};
    Impl(PlayerRoster& p,PlayerNotifications& n):players(p),notifications(n){}
    std::uint32_t Handle() const{return 0x10000+players.Leader()*0xb14;}
    void Require(std::uint32_t actor) const{if(!players.Prepared() || actor!=Handle())throw std::logic_error("invalid rotation player handle");}
    std::uint32_t PlayerManager() override{return 0x8000;}
    std::uint32_t CurrentPlayer(std::uint32_t manager) override {
        if(manager!=0x8000 || !players.Prepared())throw std::logic_error("rotation player manager is unavailable");
        return Handle();
    }
    std::uint32_t PlayerByIndex(std::uint32_t,std::int32_t) override{Missing();}
    std::uint32_t NpcManager() override{Missing();}
    std::uint32_t NpcByName(std::uint32_t,std::string_view) override{Missing();}
    std::string_view ActorName(std::uint32_t actor) override{Require(actor);return players.Slots()[players.Leader()].name;}
    float ReadX(std::uint32_t actor) override{Require(actor);return players.Slots()[players.Leader()].rotation[0];}
    float ReadZ(std::uint32_t actor) override{Require(actor);return players.Slots()[players.Leader()].rotation[2];}
    void SetRotation(std::uint32_t actor,const pal4::PlayerRotation& rotation,std::uint32_t mode) override {
        Require(actor);if(mode)throw std::logic_error("invalid player rotation mode");
        players.SetRotation(players.Leader(),{rotation.x,rotation.yaw,rotation.z});
    }
    pal4::PlayerRotationWaitState* Allocate(std::uint32_t size) override{return notifications.Allocate(size);}
    std::uint32_t ScriptExecutor() override{return PlayerNotifications::executor;}
    void Construct(pal4::PlayerRotationWaitState& state,std::uint32_t mode,std::uint32_t queue) override{notifications.Construct(state,mode,queue);}
    bool AllocateText(pal4::PlayerRotationWaitState& state,std::uint32_t size) override{return notifications.AllocateText(state,size);}
    void AssignName(pal4::PlayerRotationWaitState& state,std::string_view name) override{notifications.AssignName(state,name,2,pal4::ScriptPlayerRotation::kVtable);}
    void Close(pal4::PlayerRotationWaitState& state) override{notifications.Close(state);}
    void AddHandler(std::uint32_t queue,pal4::PlayerRotationWaitState* state) override{notifications.Add(queue,state);}
    std::uint32_t CleanupName() override{return 1;}
};
PlayerRotation::PlayerRotation(PlayerRoster& p,PlayerNotifications& n):impl_(std::make_unique<Impl>(p,n)){}
PlayerRotation::~PlayerRotation()=default;
void PlayerRotation::SetCurrentAngle(float angle) {
    if(!std::isfinite(angle))throw std::invalid_argument("non-finite player angle");
    ++impl_->calls;impl_->requested=angle;impl_->completed=false;
    pal4::ScriptPlayerRotation script(*impl_);(void)script.SetCurrentAngle(angle);impl_->completed=true;
}
void PlayerRotation::WriteReport(std::ostream& output) const {
    const auto& s=*impl_;const auto& player=s.players.Slots()[s.players.Leader()];
    output<<std::setprecision(9)<<"{\"calls\":"<<s.calls<<",\"requested\":"<<s.requested<<",\"completed\":"<<(s.completed?"true":"false")
        <<",\"rotation\":["<<player.rotation[0]<<','<<player.rotation[1]<<','<<player.rotation[2]<<"],\"waits\":"<<s.notifications.Count(2)<<'}';
}
}
