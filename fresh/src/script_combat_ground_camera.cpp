#include "pal4/script_combat_ground_camera.h"

namespace pal4 {

std::uint8_t ScriptCombatGroundCamera::Configure(
    const std::uint32_t text, IScriptCombatGroundCameraRuntime& runtime) {
    return runtime.UpdateText(runtime.CombatSystem(), text);
}

}  // namespace pal4
