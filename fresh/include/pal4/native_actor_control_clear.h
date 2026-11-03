#pragma once

#include <cstdint>

namespace pal4 {
class NativeActorControlSession;
class InputManager;
class IEffectHandleContentRuntime;
class NativeSceneUiEvents;
class NativeCeguiUiSession;

// IDA423F60. Borrowers and effect/UI owners must remain alive during cleanup.
// The original AL result is unused by arena release and is not exposed here.
void ClearNativeActorControl(NativeActorControlSession& control, InputManager& input,
    IEffectHandleContentRuntime& effects, NativeSceneUiEvents& events,
    NativeCeguiUiSession& ui, std::uint32_t arena);
}  // namespace pal4
