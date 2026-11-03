#pragma once
namespace pal4 {
struct NativeCombatSystemImage;
class NativeCombatConsole;
class NativeCameraManagerOwner;
class NativeScriptEventRouter;
// Borrow existing console/cameras/router for the shared Start reset block.
// This neither clears combat rosters nor activates the battle state/camera.
void ResetNativeCombatStartConsole(NativeCombatSystemImage&,NativeCombatConsole&,
    NativeCameraManagerOwner&,NativeScriptEventRouter&);
}
