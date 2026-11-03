#pragma once

#include "pal4/stop_effect_task.h"

#include <bit>
#include <cstddef>

namespace pal4::stop_effect_detail {

inline bool ReadFlag(const StopEffectTaskState& state, std::size_t byte) {
    return ((state.words[byte / 4u] >> (8u * (byte % 4u))) & 0xffu) != 0u;
}
inline void WriteFlag(StopEffectTaskState& state, std::size_t byte,
                      bool value) {
    const auto shift = static_cast<unsigned>(8u * (byte % 4u));
    auto& word = state.words[byte / 4u];
    word = (word & ~(0xffu << shift)) | (std::uint32_t{value} << shift);
}
inline float ReadFloat(const StopEffectTaskState& state, std::size_t word) {
    return std::bit_cast<float>(state.words[word]);
}
inline void WriteFloat(StopEffectTaskState& state, std::size_t word,
                       float value) {
    state.words[word] = std::bit_cast<std::uint32_t>(value);
}

}  // namespace pal4::stop_effect_detail
