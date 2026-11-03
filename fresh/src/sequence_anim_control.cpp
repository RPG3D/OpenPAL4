#include "pal4/sequence_anim_control.h"

namespace pal4 {

std::uint8_t SequenceAnimControl::Reset(
    SequenceAnimObjectState& state, ISequenceAnimControlRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return 0u;

    const auto resource = state.words[30u];
    state.words[20u] = 0u;
    state.words[16u] = 0u;
    state.words[17u] = state.words[15u] - 1u;
    runtime.WriteResourceWord(resource + 24u, state.words[36u]);
    runtime.WriteResourceWord(resource + 40u, state.words[37u]);
    runtime.WriteResourceWord(resource + 44u, state.words[38u]);
    const auto geometry = runtime.Geometry(resource);
    runtime.SetColor(geometry, state.words[35u]);
    runtime.WriteResourceWord(resource + 28u, state.words[39u]);
    runtime.WriteResourceWord(resource + 32u, state.words[40u]);
    runtime.WriteResourceWord(resource + 36u, state.words[41u]);
    runtime.InitializeTransform(resource);
    runtime.UpdateGeometry(runtime.Geometry(resource), {
        state.words[31u], state.words[32u],
        state.words[33u], state.words[34u]});
    state.words[2u] &= 0x00ffffffu;
    return 1u;
}

std::uint32_t SequenceAnimControl::Stop(
    SequenceAnimObjectState& state, ISequenceAnimControlRuntime& runtime) {
    if ((state.words[1u] & 0xffu) != 0u) {
        if (((state.words[1u] >> 16u) & 0xffu) != 0u)
            runtime.StopPlaybackFacet(state);
        const auto resource = state.words[30u];
        state.words[1u] &= 0xff00ffffu;
        state.words[1u] &= 0x00ffffffu;
        state.words[1u] &= 0xffffff00u;
        for (std::uint32_t i = 13u; i <= 20u; ++i)
            state.words[i] = 0u;
        for (std::uint32_t i = 25u; i <= 28u; ++i)
            state.words[i] = 0u;
        state.words[21u] = 0u;
        state.words[22u] = 0u;
        state.words[23u] = 0x3f800000u;
        state.words[24u] = 0x3f800000u;
        if (resource != 0u) {
            runtime.DeinitializeResource(resource);
            runtime.FreeResource(resource);
            state.words[30u] = 0u;
        }
        runtime.ProcessEvent(state.words[4u], state);
    }
    return runtime.DeletingDestroy(state, 1u);
}

}  // namespace pal4
