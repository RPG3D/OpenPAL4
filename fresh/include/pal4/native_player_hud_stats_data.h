#pragma once

#include "pal4/minimap_scene_ui_stats.h"
#include "pal4/player_manager_object.h"
#include "pal4/game_object_property_access.h"

namespace pal4 {

// Borrows the session's four owned player slots; no calls into EXE vtables.
class NativePlayerHudStatsData final : public IMinimapSceneUiStatsDataRuntime {
public:
    explicit NativePlayerHudStatsData(PlayerManagerObjectState& players)
        : players_(players) {}
    [[nodiscard]] std::uint32_t PlayerManager() override;
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
    [[nodiscard]] std::int32_t PlayerStat(std::uint32_t state, std::uint32_t index) override;
    [[nodiscard]] std::int32_t PlayerMaxStat(std::uint32_t state, std::uint32_t index) override;
    [[nodiscard]] std::uint32_t PlayerIdentity(std::uint32_t state);
    [[nodiscard]] std::int32_t PlayerMode(std::uint32_t state) override;
private:
    [[nodiscard]] GameObjectPropertyState Properties(std::uint32_t address);
    PlayerManagerObjectState& players_;
};

}  // namespace pal4
