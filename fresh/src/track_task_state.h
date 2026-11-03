#pragma once

#include "pal4/track_task.h"

#include <bit>
#include <cstddef>

namespace pal4::track_task_detail {

inline float ReadFloat(const TrackTaskState& state, std::size_t word) {
    return std::bit_cast<float>(state.words[word]);
}
inline void WriteFloat(TrackTaskState& state, std::size_t word, float value) {
    state.words[word] = std::bit_cast<std::uint32_t>(value);
}
inline bool ReadFlag(const TrackTaskState& state, std::size_t byte) {
    return ((state.words[byte / 4u] >> (8u * (byte % 4u))) & 0xffu) != 0u;
}
inline void WriteFlag(TrackTaskState& state, std::size_t byte, bool value) {
    const auto shift = static_cast<unsigned>(8u * (byte % 4u));
    auto& word = state.words[byte / 4u];
    word = (word & ~(0xffu << shift)) | (std::uint32_t{value} << shift);
}
inline void WriteByte(TrackTaskState& state, std::size_t byte,
                      std::uint8_t value) {
    const auto shift = static_cast<unsigned>(8u * (byte % 4u));
    auto& word = state.words[byte / 4u];
    word = (word & ~(0xffu << shift)) | (std::uint32_t{value} << shift);
}
inline TrackPoint ReadPoint(const TrackTaskState& state, std::size_t word) {
    return {ReadFloat(state, word), ReadFloat(state, word + 1u),
            ReadFloat(state, word + 2u)};
}
inline void WritePoint(TrackTaskState& state, std::size_t word,
                       const TrackPoint& point) {
    for (std::size_t i = 0; i < point.size(); ++i)
        WriteFloat(state, word + i, point[i]);
}
inline TrackPoint Difference(const TrackPoint& right, const TrackPoint& left) {
    return {right[0] - left[0], right[1] - left[1], right[2] - left[2]};
}

}  // namespace pal4::track_task_detail
