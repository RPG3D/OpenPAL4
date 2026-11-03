#pragma once
#include <cstdint>
namespace pal4 {
struct NativeCombatSystemImage;
class NativeCombatPlayerManager;
class NativeCombatMouseInput;
class NativeScriptEventRouter;
class NativeMiles3dListener;
// This is only the original ready-commit tail, after the preceding Start phases.
// It does not initialize resources, show UI, create actors or enter Combat state.
bool CommitNativeCombatStartReady(NativeCombatSystemImage&,NativeCombatPlayerManager&,
    NativeCombatMouseInput&,NativeScriptEventRouter&,NativeMiles3dListener&,std::int32_t start_type);
}
