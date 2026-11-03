#include "pal4/dialog_object.h"

#include <bit>
#include <cstddef>

namespace pal4 {
namespace {
void WriteByte(DialogObjectState& state, std::size_t offset,
               std::uint8_t value) {
    const auto shift = static_cast<unsigned>(8u * (offset % 4u));
    auto& word = state.words[offset / 4u];
    word = (word & ~(0xffu << shift)) | (std::uint32_t{value} << shift);
}
}  // namespace

std::uint32_t DialogObject::ResetRuntimeState(
    DialogObjectState& state, IDialogObjectResetRuntime& runtime) {
    if (state.words[2u] != 0u) {
        runtime.CleanupObjectCollection(state.words[2u]);
        runtime.DestroyTaskContainer(state.words[2u]);
        runtime.Free(state.words[2u]);
        state.words[2u] = 0u;
        WriteByte(state, 81u, 0u);
        WriteByte(state, 82u, 0u);
        WriteByte(state, 4u, 1u);
        WriteByte(state, 80u, 0u);
        for (const auto index : {50u, 51u, 52u, 44u})
            state.words[index] = 0u;
        for (const auto index : {47u, 48u, 49u, 45u, 46u})
            state.words[index] = std::bit_cast<std::uint32_t>(1.0f);
    }
    for (auto record = state.words[24u]; record != state.words[25u];
         record += 8u)
        runtime.WriteWord(record, 4u, 0u);
    for (auto record = state.words[28u]; record != state.words[29u];
         record += 20u) {
        const auto effect = runtime.ReadWord(record, 16u);
        if (effect == 0u)
            continue;
        if (runtime.EffectsEnabled() || !runtime.EffectIsActive(effect)) {
            runtime.SetEffectMode(effect, 0u);
            runtime.ReleaseEffectReferences(effect);
            runtime.ProcessEffectEvent(state.words[3u], effect);
        }
    }
    return state.words[29u];
}

}  // namespace pal4
