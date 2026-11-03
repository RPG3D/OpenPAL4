#include "pal4/role_state_window_button_state.h"

namespace pal4 {

std::optional<std::uint32_t> RoleStateWindowButtonState::Release(
    const std::uint32_t slot,
    IRoleStateWindowButtonStateRuntime& runtime) {
    const auto object = runtime.ReadWord(slot);
    if (object == 0u) return std::nullopt;
    const auto remaining = runtime.ReadWord(object + 4u) - 1u;
    runtime.WriteWord(object + 4u, remaining);
    const auto result = remaining == 0u
        ? runtime.DeleteObject(object, 1u) : remaining;
    runtime.WriteWord(slot, 0u);
    return result;
}

std::uint32_t RoleStateWindowButtonState::Create(
    const std::uint32_t slot, const std::uint32_t first,
    const std::uint32_t second,
    IRoleStateWindowButtonStateRuntime& runtime) {
    const auto object = runtime.Allocate(0xcu);
    if (object != 0u) {
        runtime.WriteWord(object + 4u, first);
        runtime.WriteWord(object + 8u, second);
        runtime.WriteWord(object, kVtable);
    }
    runtime.WriteWord(slot, object);
    return slot;
}

std::uint32_t RoleStateWindowButtonState::Construct(
    const std::uint32_t object, const std::uint32_t first,
    const std::uint32_t second,
    IRoleStateWindowButtonStateRuntime& runtime) {
    runtime.WriteWord(object + 4u, first);
    runtime.WriteWord(object + 8u, second);
    runtime.WriteWord(object, kVtable);
    return object;
}

}  // namespace pal4
