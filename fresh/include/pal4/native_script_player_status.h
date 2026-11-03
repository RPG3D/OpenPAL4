#pragma once
#include "pal4/script_player_status.h"
#include "pal4/script_party_recovery.h"
#include "pal4/script_party_magic_query.h"
#include "pal4/script_player_favor.h"
#include "pal4/script_forbidden_skill.h"
#include <memory>

namespace pal4 {
struct PlayerManagerObjectState;
class NativeBaseObjectMemory;
class NativeGameDatabase;
class NativePlayerEventRuntime;
class NativePlayerScriptRefreshRuntime;

// Borrows persistent player, growth and allocation owners; owns typed Stunt
// learning requirements. Does not own scene actors or change leader/visibility.
class NativeScriptPlayerStatus final : public IScriptPlayerStatusRuntime,
    public IScriptPartyRecoveryRuntime, public IScriptPartyMagicQueryRuntime,
    public IScriptPlayerFavorRuntime, public IScriptForbiddenSkillRuntime {
public:
    NativeScriptPlayerStatus(PlayerManagerObjectState& players,
        NativeBaseObjectMemory& memory, const NativeGameDatabase& database,
        NativePlayerEventRuntime& events, NativePlayerScriptRefreshRuntime& refresh);
    ~NativeScriptPlayerStatus();
    std::uint32_t PlayerManager() override;
    std::uint32_t FindPlayer(std::uint32_t manager, std::int32_t index) override;
    std::uint8_t SetPresent(std::uint32_t collection, std::int32_t skill,
                          std::uint32_t enabled) override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    std::uint32_t PlayerByIndex(std::uint32_t manager, std::int32_t index) override;
    std::uint32_t PlayerByIndex(std::uint32_t manager, std::uint32_t index) override;
    bool HasAnyValues(std::uint32_t collection) override;
    std::uint32_t ReadMaxStat(std::uint32_t script, std::uint32_t stat) override;
    std::uint32_t WriteCurrentStat(std::uint32_t script, std::uint32_t stat, std::uint32_t value) override;
    void WriteLevel(std::uint32_t script, std::uint32_t stat, std::int32_t level) override;
    void RefreshPlayerState(std::uint32_t script, std::int32_t level, std::uint32_t player_state) override;
    std::uint32_t BuildMatchingRecords(std::uint32_t collection, std::int32_t level, std::uint32_t player_state) override;
    std::uint32_t FreeRecords(std::uint32_t buffer) noexcept override;
private:
    void ValidatePlayer(std::uint32_t player);
    class Learning;
    PlayerManagerObjectState& players_;
    NativeBaseObjectMemory& memory_;
    NativePlayerEventRuntime& events_;
    NativePlayerScriptRefreshRuntime& refresh_;
    std::unique_ptr<Learning> learning_;
};
}
