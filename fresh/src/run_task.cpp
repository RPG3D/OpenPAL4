#include "pal4/run_task.h"

namespace pal4 {

std::uint32_t RunTask::Construct(
    const std::uint32_t task, IRunTaskRuntime& runtime) {
    runtime.ConstructBase(task);
    runtime.WriteWord(task + 72u, 0u);
    runtime.WriteWord(task, kVtable);
    return task;
}

std::uint32_t RunTask::Create(IRunTaskCreationRuntime& runtime) {
    const auto task = runtime.Allocate(kSize);
    if (task == 0u) return 0u;
    try {
        return Construct(task, runtime);
    } catch (...) {
        runtime.Free(task);
        throw;
    }
}

std::uint32_t RunTask::SetTarget(
    const std::uint32_t task, const std::uint32_t actor,
    const std::uint32_t source_position, IRunTaskRuntime& runtime) {
    runtime.WriteWord(task + 72u, actor);
    runtime.WriteWord(task + 60u, runtime.ReadWord(source_position));
    const auto second = runtime.ReadWord(source_position + 4u);
    runtime.WriteWord(task + 64u, second);
    runtime.WriteWord(task + 68u, runtime.ReadWord(source_position + 8u));
    return second;
}

std::uint8_t RunTask::Process(
    const std::uint32_t task, IRunTaskRuntime& runtime) {
    if (!runtime.ReadWord(task + 72u)) return 1u;
    const auto first = runtime.ChildPosition(runtime.ReadWord(task + 72u));
    const auto x = runtime.ReadFloat(task + 60u) - first.x;
    runtime.WriteFloat(task + 76u, x);
    const auto target_y = runtime.ReadFloat(task + 64u);
    const auto second = runtime.ChildPosition(runtime.ReadWord(task + 72u));
    const auto y = target_y - second.y;
    runtime.WriteFloat(task + 80u, y);
    const auto target_z = runtime.ReadFloat(task + 68u);
    const auto third = runtime.ChildPosition(runtime.ReadWord(task + 72u));
    const auto z = target_z - third.z;
    runtime.WriteFloat(task + 84u, z);
    RunTaskPosition delta{x, y, z};
    runtime.WriteFloat(task + 20u, static_cast<float>(
        static_cast<double>(runtime.CalculateMagnitude(delta)) *
        static_cast<double>(0.0027777778f)));
    runtime.Normalize(delta);
    runtime.WriteFloat(task + 76u, delta.x);
    runtime.WriteFloat(task + 80u, delta.y);
    runtime.WriteFloat(task + 84u, delta.z);
    runtime.NotifyRun(runtime.ReadWord(task + 72u), kRunPayload, 1u);
    return 1u;
}

std::uint8_t RunTask::UpdatePosition(
    const std::uint32_t task, float step, IRunTaskRuntime& runtime) {
    if (!runtime.ReadWord(task + 72u)) return 1u;
    const auto duration = runtime.ReadFloat(task + 20u);
    const auto elapsed = runtime.ReadFloat(task + 32u);
    if (static_cast<double>(step) + elapsed > duration)
        step = duration - elapsed;
    if (!(step > 0.0f)) return 1u;

    //55A5C5 multiplies under the game's PC24 mode before each direction
    // multiply. Keep that rounding boundary explicit on modern SSE hosts.
    const float scale = step * 360.0f;
    const auto x_step = static_cast<float>(runtime.ReadFloat(task + 76u) * scale);
    const auto y_step = static_cast<float>(runtime.ReadFloat(task + 80u) * scale);
    const auto z_step = static_cast<float>(runtime.ReadFloat(task + 84u) * scale);
    const auto first = runtime.ChildPosition(runtime.ReadWord(task + 72u));
    const auto second = runtime.ChildPosition(runtime.ReadWord(task + 72u));
    const auto third = runtime.ChildPosition(runtime.ReadWord(task + 72u));
    const RunTaskPosition position{
        first.x + x_step, second.y + y_step, third.z + z_step};
    (void)runtime.SetChildPosition(runtime.ReadWord(task + 72u),
                                   position, 0u);
    return 1u;
}

}  // namespace pal4
