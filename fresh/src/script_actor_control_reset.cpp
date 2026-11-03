#include "pal4/script_actor_control_reset.h"

namespace pal4 {

void ScriptActorControlReset::ToJumpStart(
    const std::uint32_t caller_context,
    IScriptActorControlResetRuntime& runtime) {
    runtime.ResetPlayerToJumpStart(runtime.ActorControl(), caller_context);
}

}  // namespace pal4
