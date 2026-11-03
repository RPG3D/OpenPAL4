#include "pal4/clump_object_visibility.h"

#include <stdexcept>

namespace pal4 {
namespace {

std::uint8_t ReadByte(const ClumpObjectState& state,
                      const std::uint32_t offset) {
    return static_cast<std::uint8_t>(
        state.words[offset / 4u] >> ((offset % 4u) * 8u));
}

void WriteByte(ClumpObjectState& state, const std::uint32_t offset,
               const std::uint8_t value) {
    const auto shift = (offset % 4u) * 8u;
    const auto mask = 0xffu << shift;
    auto& word = state.words[offset / 4u];
    word = (word & ~mask) | (static_cast<std::uint32_t>(value) << shift);
}

std::uint32_t Clump(const ClumpObjectState& state,
                    IClumpObjectVisibilityRuntime& runtime) {
    return runtime.ReadWord(state.words[18u] + 20u);
}

std::uint32_t RequireEffectManager(const std::uint32_t manager) {
    if (manager == 0u)
        throw std::runtime_error(
            "\xB5\xA5\xBC\xFE\xB6\xD4\xCF\xF3\xCE\xB4\xB1\xBB"
            "\xB4\xB4\xBD\xA8\xA3\xAC\xC7\xEB\xD4\xDA\xCA\xB9"
            "\xD3\xC3\xD6\xAE\xC7\xB0\xB4\xB4\xBD\xA8\xA3\xA1");
    return manager;
}

}  // namespace

std::uint8_t ClumpObjectVisibilityFacet::Deactivate(
    ClumpObjectState& state, IClumpObjectVisibilityRuntime& runtime) {
    const auto active = ReadByte(state, 4u);
    if (active == 0u)
        return active;

    WriteByte(state, 6u, 0u);
    WriteByte(state, 7u, 0u);
    const auto effect_manager = runtime.EffectManager();
    const auto first_clump = Clump(state, runtime);
    runtime.RemoveClumpAndProcessAtomics(
        RequireEffectManager(effect_manager), first_clump);
    runtime.PerformOwnerAction2(state.words[4u], Clump(state, runtime));
    const auto clump = Clump(state, runtime);
    const auto world = runtime.WorldForClump(clump);
    return world == 0u ? 0u : runtime.RemoveClump(world, clump);
}

std::uint8_t ClumpObjectVisibilityFacet::Activate(
    ClumpObjectState& state, IClumpObjectVisibilityRuntime& runtime) {
    static_cast<void>(Deactivate(state, runtime));
    const auto active = ReadByte(state, 4u);
    if (active == 0u)
        return active;

    WriteByte(state, 6u, 1u);
    WriteByte(state, 7u, 1u);
    const auto owner = state.words[4u];
    const auto clump = Clump(state, runtime);
    if (runtime.OwnerAllowsClump(owner, clump) &&
        runtime.WorldForClump(clump) != 0u)
        WriteByte(state, 7u, 0u);
    runtime.PerformOwnerAction(owner, clump);
    const auto effect_manager = runtime.EffectManager();
    const auto final_clump = Clump(state, runtime);
    return runtime.HandleEffectAddition(
        RequireEffectManager(effect_manager), final_clump);
}

}  // namespace pal4
