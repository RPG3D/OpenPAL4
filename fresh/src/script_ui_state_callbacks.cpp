#include "pal4/script_ui_state_callbacks.h"

namespace pal4 {

std::uint32_t ScriptWorldMapState::Set(
    std::uint32_t name, std::int32_t state,
    IScriptWorldMapStateRuntime& runtime) {
    return runtime.SetExternalButtonVisibility(name, state);
}

std::uint32_t ScriptPuzzleResult::Get(
    IScriptPuzzleResultRuntime& runtime) {
    return runtime.ReadByte(runtime.UiFrameManager() + 0x222u);
}

std::uint32_t ScriptAlwaysJump::Set(
    std::uint32_t enabled, IScriptAlwaysJumpRuntime& runtime) {
    const auto actor = runtime.ActorControl();
    runtime.WriteWord(actor + 0x110u, enabled & 0xffu);
    return actor;
}

}  // namespace pal4
