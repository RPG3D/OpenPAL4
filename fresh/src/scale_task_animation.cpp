#include "pal4/scale_task.h"

#include "scale_task_state.h"

namespace pal4 {
using namespace scale_task_detail;

std::uint8_t ScaleTask::Update(ScaleTaskState& state, float elapsed,
                                IScaleTaskRuntime& runtime) {
    const auto rollback = ReadFlag(state, 104u);
    if (rollback) {
        if (ReadFlag(state, 120u)) {
            if (ReadFloat(state, 32u) < ReadFloat(state, 31u)) {
                WriteFloat(state, 32u, ReadFloat(state, 32u) + elapsed);
                return 1u;
            }
            WriteFlag(state, 120u, false);
            state.words[32u] = 0u;
        }
        if (ReadFlag(state, 132u)) {
            if (ReadFloat(state, 35u) < ReadFloat(state, 34u)) {
                WriteFloat(state, 35u, ReadFloat(state, 35u) + elapsed);
                return 1u;
            }
            WriteFlag(state, 132u, false);
            state.words[35u] = 0u;
        }
    }

    auto step = elapsed;
    if (rollback || ReadFlag(state, 72u)) {
        const auto total = elapsed + ReadFloat(state, 27u);
        if (total < ReadFloat(state, 28u)) {
            WriteFloat(state, 27u, total);
        } else {
            step = ReadFloat(state, 28u) - ReadFloat(state, 27u);
            state.words[27u] = state.words[28u];
        }
    }

    auto current = ReadVector(state, 36u);
    runtime.ReadScale(state.words[25u], current);
    for (std::size_t i = 0; i < current.size(); ++i)
        current[i] += step * ReadFloat(state, 19u + i);
    runtime.SetScale(state.words[25u], current, 0u);

    if (!rollback || ReadFloat(state, 27u) < ReadFloat(state, 28u))
        return 1u;

    const auto was_reverse = ReadFlag(state, 105u);
    state.words[27u] = 0u;
    WriteFlag(state, 105u, !was_reverse);
    if (!was_reverse) {
        for (std::size_t i = 0; i < 3u; ++i)
            WriteFloat(state, 19u + i, -ReadFloat(state, 19u + i));
        if (ReadFlag(state, 72u))
            runtime.SetScale(state.words[25u], ReadVector(state, 22u), 0u);
        WriteFlag(state, 120u, true);
        state.words[32u] = 0u;
        return 1u;
    }

    WriteFloat(state, 28u, BiasedDuration(state, runtime));
    runtime.SetScale(state.words[25u], ReadVector(state, 36u), 0u);
    if (ReadFlag(state, 72u)) {
        const auto reciprocal = 1.0f / ReadFloat(state, 28u);
        for (std::size_t i = 0; i < 3u; ++i)
            WriteFloat(state, 19u + i,
                       (ReadFloat(state, 22u + i) -
                        ReadFloat(state, 36u + i)) * reciprocal);
    } else {
        for (std::size_t i = 0; i < 3u; ++i)
            state.words[19u + i] = state.words[22u + i];
    }
    WriteFlag(state, 132u, true);
    state.words[35u] = 0u;
    return 1u;
}

}  // namespace pal4
