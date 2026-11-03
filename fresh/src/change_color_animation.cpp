#include "pal4/change_color_task.h"
#include "pal4/effect_task_timing.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>

namespace pal4 {
namespace {

float ReadFloat(const ChangeColorTaskState& state, const std::uint32_t offset) {
    return std::bit_cast<float>(state.words[offset / 4u]);
}

void WriteFloat(ChangeColorTaskState& state, const std::uint32_t offset,
                const float value) {
    state.words[offset / 4u] = std::bit_cast<std::uint32_t>(value);
}

std::uint8_t ReadByte(const ChangeColorTaskState& state,
                      const std::uint32_t offset) {
    return static_cast<std::uint8_t>(
        state.words[offset / 4u] >> (8u * (offset % 4u)));
}

void WriteByte(ChangeColorTaskState& state, const std::uint32_t offset,
               const std::uint8_t value) {
    const auto shift = 8u * (offset % 4u);
    const auto mask = 0xffu << shift;
    auto& word = state.words[offset / 4u];
    word = (word & ~mask) | (static_cast<std::uint32_t>(value) << shift);
}

bool BelowOrUnordered(const float lhs, const float rhs) {
    return lhs < rhs || std::isunordered(lhs, rhs);
}

std::uint8_t ColorByte(const float color) {
    const auto scaled = static_cast<float>(
        static_cast<double>(color) * 255.0 + 0.5);
    if (!std::isfinite(scaled) ||
        scaled >= static_cast<float>(std::numeric_limits<std::int32_t>::max()) ||
        scaled < static_cast<float>(std::numeric_limits<std::int32_t>::min()))
        return 0u;
    return static_cast<std::uint8_t>(static_cast<std::int32_t>(scaled));
}

std::array<std::uint8_t, 4u> ColorAt(
    const ChangeColorTaskState& state, const std::uint32_t offset) {
    std::array<std::uint8_t, 4u> result{};
    for (std::uint32_t i = 0u; i < result.size(); ++i)
        result[i] = ColorByte(ReadFloat(state, offset + i * 4u));
    return result;
}

float RandomDuration(ChangeColorTaskState& state, const std::int32_t random) {
    const auto bias = ReadFloat(state, 156u);
    const auto initial_duration = ReadFloat(state, 20u);
    const auto duration=RandomizedEffectDuration(random,initial_duration,bias);
    WriteFloat(state, 152u, duration);
    return duration;
}

std::uint32_t WithLowByteOne(const std::uint32_t value) {
    return (value & 0xffffff00u) | 1u;
}

}  // namespace

std::uint32_t ChangeColorTask::UpdateAnimation(
    ChangeColorTaskState& state, const float elapsed,
    IChangeColorAnimationRuntime& runtime) {
    const auto rollback = ReadByte(state, 144u) != 0u;
    if (rollback) {
        if (ReadByte(state, 160u) != 0u) {
            if (BelowOrUnordered(ReadFloat(state, 168u),
                                 ReadFloat(state, 164u))) {
                WriteFloat(state, 168u, elapsed + ReadFloat(state, 168u));
                return 0x101u;
            }
            WriteByte(state, 160u, 0u);
            state.words[42u] = 0u;
        }
        if (ReadByte(state, 172u) != 0u) {
            if (BelowOrUnordered(ReadFloat(state, 180u),
                                 ReadFloat(state, 176u))) {
                WriteFloat(state, 180u, elapsed + ReadFloat(state, 180u));
                return 0x101u;
            }
            WriteByte(state, 172u, 0u);
            state.words[45u] = 0u;
        }
    }

    auto effective_elapsed = elapsed;
    if (rollback) {
        const auto progress = ReadFloat(state, 148u);
        const auto duration = ReadFloat(state, 152u);
        const auto next = elapsed + progress;
        if (BelowOrUnordered(next, duration)) {
            WriteFloat(state, 148u, next);
        } else {
            effective_elapsed = duration - progress;
            state.words[37u] = state.words[38u];
        }
    }

    for (std::uint32_t i = 0u; i < 4u; ++i) {
        const auto current = ReadFloat(state, 120u + i * 4u);
        const auto slope = ReadFloat(state, 104u + i * 4u);
        WriteFloat(state, 120u + i * 4u,
                   static_cast<float>(static_cast<double>(current) +
                                      static_cast<double>(slope) *
                                          effective_elapsed));
    }
    const auto result = runtime.WriteColor(
        state.words[35u], ColorAt(state, 120u));
    if (!rollback)
        return WithLowByteOne(result);
    if (BelowOrUnordered(ReadFloat(state, 148u),
                         ReadFloat(state, 152u)))
        return 0x101u;

    const auto old_phase = ReadByte(state, 145u);
    state.words[37u] = 0u;
    WriteByte(state, 145u, static_cast<std::uint8_t>(old_phase == 0u));
    if (old_phase == 0u) {
        (void)runtime.WriteColor(state.words[35u], ColorAt(state, 88u));
        for (std::uint32_t i = 0u; i < 4u; ++i) {
            state.words[30u + i] = state.words[22u + i];
            WriteFloat(state, 104u + i * 4u,
                       -ReadFloat(state, 104u + i * 4u));
        }
        WriteByte(state, 160u, 1u);
        state.words[42u] = 0u;
        return WithLowByteOne(state.words[24u]);
    }

    (void)runtime.WriteColor(state.words[35u], ColorAt(state, 72u));
    for (std::uint32_t i = 0u; i < 4u; ++i)
        state.words[30u + i] = state.words[18u + i];
    const auto random = runtime.RandomInt();
    const auto duration = RandomDuration(state, random);
    for (std::uint32_t i = 0u; i < 4u; ++i) {
        const auto start = ReadFloat(state, 72u + 4u * i);
        const auto end = ReadFloat(state, 88u + 4u * i);
        WriteFloat(state, 104u + 4u * i,
                   static_cast<float>((static_cast<double>(end) - start) /
                                      duration));
    }
    WriteByte(state, 172u, 1u);
    state.words[45u] = 0u;
    return WithLowByteOne(static_cast<std::uint32_t>(random));
}

std::uint8_t ChangeColorTask::FinishAnimation(
    ChangeColorTaskState& state, IChangeColorAnimationRuntime& runtime) {
    (void)runtime.WriteColor(state.words[35u], ColorAt(state, 88u));
    state.words[35u] = 0u;
    return 1u;
}

}  // namespace pal4
