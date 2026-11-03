#include "pal4/effect_rotate_task.h"

#include "effect_rotate_task_state.h"

#include <array>

namespace pal4 {
using namespace effect_rotate_detail;

std::uint8_t EffectRotateTask::Update(
    EffectRotateTaskState& state, const float elapsed,
    IEffectRotateTaskRuntime& runtime) {
    const auto rollback = ReadFlag(state, 132u);
    if (rollback) {
        if (ReadFlag(state, 148u)) {
            if (ReadFloat(state, 39u) < ReadFloat(state, 38u)) {
                WriteFloat(state, 39u, elapsed + ReadFloat(state, 39u));
                return 1u;
            }
            WriteFlag(state, 148u, false);
            state.words[39u] = 0u;
        }
        if (ReadFlag(state, 160u)) {
            if (ReadFloat(state, 42u) < ReadFloat(state, 41u)) {
                WriteFloat(state, 42u, elapsed + ReadFloat(state, 42u));
                return 1u;
            }
            WriteFlag(state, 160u, false);
            state.words[42u] = 0u;
        }
    }

    auto step = elapsed;
    if (rollback || ReadFlag(state, 72u)) {
        const auto elapsed_total = elapsed + ReadFloat(state, 34u);
        const auto duration = ReadFloat(state, 35u);
        if (elapsed_total < duration) {
            WriteFloat(state, 34u, elapsed_total);
        } else {
            step = duration - ReadFloat(state, 34u);
            state.words[34u] = state.words[35u];
        }
    }

    if ((rollback || !ReadFlag(state, 72u)) && ReadFlag(state, 73u)) {
        for (std::size_t i = 0; i < 3u; ++i)
            WriteFloat(state, 26u + i,
                       step * ReadFloat(state, 29u + i) +
                           ReadFloat(state, 26u + i));
    }

    std::array<float, 3u> movement{};
    for (std::size_t i = 0; i < 3u; ++i)
        movement[i] = step * ReadFloat(state, 26u + i);
    runtime.ApplyRotation(state.words[32u], movement, true);

    if (!rollback || ReadFloat(state, 34u) < ReadFloat(state, 35u))
        return 1u;

    // Each completed leg flips phase; the forward and reverse boundaries
    // reset different timers and rotation baselines in the native state.
    const auto was_reverse = ReadFlag(state, 133u);
    state.words[34u] = 0u;
    WriteFlag(state, 133u, !was_reverse);
    if (!was_reverse) {
        for (std::size_t i = 0; i < 3u; ++i)
            WriteFloat(state, 26u + i, -ReadFloat(state, 26u + i));
        if (ReadFlag(state, 72u))
            runtime.ApplyRotation(state.words[32u], ReadVector(state, 20u),
                                  false);
        WriteFlag(state, 148u, true);
        state.words[39u] = 0u;
        return 1u;
    }

    WriteFloat(state, 35u, BiasedDuration(state, runtime));
    runtime.ApplyRotation(state.words[32u], ReadVector(state, 43u), false);
    if (ReadFlag(state, 72u)) {
        const auto reciprocal = 1.0f / ReadFloat(state, 35u);
        for (std::size_t i = 0; i < 3u; ++i)
            WriteFloat(state, 26u + i,
                       (ReadFloat(state, 20u + i) -
                        ReadFloat(state, 43u + i)) * reciprocal);
    } else {
        for (std::size_t i = 0; i < 3u; ++i)
            state.words[26u + i] = state.words[20u + i];
        if (ReadFlag(state, 73u)) {
            const auto reciprocal = 1.0f / ReadFloat(state, 35u);
            for (std::size_t i = 0; i < 3u; ++i)
                WriteFloat(state, 29u + i,
                           (ReadFloat(state, 23u + i) -
                            ReadFloat(state, 20u + i)) * reciprocal);
        }
    }
    WriteFlag(state, 160u, true);
    state.words[42u] = 0u;
    return 1u;
}

}  // namespace pal4
