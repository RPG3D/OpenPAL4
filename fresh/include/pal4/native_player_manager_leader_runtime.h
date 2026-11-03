#pragma once
#include "pal4/player_manager_leader_transition.h"
namespace pal4 {
class NativePlayerManagerSession;
class NativeScriptPlayerActionsRuntime;
class INativeScriptPlayerLeaderServices;
// Adapter for441730; the shared algorithm retains ordering and early returns.
class NativePlayerManagerLeaderRuntime final : public IPlayerManagerLeaderRuntime {
public:
    NativePlayerManagerLeaderRuntime(NativePlayerManagerSession&,
        NativeScriptPlayerActionsRuntime&,INativeScriptPlayerLeaderServices*);
    std::uint32_t ReadDword(std::uint32_t)override;
    void WriteDword(std::uint32_t,std::uint32_t)override;
    void SetStateFlag(std::uint32_t,std::uint32_t)override;
    void SetAction(std::uint32_t,std::uint32_t)override;
    void CopyTransform(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)override;
    void SetPlayerState(std::uint32_t,std::uint32_t)override;
    std::uint32_t Arena()override;
    void OnRemove(std::uint32_t,std::uint32_t)override;
    void OnAdd(std::uint32_t,std::uint32_t)override;
    std::uint32_t ActorControl()override;
    void BindActor(std::uint32_t,std::uint32_t)override;
    void SortActive(std::vector<std::uint32_t>&,std::uint32_t)override;
    std::int32_t GetPriority(std::uint32_t)override;
    void SetPriority(std::uint32_t,std::int32_t)override;
private:
    void RequirePlayer(std::uint32_t)const;
    INativeScriptPlayerLeaderServices& Host()const;
    NativePlayerManagerSession& players_;
    NativeScriptPlayerActionsRuntime& scene_;
    INativeScriptPlayerLeaderServices* host_;
};
}
