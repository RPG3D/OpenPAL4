#pragma once

#include "pal4/role_state_window_interaction.h"

#include <cstdint>

namespace pal4 {

class IRoleStateWindowEventRuntime :
    public IRoleStateWindowInteractionRuntime {
public:
    virtual ~IRoleStateWindowEventRuntime() = default;
    virtual void Close(std::uint32_t owner) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void ProcessEvent(std::uint32_t event_tree,
                              std::uint32_t command,
                              std::uint32_t first,
                              std::uint32_t second) = 0;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t ActivePlayerCount(
        std::uint32_t manager) = 0;
    virtual void HandleMouseEvent(std::uint32_t window) = 0;
};

class RoleStateWindowEvents final {
public:
    static constexpr std::uint32_t kCloseEvent = 0x8a80acu;

    [[nodiscard]] static bool OnEvent(
        RoleStateWindowState& state, std::uint32_t owner,
        std::uint32_t event, IRoleStateWindowEventRuntime& runtime);
    [[nodiscard]] static bool UpdateButtonImages(
        RoleStateWindowState& state,
        IRoleStateWindowSelectionRuntime& runtime);
};

}  // namespace pal4
