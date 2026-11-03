#pragma once
#include <array>

namespace pal4 {
class NativeCombatActorOwner;
class NativeCombatPlayerActorOwner;
class NativeCameraManagerOwner;
class NativeCeguiUiSession;

// IDA52DF90: actor808 -> child position172 (Y+80), CombatCamera -> UI frame.
// Borrows live owners. The SDK camera's view matrix must already be current.
// A genuinely absent child preserves the original FLT_MAX pair.
[[nodiscard]] std::array<float,2> ProjectNativeCombatActorScreen(
    NativeCombatActorOwner&, NativeCameraManagerOwner&, NativeCeguiUiSession&);
[[nodiscard]] std::array<float,2> ProjectNativeCombatActorScreen(
    NativeCombatPlayerActorOwner&,NativeCameraManagerOwner&,NativeCeguiUiSession&);
}
