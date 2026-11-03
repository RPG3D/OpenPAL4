#include "pal4/dynamic_strip_object_config.h"

#include <bit>

namespace pal4 {

namespace {

void WriteFloat(StripVisualBlockView state,
                std::uint32_t offset, float value) {
    state.words[offset / 4u] = std::bit_cast<std::uint32_t>(value);
}

void WriteByte(StripVisualBlockView state,
               std::uint32_t offset, std::uint8_t value) {
    auto& word = state.words[offset / 4u];
    const auto shift = 8u * (offset % 4u);
    word = (word & ~(0xffu << shift)) |
           (static_cast<std::uint32_t>(value) << shift);
}

std::uint8_t SetupHookBlade(
    StripVisualBlockView state, std::uint32_t texture,
    std::uint32_t clump, float life, bool alternate_hooks,
    IStripHookLookupRuntime& runtime) {
    if (texture == 0u || clump == 0u || !(life > 0.0f)) return 0u;

    const auto first = runtime.FindHook(
        clump, alternate_hooks ? "hook_blade_a" : "hook_blade_c");
    const auto second = runtime.FindHook(
        clump, alternate_hooks ? "hook_blade_b" : "hook_blade_d");
    if (first == 0u || second == 0u) return 0u;

    std::uint32_t texture_width{};
    std::uint32_t texture_height{};
    state.words[14u] = 0u;
    state.words[25u] = runtime.HookFrame(first);
    state.words[26u] = runtime.HookFrame(second);
    WriteFloat(state, 60u, life);
    state.words[16u] = texture;
    runtime.TextureSize(texture, texture_width, texture_height);
    WriteFloat(state, 68u, static_cast<float>(
        0.5 / static_cast<double>(texture_width)));
    WriteFloat(state, 72u, static_cast<float>(
        0.5 / static_cast<double>(texture_height)));
    runtime.AddTextureReference(texture);
    WriteByte(state, 0u, 1u);
    WriteByte(state, 2u, 1u);
    WriteByte(state, 3u, 1u);
    return 1u;
}

}  // namespace

std::uint8_t DynamicStripObjectConfiguration::VisualSetupHookBlade(
    DynamicStripObjectState& state, std::uint32_t texture,
    std::uint32_t clump, float life, bool alternate_hooks,
    IDynamicStripHookRuntime& runtime) {
    runtime.CleanupResources(state);
    return SetupHookBlade(StripVisualBlockView{std::span{state.words}.first<34u>()},
                          texture, clump, life, alternate_hooks, runtime);
}

std::uint8_t DynamicStripObjectConfiguration::VisualSetupHookBlade(
    StripVisualBlockView state, std::uint32_t texture,
    std::uint32_t clump, float life, bool alternate_hooks,
    IStripVisualBlockHookRuntime& runtime) {
    runtime.CleanupResources(state);
    return SetupHookBlade(state, texture, clump, life, alternate_hooks, runtime);
}

}  // namespace pal4
