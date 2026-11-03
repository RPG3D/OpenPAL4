#include "pal4/dialog_object.h"

namespace pal4 {

std::uint32_t DialogObject::StopEffects(
    DialogObjectState& state, IDialogObjectEffectCleanupRuntime& runtime) {
    for (auto record = state.words[28u]; record != state.words[29u];
         record += 20u) {
        const auto effect = runtime.ReadWord(record, 16u);
        if (effect != 0u &&
            (runtime.EffectsEnabled() || !runtime.EffectIsActive(effect))) {
            runtime.ProcessEffectEvent(state.words[3u], effect);
            const auto frame = runtime.EffectFrame(effect);
            if (frame != 0u && runtime.FrameParent(frame) != 0u)
                runtime.RemoveFrameChild(frame);
        }
        if (effect != 0u) {
            runtime.DestroyEffect(effect);
            runtime.WriteWord(record, 16u, 0u);
        }
    }
    return state.words[29u];
}

}  // namespace pal4
