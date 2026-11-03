#pragma once

#include "pal4/role_state_window_selection.h"
#include "pal4/role_state_window_player_swap.h"

#include <cstdint>

namespace pal4 {

class IRoleStateWindowButtonRuntime :
    public IRoleStateWindowSelectionRuntime {
public:
    virtual ~IRoleStateWindowButtonRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void HandlePlayerSwap(RoleStateWindowState& state,
                                  std::uint32_t target_index,
                                  std::uint32_t selected_index) = 0;
};

class RoleStateWindowButtonUp final {
public:
    [[nodiscard]] static bool Handle(
        RoleStateWindowState& state, std::uint32_t event,
        IRoleStateWindowButtonRuntime& runtime);
    [[nodiscard]] static bool Handle(
        RoleStateWindowState& state, std::uint32_t owner,
        std::uint32_t event,
        IRoleStateWindowPlayerRuntime& player_runtime,
        IRoleStateWindowSwapRuntime& swap_runtime);
};

}  // namespace pal4
