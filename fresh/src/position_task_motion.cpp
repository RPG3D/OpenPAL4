#include "pal4/position_task.h"

#include "position_task_state.h"

namespace pal4 {
namespace {
using namespace position_task_detail;

float RandomizedDuration(PositionTaskState& state,
                         IPositionTaskRuntime& runtime) {
    const auto bias = ReadFloat(state, 80u);
    return ReadFloat(state, 20u) - bias +
           static_cast<float>(runtime.RandomInt()) *
               (2.0f * bias) * 0.000030518509f;
}

void PrepareVelocity(PositionTaskState& state) {
    if (ReadByte(state, 72u) != 0u) {
        const auto current = ReadVector(state, 192u);
        const auto target = ReadVector(state, 88u);
        WriteVector(state, 124u, target);
        PositionVector velocity{};
        for (std::size_t i = 0u; i < velocity.size(); ++i)
            velocity[i] = (target[i] - current[i]) / ReadFloat(state, 76u);
        WriteVector(state, 100u, velocity);
        return;
    }
    const auto velocity = ReadVector(state, 88u);
    WriteVector(state, 100u, velocity);
    if (ReadByte(state, 73u) != 0u) {
        const auto added = ReadVector(state, 204u);
        PositionVector acceleration{};
        const auto configured_duration = ReadFloat(state, 76u);
        const auto duration = configured_duration <= 0.0f
            ? 1.0f : configured_duration;
        for (std::size_t i = 0u; i < acceleration.size(); ++i)
            acceleration[i] = (added[i] - velocity[i]) / duration;
        WriteVector(state, 112u, acceleration);
    }
}

void FaceVelocity(PositionTaskState& state, IPositionTaskRuntime& runtime,
                  bool result_mode) {
    if (ReadByte(state, 136u) == 0u)
        return;
    auto direction = result_mode
        ? ReadVector(state, 124u) : ReadVector(state, 100u);
    if (!result_mode)
        for (auto& component : direction) component *= 100.0f;
    const auto facet = runtime.ResolveFaceFacet(state.words[16u]);
    if (facet != 0u) {
        runtime.Face(facet, direction);
        runtime.SetRotation(facet, ReadVector(state, 140u));
        runtime.ActivateFace(facet);
    }
}
}  // namespace

std::uint8_t PositionTask::Start(PositionTaskState& state,
                                 IPositionTaskRuntime& runtime) {
    using namespace position_task_detail;
    const auto facet = runtime.ResolvePositionFacet(
        state.words[16u], ClassName());
    state.words[38u] = facet;
    const auto current = runtime.GetPosition(facet);
    WriteVector(state, 192u, current);
    WriteVector(state, 216u, current);
    WriteByte(state, 189u, 0u);
    WriteFloat(state, 76u, ReadFloat(state, 20u));
    if (ReadByte(state, 188u) != 0u)
        WriteFloat(state, 76u, RandomizedDuration(state, runtime));
    PrepareVelocity(state);
    FaceVelocity(state, runtime, ReadByte(state, 72u) != 0u);
    const auto control = runtime.ResolveControlFacet(state.words[16u]);
    state.words[40u] = control;
    if (control != 0u) {
        WriteByte(state, 156u, runtime.ControlEnabled(control));
        runtime.SetControlEnabled(control, 1u);
    }
    WriteByte(state, 164u, 0u);
    WriteFloat(state, 172u, 0.0f);
    WriteByte(state, 176u, 0u);
    WriteFloat(state, 184u, 0.0f);
    return 1u;
}

std::uint8_t PositionTask::Update(PositionTaskState& state, float elapsed,
                                  IPositionTaskRuntime& runtime) {
    using namespace position_task_detail;
    const auto rollback = ReadByte(state, 188u) != 0u;
    if (rollback) {
        if (ReadByte(state, 164u) != 0u) {
            if (ReadFloat(state, 172u) < ReadFloat(state, 168u)) {
                WriteFloat(state, 172u, ReadFloat(state, 172u) + elapsed);
                return 1u;
            }
            WriteByte(state, 164u, 0u);
            WriteFloat(state, 172u, 0.0f);
        }
        if (ReadByte(state, 176u) != 0u) {
            if (ReadFloat(state, 184u) < ReadFloat(state, 180u)) {
                WriteFloat(state, 184u, ReadFloat(state, 184u) + elapsed);
                return 1u;
            }
            WriteByte(state, 176u, 0u);
            WriteFloat(state, 184u, 0.0f);
        }
    }

    const auto result_mode = ReadByte(state, 72u) != 0u;
    auto delta = elapsed;
    if (result_mode || rollback) {
        const auto remaining = ReadFloat(state, 76u) - ReadFloat(state, 84u);
        if (elapsed < remaining)
            WriteFloat(state, 84u, ReadFloat(state, 84u) + elapsed);
        else {
            delta = remaining;
            WriteFloat(state, 84u, ReadFloat(state, 76u));
        }
    }
    if (!result_mode && ReadByte(state, 73u) != 0u) {
        auto velocity = ReadVector(state, 100u);
        const auto acceleration = ReadVector(state, 112u);
        for (std::size_t i = 0u; i < velocity.size(); ++i)
            velocity[i] += delta * acceleration[i];
        WriteVector(state, 100u, velocity);
    }
    auto movement = ReadVector(state, 100u);
    for (auto& component : movement) component *= delta;
    runtime.Move(state.words[38u], movement, true);

    if (rollback && ReadFloat(state, 84u) >= ReadFloat(state, 76u)) {
        const auto returning = ReadByte(state, 189u) != 0u;
        WriteFloat(state, 84u, 0.0f);
        WriteByte(state, 189u, returning ? 0u : 1u);
        if (returning) {
            WriteFloat(state, 76u, RandomizedDuration(state, runtime));
            runtime.Move(state.words[38u], ReadVector(state, 192u), false);
            PrepareVelocity(state);
            WriteByte(state, 176u, 1u);
            WriteFloat(state, 184u, 0.0f);
        } else {
            WriteByte(state, 164u, 1u);
            WriteFloat(state, 172u, 0.0f);
            auto velocity = ReadVector(state, 100u);
            for (auto& component : velocity) component = -component;
            WriteVector(state, 100u, velocity);
        }
    }
    if (!result_mode)
        FaceVelocity(state, runtime, false);
    return 1u;
}

std::uint8_t PositionTask::Stop(PositionTaskState& state,
                                IPositionTaskRuntime& runtime) {
    using namespace position_task_detail;
    if (ReadByte(state, 72u) != 0u)
        runtime.Move(state.words[38u], ReadVector(state, 124u), false);
    state.words[38u] = 0u;
    const auto control = state.words[40u];
    if (control != 0u) {
        runtime.SetControlEnabled(control, ReadByte(state, 156u));
        state.words[40u] = 0u;
    }
    return 1u;
}

}  // namespace pal4
