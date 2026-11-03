#pragma once

#include <array>
#include <cstdint>
#include <functional>

namespace pal4 {
struct PalActorControlState;
class NativeCameraManagerOwner;
class NativeMonsterManagerOwner;
class NativeSceneUiEvents;
class NativeCeguiUiSession;
class NativeGameArenaManagerOwner;

// Borrows current scene services for one original JO reset. The coordinate
// boundary is the live CEGUI system transform, shared with HUD layout/input.
[[nodiscard]] std::uint8_t ResetNativeActorControlDisplay(
    PalActorControlState& control, NativeCameraManagerOwner& cameras,
    NativeMonsterManagerOwner& monsters, NativeSceneUiEvents& events,
    const std::function<std::array<float, 2>(const std::array<float, 2>&)>& coordinates);
// Original database virtual+0x20, including hide/progress/clump side effects.
[[nodiscard]] std::uint8_t UpdateNativeActorControlDisplay(
    PalActorControlState& control, std::uint32_t value,
    NativeCameraManagerOwner& cameras, NativeMonsterManagerOwner& monsters,
    NativeSceneUiEvents& events, NativeCeguiUiSession& ui,
    NativeGameArenaManagerOwner& arena);
}  // namespace pal4
