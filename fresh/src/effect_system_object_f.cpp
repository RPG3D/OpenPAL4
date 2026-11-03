#include "pal4/effect_system_object_f.h"

#include <bit>

namespace pal4 {

EffectSystemObjectFState& EffectSystemObjectF::Construct(
    EffectSystemObjectFState& state, const std::uint8_t initial_byte,
    IEffectSystemObjectFRuntime& runtime) {
    state.words[1u] = 0u;
    state.words[2u] = 0u;
    state.words[3u] = 0u;
    state.words[4u] = 0u;
    state.words[0u] = 0x84b240u;

    constexpr std::array<std::uint32_t, 9u> base_facets{
        0x84a1dcu, 0x84a1d0u, 0x84a1c8u,
        0x84a1bcu, 0x84a1acu, 0x84a1a0u,
        0x84a194u, 0x84a184u, 0x84a178u};
    for (std::uint32_t index = 0u; index < base_facets.size(); ++index) {
        state.words[5u + index] = base_facets[index];
    }
    runtime.InitializeXmlDocument(state, 56u);
    state.words[29u] = 0u;
    state.words[38u] =
        (state.words[38u] & 0xffffff00u) | initial_byte;
    state.words[39u] = 0u;
    state.words[40u] = 0u;
    state.words[41u] = 0u;

    state.words[0u] = 0x84a120u;
    constexpr std::array<std::uint32_t, 9u> derived_facets{
        0x84a114u, 0x84a108u, 0x84a100u,
        0x84a0f4u, 0x84a0e4u, 0x84a0d8u,
        0x84a0ccu, 0x84a0bcu, 0x84a0b0u};
    for (std::uint32_t index = 0u; index < derived_facets.size(); ++index) {
        state.words[5u + index] = derived_facets[index];
    }
    const auto system = runtime.EffectSystemInstance();
    const auto count = runtime.ReadWord(system + 164u);
    runtime.WriteWord(system + 164u, count + 1u);
    return state;
}

void EffectSystemObjectF::Destroy(
    EffectSystemObjectFState& state,
    IEffectSystemObjectFRuntime& runtime) {
    state.words[0u] = 0x84a120u;
    constexpr std::array<std::uint32_t, 9u> derived_facets{
        0x84a114u, 0x84a108u, 0x84a100u,
        0x84a0f4u, 0x84a0e4u, 0x84a0d8u,
        0x84a0ccu, 0x84a0bcu, 0x84a0b0u};
    for (std::uint32_t index = 0u; index < derived_facets.size(); ++index) {
        state.words[5u + index] = derived_facets[index];
    }
    const auto system = runtime.EffectSystemInstance();
    const auto count = runtime.ReadWord(system + 164u);
    runtime.WriteWord(system + 164u, count - 1u);
    runtime.Free(state.words[39u]);
    state.words[39u] = 0u;
    state.words[40u] = 0u;
    state.words[41u] = 0u;
    runtime.DestroyXmlDocument(state, 56u);

    constexpr std::array<std::uint32_t, 9u> base_facets{
        0x84a1dcu, 0x84a1d0u, 0x84a1c8u,
        0x84a1bcu, 0x84a1acu, 0x84a1a0u,
        0x84a194u, 0x84a184u, 0x84a178u};
    for (std::uint32_t index = 0u; index < base_facets.size(); ++index) {
        state.words[5u + index] = base_facets[index];
    }
    state.words[0u] = 0x84b240u;
}

std::uint32_t EffectSystemObjectF::ObjectHandle(
    const EffectSystemObjectFState& state,
    IEffectSystemObjectFRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return 0u;
    return runtime.ReadWord(state.words[29u] + 20u);
}

std::string_view EffectSystemObjectF::ClassName() {
    return "BillBordObject";
}

void EffectSystemObjectF::SetTimedMode(
    EffectSystemObjectFState& state, const std::uint32_t enabled,
    IEffectSystemObjectFPlaybackRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return;

    const auto new_mode = static_cast<std::uint8_t>(enabled);
    const auto old_mode = static_cast<std::uint8_t>(state.words[2u] >> 24u);
    if (old_mode == new_mode)
        return;
    state.words[2u] = (state.words[2u] & 0x00ffffffu) |
                      (static_cast<std::uint32_t>(new_mode) << 24u);
    if (new_mode == 0u)
        return;

    std::array<std::int32_t, 4> values{};
    runtime.QueryTaskValues(state, values);
    const auto numerator = static_cast<float>(values[3u]);
    const auto ratio = static_cast<float>(numerator / runtime.Duration(state));
    state.words[30u] = std::bit_cast<std::uint32_t>(ratio);
}

}  // namespace pal4
