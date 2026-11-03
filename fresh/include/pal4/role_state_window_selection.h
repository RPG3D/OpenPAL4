#pragma once

#include "pal4/role_state_window_initializer.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class IRoleStateWindowSelectionRuntime {
public:
    virtual ~IRoleStateWindowSelectionRuntime() = default;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    virtual void SetImage(std::uint32_t window,
                          std::string_view imageset,
                          std::string_view image) = 0;
    virtual void ClearImage(std::uint32_t window) = 0;
};

class RoleStateWindowSelection final {
public:
    static void SetRoleState(RoleStateWindowState& state,
                             IRoleStateWindowSelectionRuntime& runtime);
    static void SetButtonImage(RoleStateWindowState& state,
                               IRoleStateWindowSelectionRuntime& runtime);
    static void SetHoveredButtonImage(
        RoleStateWindowState& state,
        IRoleStateWindowSelectionRuntime& runtime);
};

}  // namespace pal4
