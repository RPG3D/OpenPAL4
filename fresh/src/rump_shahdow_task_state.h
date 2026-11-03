#pragma once

#include "pal4/rump_shahdow_task.h"

#include <bit>
#include <cstddef>

namespace pal4::rump_shahdow_detail {

inline float ReadFloat(const RumpShahdowTaskState& state,
                       const std::size_t word) {
    return std::bit_cast<float>(state.words[word]);
}
inline void WriteFloat(RumpShahdowTaskState& state, const std::size_t word,
                       const float value) {
    state.words[word] = std::bit_cast<std::uint32_t>(value);
}
inline bool ReadFlag(const RumpShahdowTaskState& state,
                     const std::size_t byte) {
    return ((state.words[byte / 4u] >> (8u * (byte % 4u))) & 0xffu) != 0u;
}
inline void WriteByte(RumpShahdowTaskState& state, const std::size_t byte,
                      const std::uint8_t value) {
    const auto shift = static_cast<unsigned>(8u * (byte % 4u));
    auto& word = state.words[byte / 4u];
    word = (word & ~(0xffu << shift)) |
           (static_cast<std::uint32_t>(value) << shift);
}
inline void WriteFlag(RumpShahdowTaskState& state, const std::size_t byte,
                      const bool value) {
    WriteByte(state, byte, static_cast<std::uint8_t>(value));
}
inline std::uint8_t ReadByte(const RumpShahdowTaskState& state,
                             const std::size_t byte) {
    return static_cast<std::uint8_t>(
        state.words[byte / 4u] >> (8u * (byte % 4u)));
}

}  // namespace pal4::rump_shahdow_detail
