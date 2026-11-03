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

void DialogObject::PrepareForDelete(
    DialogObjectState& state, IDialogObjectPreDeleteRuntime& runtime,
    IDialogObjectFacetCleanupRuntime& facet_runtime) {
    runtime.ResetProperties(state);
    for (auto address = state.words[24u]; address != state.words[25u];
         address += 8u)
        runtime.WriteWord(address, 4u, 0u);
    state.words[25u] = state.words[24u];

    for (auto address = state.words[32u]; address != state.words[33u];
         address += 4u) {
        const auto node = runtime.ReadWord(address, 0u);
        if (node != 0u) {
            runtime.DestroyXmlNode(node);
            runtime.Free(node);
        }
    }
    state.words[33u] = state.words[32u];

    const auto effects_begin = state.words[28u];
    const auto effects_end = state.words[29u];
    for (auto address = effects_begin; address != effects_end;
         address += 20u) {
        const auto effects_enabled = runtime.EffectsEnabled();
        const auto effect = runtime.ReadWord(address, 16u);
        if (effect == 0u)
            continue;
        if (effects_enabled || !runtime.EffectIsActive(effect)) {
            runtime.ReleaseEffectReferences(effect);
            const auto frame = runtime.EffectFrame(effect);
            if (frame != 0u && runtime.FrameParent(frame) != 0u)
                runtime.RemoveFrameChild(frame);
            runtime.ProcessEffectEvent(state.words[3u], effect);
        }
        runtime.DestroyEffect(effect);
        runtime.WriteWord(address, 16u, 0u);
    }
    for (auto address = effects_begin; address != effects_end;
         address += 20u)
        runtime.ReleaseTextRecord(address);
    state.words[29u] = effects_begin;

    const auto frame = state.words[43u];
    if (frame != 0u) {
        if (runtime.FrameParent(frame) != 0u)
            runtime.RemoveFrameChild(frame);
        runtime.DestroyFrame(frame);
        state.words[43u] = 0u;
    }
    ClearFacets(state, facet_runtime);
    runtime.CleanupXmlDocument(state);
    runtime.ClearTextElements(state.words[4u]);
    state.words[3u] = 0u;
    WriteByte(state, 212u, 0u);
    state.words[54u] = 0u;
    for (const auto offset : {80u, 81u, 82u, 4u})
        WriteByte(state, offset, 0u);
    for (const auto index : {44u, 50u, 51u, 52u})
        state.words[index] = 0u;
    for (const auto index : {45u, 46u, 47u, 48u, 49u})
        state.words[index] = std::bit_cast<std::uint32_t>(1.0f);
}

}  // namespace pal4
