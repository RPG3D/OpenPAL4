#pragma once
#include "pal4/portable/player_action.h"
namespace pal4::portable {
struct PlayerTurnCompletion {ScriptToken token;std::string error;};
class PlayerTurns {
public:
    PlayerTurns(PlayerRoster& players,PlayerActions& actions);
    ~PlayerTurns();
    PlayerTurns(const PlayerTurns&)=delete;
    PlayerTurns& operator=(const PlayerTurns&)=delete;
    ScriptResponse Start(ScriptToken token,std::int32_t index,float angle,bool wait);
    std::vector<PlayerTurnCompletion> Advance(float elapsed);
    void WriteReport(std::ostream& output) const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
