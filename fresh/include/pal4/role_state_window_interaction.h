#pragma once

#include "pal4/role_state_window_selection.h"

#include <cstdint>

namespace pal4 {

class IRoleStateWindowInteractionRuntime :
    public IRoleStateWindowSelectionRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CapturedWindow() = 0;
    [[nodiscard]] virtual std::uint32_t RootWindow() = 0;
    [[nodiscard]] virtual std::uint32_t ChildAtPosition(
        std::uint32_t root, std::uint32_t position) = 0;
    [[nodiscard]] virtual bool IsVisible(std::uint32_t window,
                                         bool recursive) = 0;
};

class RoleStateWindowInteraction final {
public:
    [[nodiscard]] static bool OnMouseMove(
        RoleStateWindowState& state, std::uint32_t event,
        IRoleStateWindowInteractionRuntime& runtime);
    [[nodiscard]] static bool OnMouseLeave(
        RoleStateWindowState& state,
        IRoleStateWindowInteractionRuntime& runtime);
    static void SetVisible(RoleStateWindowState& state, bool visible,
                           IRoleStateWindowSelectionRuntime& runtime);
    [[nodiscard]] static bool IsVisible(
        const RoleStateWindowState& state,
        IRoleStateWindowInteractionRuntime& runtime);
};

}  // namespace pal4
