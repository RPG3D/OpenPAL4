#include "pal4/dialog_object.h"

#include <cmath>

namespace pal4 {

double DialogObject::StopPlayback(
    DialogObjectState& state, bool detach_frame,
    IDialogObjectStopRuntime& runtime) {
    if (state.words[2u] == 0u)
        return 0.0;
    runtime.CleanupObjectCollection(state.words[2u]);
    runtime.DestroyTaskContainer(state.words[2u]);
    runtime.Free(state.words[2u]);
    state.words[2u] = 0u;

    const auto frame = state.words[43u];
    if (frame != 0u && detach_frame && runtime.FrameParent(frame) != 0u)
        runtime.RemoveFrameChild(frame);
    for (auto record = state.words[24u]; record != state.words[25u];
         record += 8u)
        runtime.WriteWord(record, 4u, 0u);

    float result = 0.0f;
    for (auto record = state.words[28u]; record != state.words[29u];
         record += 20u) {
        const auto effect = runtime.ReadWord(record, 16u);
        if (effect == 0u)
            continue;
        runtime.InitializeEffectSystem();
        runtime.SetEffectMode(effect, 1u);
        const auto value = runtime.EffectValue(effect);
        if (std::isnan(result) || std::isnan(value) || result < value)
            result = static_cast<float>(value);
    }
    return result;
}

}  // namespace pal4
