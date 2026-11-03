#include "pal4/effect_system_object_f_shutdown.h"

namespace pal4 {

std::int32_t EffectSystemObjectFShutdown::ShutdownAndFinalize(
    EffectSystemObjectFState& state,
    IEffectSystemObjectFShutdownRuntime& runtime) {
    if ((state.words[1u] & 0xffu) != 0u) {
        runtime.ClearComponent(state, 48u);
        runtime.ClearComponent(state, 20u);
        state.words[1u] &= 0x0000ff00u;
        runtime.CleanupXml(state, 56u);
        const auto object = state.words[29u];
        if (object != 0u) {
            runtime.DeinitializeObject(object);
            runtime.FreeObject(object);
            state.words[29u] = 0u;
        }
        runtime.ProcessEvent(state.words[4u], state);
    }
    return runtime.FinalizeObject(state, 1u);
}

}  // namespace pal4
