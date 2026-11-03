#include "pal4/clump_object_control.h"

#include "pal4/clump_object_config.h"

#include <array>
#include <bit>

namespace pal4 {
namespace {

std::array<float, 3> CachedVector(const ClumpObjectState& state,
                                  const std::uint32_t first_word) {
    return {std::bit_cast<float>(state.words[first_word]),
            std::bit_cast<float>(state.words[first_word + 1u]),
            std::bit_cast<float>(state.words[first_word + 2u])};
}

void WriteByte(ClumpObjectState& state, const std::uint32_t offset,
               const std::uint8_t value) {
    const auto shift = (offset % 4u) * 8u;
    const auto mask = 0xffu << shift;
    auto& word = state.words[offset / 4u];
    word = (word & ~mask) | (static_cast<std::uint32_t>(value) << shift);
}

}  // namespace

std::uint8_t ClumpObject::Reset(ClumpObjectState& state,
                                IClumpObjectConfigRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return 0u;

    const auto manager = state.words[18u];
    runtime.Rotate(manager, CachedVector(state, 68u), 0u);
    runtime.Scale(manager, CachedVector(state, 71u), 0u);
    runtime.Translate(manager, CachedVector(state, 74u), 0u);
    runtime.SetColor(manager, state.words[67u]);
    const auto clump = runtime.ReadWord(manager + 20u);
    const auto hierarchy = runtime.FindSkinHierarchy(clump);
    if (hierarchy != 0u)
        runtime.SetCurrentTime(runtime.ReadWord(hierarchy + 32u), 0.0f);
    WriteByte(state, 11u, 0u);
    return 1u;
}

std::uint32_t ClumpObject::StopAndDestroy(
    ClumpObjectState& state, IClumpObjectControlRuntime& runtime) {
    if ((state.words[1u] & 0xffu) != 0u) {
        runtime.StopFacet(state, 60u);
        runtime.StopFacet(state, 40u);
        WriteByte(state, 4u, 0u);
        state.words[17u] = 0xffffffffu;
        WriteByte(state, 6u, 0u);
        WriteByte(state, 7u, 0u);
        state.words[20u] = 0u;
        state.words[21u] = 0x3f800000u;
        state.words[22u] = 0x3f800000u;
        state.words[23u] = 0u;
        state.words[24u] = 0u;
        WriteByte(state, 104u, 0u);
        state.words[25u] = 0u;
        runtime.CleanupXml(state, 108u);
        WriteByte(state, 168u, 0u);
        WriteByte(state, 208u, 0u);
        WriteByte(state, 209u, 0u);
        WriteByte(state, 210u, 0u);
        state.words[53u] = 0u;
        state.words[54u] = 0u;
        for (const auto offset : {220u, 236u, 252u})
            runtime.AssignEmptyText(state, offset);

        if (state.words[18u] != 0u) {
            runtime.CleanupClump(state.words[18u],
                                 runtime.SystemInstance() + 52u);
            runtime.CleanupTexture(state.words[18u]);
            if (state.words[18u] != 0u) {
                runtime.DeleteManager(state.words[18u], 1u);
                state.words[18u] = 0u;
            }
        }
        if (state.words[19u] != 0u) {
            runtime.FreeAnimation(state.words[19u]);
            state.words[19u] = 0u;
        }
        runtime.NotifyOwner(state.words[4u], state);
        WriteByte(state, 5u, 0u);
    }
    return runtime.DeleteObject(state, 1u);
}

}  // namespace pal4
