#include "pal4/dialog_object.h"

namespace pal4 {

bool DialogObject::ActivateEffects(
    DialogObjectState& state, std::uint32_t object,
    IDialogObjectActivationRuntime& runtime) {
    for (auto record = state.words[28u]; record != state.words[29u];
         record += 20u) {
        const auto text_address = runtime.ReadWord(record, 4u);
        const auto name = text_address != 0u
            ? runtime.ReadText(text_address) : std::string_view{};
        const auto property = runtime.ResolveTextProperty(state.words[4u], name);
        const auto effect = runtime.HandleEffectEvent(
            property, object, state.words[3u]);
        runtime.WriteWord(record, 16u, effect);
        if (effect == 0u)
            return false;
        if (runtime.EffectsEnabled() || !runtime.EffectIsActive(effect)) {
            runtime.ProcessEffectEvent(state.words[3u], effect);
            if (runtime.EffectFrame(effect) == 0u)
                return false;
            runtime.AddFrameChild(state.words[43u],
                                  runtime.EffectFrame(effect));
        }
    }
    return true;
}

}  // namespace pal4
