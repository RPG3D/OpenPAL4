#pragma once
#include "pal4/portable/actor_playback.h"
#include "pal4/portable/player.h"
#include "pal4/portable/script_stack.h"
namespace pal4::portable {
struct PlayerActionCompletion {ScriptToken token;std::string error;};
class PlayerActions {
public:
    PlayerActions(PlayerRoster& players,ActorMemberReader read);
    ~PlayerActions();
    PlayerActions(const PlayerActions&)=delete;
    PlayerActions& operator=(const PlayerActions&)=delete;
    ScriptResponse Start(ScriptToken token,std::int32_t index,std::string_view action,std::int32_t execute_flag,bool wait);
    void Unhold(std::int32_t index);
    bool SelectTaskAction(std::uint32_t index,std::string_view action,float blend=0);
    void SetTaskRestart(std::uint32_t index,bool enabled);
    void SetTaskSpeed(std::uint32_t index,float speed);
    void CleanupTaskWeaponEffects(std::uint32_t index);
    ScriptResponse End(ScriptToken token,std::int32_t index);
    std::vector<PlayerActionCompletion> Advance(float elapsed);
    const ActorPlayback* Playback(std::uint32_t index) const;
    void Sample(std::uint32_t index,std::vector<Vertex>& output,std::vector<Matrix>* frames=nullptr);
    void WriteReport(std::ostream& output,bool include_source=true) const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
