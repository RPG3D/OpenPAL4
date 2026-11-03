#include "pal4/script_status_mode.h"

namespace pal4 {

std::uint32_t ScriptStatusMode::Enable(
    const std::int32_t mode, IScriptStatusModeRuntime& runtime) {
    if (mode < 1 || mode > 3) return static_cast<std::uint32_t>(mode);
    return runtime.ActivateStatusMode(runtime.ActorControl(), mode);
}

}  // namespace pal4
