#pragma once

#include "pal4/role_state_window_player_info.h"

#include <cstdint>

namespace pal4 {

class IRoleStateWindowInputRuntime {
public:
    virtual ~IRoleStateWindowInputRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t InputManager() = 0;
    [[nodiscard]] virtual std::uint32_t InventoryManager() = 0;
    virtual void UpdateAndClampCounter(std::uint32_t inventory,
                                       std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void ProcessEvent(std::uint32_t tree, std::uint32_t command,
                              std::uint32_t first,
                              std::uint32_t second) = 0;
    virtual void UpdatePlayerResource(std::uint32_t player_data,
                                      std::uint32_t field,
                                      std::uint32_t value) = 0;
    [[nodiscard]] virtual bool IsVisible(std::uint32_t window,
                                         bool recursive) = 0;
};

class RoleStateWindowInputEvent final {
public:
    static constexpr std::uint32_t kGameInfoEvent = 0x8b13a0u;

    [[nodiscard]] static bool Handle(
        RoleStateWindowState& state, std::uint32_t event,
        IRoleStateWindowPlayerRuntime& player_runtime,
        IRoleStateWindowInputRuntime& runtime);
};

}  // namespace pal4
