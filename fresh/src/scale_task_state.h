#pragma once

#include "pal4/scale_task.h"

#include <bit>
#include <cstddef>

namespace pal4::scale_task_detail {

inline float ReadFloat(const ScaleTaskState& state, std::size_t word) {
    return std::bit_cast<float>(state.words[word]);
}
inline void WriteFloat(ScaleTaskState& state, std::size_t word, float value) {
    state.words[word] = std::bit_cast<std::uint32_t>(value);
}
inline bool ReadFlag(const ScaleTaskState& state, std::size_t byte) {
    return ((state.words[byte / 4u] >> (8u * (byte % 4u))) & 0xffu) != 0u;
}
inline void WriteFlag(ScaleTaskState& state, std::size_t byte, bool value) {
    const auto shift = static_cast<unsigned>(8u * (byte % 4u));
    auto& word = state.words[byte / 4u];
    word = (word & ~(0xffu << shift)) | (std::uint32_t{value} << shift);
}
inline std::array<float, 3u> ReadVector(const ScaleTaskState& state,
                                        std::size_t word) {
    return {ReadFloat(state, word), ReadFloat(state, word + 1u),
            ReadFloat(state, word + 2u)};
}
inline void WriteVector(ScaleTaskState& state, std::size_t word,
                        const std::array<float, 3u>& value) {
    for (std::size_t i = 0; i < value.size(); ++i)
        WriteFloat(state, word + i, value[i]);
}
inline float BiasedDuration(const ScaleTaskState& state,
                            IScaleTaskRuntime& runtime) {
    const auto bias = static_cast<double>(ReadFloat(state, 29u));
    return static_cast<float>(static_cast<double>(ReadFloat(state, 5u)) +
        static_cast<double>(runtime.RandomInt()) * (2.0 * bias) *
        0.000030518509447574615 - bias);
}

}  // namespace pal4::scale_task_detail
