#include "pal4/role_state_window_members.h"

#include <cstddef>

namespace pal4 {

RoleStateWindowState RoleStateWindowMembers::Load(
    const RoleStateWindowState& owner, const std::uint32_t index) {
    RoleStateWindowState member;
    const auto base = Offset(index) / 4u;
    for (std::size_t i = 0; i < kStride / 4u; ++i)
        member.words[i] = owner.words[base + i];
    return member;
}

void RoleStateWindowMembers::Store(
    RoleStateWindowState& owner, const std::uint32_t index,
    const RoleStateWindowState& member) {
    const auto base = Offset(index) / 4u;
    for (std::size_t i = 0; i < kStride / 4u; ++i)
        owner.words[base + i] = member.words[i];
}

}  // namespace pal4
