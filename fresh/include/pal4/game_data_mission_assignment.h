#pragma once

#include "pal4/game_data_mission_selection.h"

#include <cstdint>

namespace pal4 {

class IGameDataMissionAssignmentRuntime
    : public IGameDataMissionSelectionRuntime {
public:
    [[nodiscard]] virtual std::uint32_t GameState() = 0;
    [[nodiscard]] virtual std::int32_t CurrentLevel(
        std::uint32_t game_state, std::uint32_t category) = 0;
    [[nodiscard]] virtual int ReleaseTemporaryStorage(
        std::uint32_t address) = 0;
};

class GameDataMissionAssignment final {
public:
    // IDA 0x4db500: choose the highest unlocked Mission for each game slot.
    [[nodiscard]] static int Refresh(
        std::uint32_t manager, GameDataMissionPointerArray& output,
        IGameDataMissionAssignmentRuntime& runtime);
};

}  // namespace pal4
