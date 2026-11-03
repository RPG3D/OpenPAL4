#pragma once

#include "pal4/role_state_window_initializer.h"

#include <cstdint>

namespace pal4 {

class RoleStateWindowMembers final {
public:
    static constexpr std::uint32_t kFirstOffset = 0x2cu;
    static constexpr std::uint32_t kStride = 0x88u;
    static constexpr std::uint32_t kCount = 4u;

    [[nodiscard]] static constexpr std::uint32_t Offset(
        std::uint32_t index) noexcept {
        return kFirstOffset + index * kStride;
    }
    [[nodiscard]] static RoleStateWindowState Load(
        const RoleStateWindowState& owner, std::uint32_t index);
    static void Store(RoleStateWindowState& owner, std::uint32_t index,
                      const RoleStateWindowState& member);
};

}  // namespace pal4
