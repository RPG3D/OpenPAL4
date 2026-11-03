#include "pal4/actor_direction_tasks.h"

#include <cmath>
#include <initializer_list>

namespace pal4 {
namespace {

void ClampComponents(ActorDirectionVector& vector) {
    for (float* component : {&vector.x, &vector.y, &vector.z}) {
        if (*component < -1.0f) *component = -1.0f;
        else if (*component > 1.0f) *component = 1.0f;
    }
}

}  // namespace

std::uint8_t TranslateActorTask::Process(
    const std::uint32_t task, ITranslateActorTaskRuntime& runtime) {
    const auto actor = runtime.ReadWord(task + 72u);
    if (actor) runtime.SetPosition(actor, task + 60u, 0u);
    return 1u;
}

std::uint32_t FaceTask::Construct(
    const std::uint32_t task, IActorDirectionTaskRuntime& runtime) {
    runtime.ConstructBase(task);
    runtime.WriteWord(task + 72u, 0u);
    runtime.WriteWord(task, kVtable);
    return task;
}

std::uint32_t FaceTask::Create(IFaceTaskCreationRuntime& runtime) {
    const auto task = runtime.Allocate(kSize);
    if (task == 0u) return 0u;
    try {
        return Construct(task, runtime);
    } catch (...) {
        runtime.Free(task);
        throw;
    }
}

std::uint8_t FaceTask::Process(
    const std::uint32_t task, IActorDirectionTaskRuntime& runtime) {
    auto actor = runtime.ReadWord(task + 72u);
    if (!actor) return 0u;
    runtime.WriteWord(task + 20u, 0u);

    const auto first = runtime.ChildPosition(actor);
    const auto x = runtime.ReadFloat(task + 60u) - first.x;
    actor = runtime.ReadWord(task + 72u);
    const auto second = runtime.ChildPosition(actor);
    const auto y = runtime.ReadFloat(task + 64u) - second.y;
    actor = runtime.ReadWord(task + 72u);
    const auto third = runtime.ChildPosition(actor);
    const auto z = runtime.ReadFloat(task + 68u) - third.z;
    ActorDirectionVector direction{x, y, z};
    constexpr double kMinimumDelta = 0.00009999999747378752;
    if (std::fabs(static_cast<double>(x)) < kMinimumDelta &&
        std::fabs(static_cast<double>(y)) < kMinimumDelta &&
        std::fabs(static_cast<double>(z)) < kMinimumDelta)
        return 1u;

    ActorDirectionVector forward{0.0f, 0.0f, 1.0f};
    runtime.Normalize(direction);
    ClampComponents(direction);
    runtime.Normalize(forward);
    ClampComponents(forward);
    const float dot = direction.x * forward.x +
                      direction.y * forward.y + direction.z * forward.z;
    // IDA55AB80..55ABDF keeps the CRT double until the actual degree store.
    const float angle = static_cast<float>(runtime.AngleRadians(dot) * 57.29577951308232);
    const float sign_test = direction.z * forward.x +
                            direction.y * forward.y +
                            direction.x * forward.z;
    const ActorDirectionVector rotation{
        0.0f, sign_test < 0.0f ? -angle : angle, 0.0f};
    runtime.SetFacing(runtime.ReadWord(task + 72u), rotation, 0u);
    return 1u;
}

std::uint8_t ChangeToPrepareActionTask::Process(
    const std::uint32_t task, IActorPrepareStateRuntime& runtime) {
    const auto actor = runtime.ReadWord(task + 60u);
    if (actor) runtime.SetPrepareState(actor, 1u);
    return 1u;
}

}  // namespace pal4
