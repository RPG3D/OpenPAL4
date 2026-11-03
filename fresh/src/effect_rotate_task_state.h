#pragma once

#include "pal4/effect_rotate_task.h"

#include <bit>
#include <cstddef>

namespace pal4::effect_rotate_detail {

inline float ReadFloat(const EffectRotateTaskState& state,
                       const std::size_t word) {
    return std::bit_cast<float>(state.words[word]);
}

inline void WriteFloat(EffectRotateTaskState& state, const std::size_t word,
                       const float value) {
    state.words[word] = std::bit_cast<std::uint32_t>(value);
}

inline bool ReadFlag(const EffectRotateTaskState& state,
                     const std::size_t byte) {
    return ((state.words[byte / 4u] >> (8u * (byte % 4u))) & 0xffu) != 0u;
}

inline void WriteFlag(EffectRotateTaskState& state, const std::size_t byte,
                      const bool value) {
    const auto shift = static_cast<unsigned>(8u * (byte % 4u));
    auto& word = state.words[byte / 4u];
    word = (word & ~(0xffu << shift)) | (std::uint32_t{value} << shift);
}

inline std::array<float, 3u> ReadVector(
    const EffectRotateTaskState& state, const std::size_t first_word) {
    return {ReadFloat(state, first_word), ReadFloat(state, first_word + 1u),
            ReadFloat(state, first_word + 2u)};
}

inline void WriteVector(EffectRotateTaskState& state,
                        const std::size_t first_word,
                        const std::array<float, 3u>& values) {
    for (std::size_t i = 0; i < values.size(); ++i)
        WriteFloat(state, first_word + i, values[i]);
}

inline float BiasedDuration(const EffectRotateTaskState& state,
                            IEffectRotateTaskRuntime& runtime) {
    const auto base = static_cast<double>(ReadFloat(state, 5u));
    const auto bias = static_cast<double>(ReadFloat(state, 36u));
    const auto random = static_cast<double>(runtime.RandomInt());
    return static_cast<float>(base + random * (2.0 * bias) *
                             0.000030518509447574615 - bias);
}

}  // namespace pal4::effect_rotate_detail
