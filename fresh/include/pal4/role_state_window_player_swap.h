#pragma once

#include "pal4/role_state_window_player_info.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IRoleStateWindowSwapRuntime {
public:
    virtual ~IRoleStateWindowSwapRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string WarningText(
        std::uint32_t trigger) = 0;
    [[nodiscard]] virtual std::uint32_t CreateCommonDialog() = 0;
    [[nodiscard]] virtual std::uint8_t ShowDialog(
        std::uint32_t dialog, const std::string& text,
        std::uint32_t x_bits, std::uint32_t y_bits,
        std::uint32_t style, std::uint32_t option) = 0;
    [[nodiscard]] virtual std::uint8_t PlaySound(
        std::string_view filename, std::uint32_t option) = 0;
};

class RoleStateWindowPlayerSwap final {
public:
    [[nodiscard]] static std::uint8_t Handle(
        RoleStateWindowState& state, std::uint32_t owner,
        std::uint32_t target_index, std::uint32_t selected_index,
        IRoleStateWindowPlayerRuntime& player_runtime,
        IRoleStateWindowSwapRuntime& runtime);
};

}  // namespace pal4
