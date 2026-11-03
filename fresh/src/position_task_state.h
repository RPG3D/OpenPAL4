#pragma once

#include "pal4/position_task.h"

#include <bit>
#include <cstddef>

namespace pal4::position_task_detail {

inline float ReadFloat(const PositionTaskState& state, std::size_t offset) {
    return std::bit_cast<float>(state.words[offset / 4u]);
}
inline void WriteFloat(PositionTaskState& state, std::size_t offset,
                       float value) {
    state.words[offset / 4u] = std::bit_cast<std::uint32_t>(value);
}
inline std::uint8_t ReadByte(const PositionTaskState& state,
                             std::size_t offset) {
    return static_cast<std::uint8_t>(
        state.words[offset / 4u] >> (8u * (offset % 4u)));
}
inline void WriteByte(PositionTaskState& state, std::size_t offset,
                      std::uint8_t value) {
    const auto shift = static_cast<unsigned>(8u * (offset % 4u));
    auto& word = state.words[offset / 4u];
    word = (word & ~(0xffu << shift)) | (std::uint32_t{value} << shift);
}
inline PositionVector ReadVector(const PositionTaskState& state,
                                 std::size_t offset) {
    return {ReadFloat(state, offset), ReadFloat(state, offset + 4u),
            ReadFloat(state, offset + 8u)};
}
inline void WriteVector(PositionTaskState& state, std::size_t offset,
                        const PositionVector& value) {
    for (std::size_t i = 0; i < value.size(); ++i)
        WriteFloat(state, offset + 4u * i, value[i]);
}

}  // namespace pal4::position_task_detail
