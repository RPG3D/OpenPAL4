#include "pal4/portable/player_turn.h"
#include "pal4/portable/player_notifications.h"
#include "pal4/script_player_turn.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <ostream>
#include <stdexcept>
namespace pal4::portable {
namespace {[[noreturn]] void Missing(){throw std::logic_error("player turn NPC/object facing endpoint is not bound");}}
struct PlayerTurns::Impl final:pal4::IScriptPlayerTurnRuntime {
    struct Handler {pal4::PlayerTurnWaitState state{};ScriptToken token;std::string name;bool blocking{},closed{};float wall_time{};};
    PlayerRoster& players;PlayerActions& actions;
    std::unique_ptr<Handler> pending;std::vector<std::unique_ptr<Handler>> handlers;
    std::vector<PlayerTurnCompletion> completions;ScriptToken request_token{};
    unsigned calls{},completed{},blocking_completed{},index{};float target{},last_elapsed{},last_duration{},last_start{},last_delta{},last_wall{};bool wait{};
    Impl(PlayerRoster& p,PlayerActions& a):players(p),actions(a){}
    unsigned Index(unsigned actor) const {
        if(actor<0x10000 || (actor-0x10000)%0xb14 || (actor-0x10000)/0xb14>=4 || !players.Prepared())throw std::logic_error("invalid turn actor handle");
        return (actor-0x10000)/0xb14;
    }
    unsigned PlayerManager() override{return 0x8000;}
    unsigned PlayerByIndex(unsigned manager,int i) override{if(manager!=0x8000)throw std::logic_error("invalid turn manager");return players.Prepared() && i>=0 && i<4?0x10000+static_cast<unsigned>(i)*0xb14:0;}
    unsigned CurrentPlayer(unsigned manager) override{return PlayerByIndex(manager,static_cast<int>(players.Leader()));}
    int CurrentPlayerIndex(unsigned manager) override{if(manager!=0x8000)throw std::logic_error("invalid turn manager");return static_cast<int>(players.Leader());}
    unsigned ObjectManager() override{Missing();}
    unsigned ObjectByName(unsigned,std::string_view) override{Missing();}
    unsigned NpcManager() override{Missing();}
    unsigned NpcByName(unsigned,std::string_view) override{Missing();}
    unsigned NpcInfoByName(unsigned,std::string_view) override{Missing();}
    float BaselineYaw(unsigned) override{Missing();}
    pal4::PlayerTurnVector ReadPosition(unsigned) override{Missing();}
    void ClampFacingVector(pal4::PlayerTurnVector&) override{Missing();}
    float FacingAngleDegrees(const pal4::PlayerTurnVector&) override{Missing();}
    pal4::PlayerTurnWaitState* Allocate(unsigned size) override {
        if(pending || size!=sizeof(pal4::PlayerTurnWaitState))throw std::logic_error("invalid turn allocation");
        pending=std::make_unique<Handler>();pending->token=request_token;return &pending->state;
    }
    unsigned ScriptExecutor() override{return PlayerNotifications::executor;}
    void Require(const pal4::PlayerTurnWaitState& s) const{if(!pending || &pending->state!=&s)throw std::logic_error("invalid pending turn");}
    void Construct(pal4::PlayerTurnWaitState& s,std::uint8_t mode,unsigned queue) override{Require(s);if(mode>1 || queue!=PlayerNotifications::queue)throw std::logic_error("invalid turn queue");pending->blocking=mode!=0;}
    void Flush(pal4::PlayerTurnWaitState& s) override{Require(s);}
    bool AllocateText(pal4::PlayerTurnWaitState& s,unsigned) override{Require(s);return true;}
    void AssignName(pal4::PlayerTurnWaitState& s,std::string_view name) override{Require(s);pending->name=name;}
    float ReadYaw(unsigned actor) override{return players.Slots()[Index(actor)].rotation[1];}
    float ReadX(unsigned actor) override{return players.Slots()[Index(actor)].rotation[0];}
    float ReadZ(unsigned actor) override{return players.Slots()[Index(actor)].rotation[2];}
    void SetRotation(unsigned actor,float x,float yaw,float z,unsigned mode) override{if(mode)throw std::logic_error("invalid turn rotation mode");players.SetRotation(Index(actor),{x,yaw,z});}
    int SetAction(unsigned actor,std::string_view action) override{return actions.SelectTaskAction(Index(actor),action)?1:0;}
    int SetTimedAction(unsigned actor,std::string_view action,float blend) override{return actions.SelectTaskAction(Index(actor),action,blend)?1:0;}
    void CleanupWeaponEffects(unsigned actor) override{actions.CleanupTaskWeaponEffects(Index(actor));}
    void Close(pal4::PlayerTurnWaitState& s) override{
        if(pending && &pending->state==&s){pending->closed=true;return;}
        for(auto& h:handlers)if(&h->state==&s){h->closed=true;return;}
        throw std::logic_error("turn close outside queue");
    }
    void AddHandler(unsigned queue,pal4::PlayerTurnWaitState* s) override{
        if(!s)throw std::logic_error("null turn handler");
        Require(*s);
        if(queue!=PlayerNotifications::queue || s->vtable_address!=pal4::ScriptPlayerTurn::kVtable || s->phase!=2)throw std::logic_error("invalid turn notification");
        // 同名转向保留最新请求；跨处理器的完整运动队列仍需接入。
        for(const auto& h:handlers)if(h->name==pending->name && h->blocking)completions.push_back({h->token,"player turn replaced"});
        std::erase_if(handlers,[&](const auto& h){return h->name==pending->name;});handlers.push_back(std::move(pending));
    }
    unsigned CleanupName() override{return 0;}
};
PlayerTurns::PlayerTurns(PlayerRoster& p,PlayerActions& a):impl_(std::make_unique<Impl>(p,a)){}
PlayerTurns::~PlayerTurns()=default;
ScriptResponse PlayerTurns::Start(ScriptToken token,std::int32_t index,float target,bool wait){
    if(!std::isfinite(target))throw std::invalid_argument("non-finite turn angle");
    auto& s=*impl_;++s.calls;s.index=index<0?4:static_cast<unsigned>(index);s.target=target;s.wait=wait;s.request_token=token;
    const auto before=s.handlers.size();(void)pal4::ScriptPlayerTurn(s).Start(index,target,wait?1:0);
    const bool created=s.handlers.size()>before || (!s.handlers.empty() && s.handlers.back()->token==token);
    return {{},created && wait};
}
std::vector<PlayerTurnCompletion> PlayerTurns::Advance(float elapsed){
    if(!std::isfinite(elapsed) || elapsed<0)throw std::invalid_argument("invalid turn elapsed");
    auto& s=*impl_;pal4::ScriptPlayerTurn turn(s);
    for(auto& h:s.handlers){
        h->wall_time+=elapsed;if(!h->closed)turn.Update(h->state,elapsed);
        s.last_elapsed=h->state.elapsed;s.last_duration=h->state.duration;s.last_start=h->state.start_yaw;s.last_delta=h->state.delta;s.last_wall=h->wall_time;
        if(h->closed){++s.completed;if(h->blocking){++s.blocking_completed;s.completions.push_back({h->token,{}});}}
    }
    std::erase_if(s.handlers,[](const auto& h){return h->closed;});auto result=std::move(s.completions);s.completions.clear();return result;
}
void PlayerTurns::WriteReport(std::ostream& out) const{
    const auto& s=*impl_;out<<std::setprecision(9)<<"{\"calls\":"<<s.calls<<",\"completed\":"<<s.completed<<",\"blocking_completed\":"<<s.blocking_completed<<",\"active\":"<<s.handlers.size()
        <<",\"player\":"<<s.index<<",\"target\":"<<s.target<<",\"wait\":"<<(s.wait?"true":"false")<<",\"wall_time\":"<<s.last_wall
        <<",\"elapsed\":"<<s.last_elapsed<<",\"duration\":"<<s.last_duration<<",\"start_yaw\":"<<s.last_start<<",\"delta\":"<<s.last_delta<<",\"yaw\":"<<(s.index<4?s.players.Slots()[s.index].rotation[1]:0)<<'}';
}
}
