#include "pal4/throw_property_task.h"

namespace pal4 {

std::uint32_t ThrowPropertyTask::Construct(
    const std::uint32_t task, IThrowPropertyConstructionRuntime& runtime) {
    runtime.ConstructBase(task);
    runtime.WriteWord(task, kVtable);
    for (const auto offset : {60u, 64u, 68u, 84u})
        runtime.WriteWord(task + offset, 0u);
    return task;
}

void ThrowPropertyTask::Destroy(
    const std::uint32_t task, IThrowPropertyTaskRuntime& runtime) {
    runtime.WriteWord(task, kVtable);
    CleanupInternal(task, runtime);
    runtime.DestroyBase(task);
}

std::uint32_t ThrowPropertyTask::Delete(
    const std::uint32_t task, const std::uint8_t flags,
    IThrowPropertyTaskRuntime& runtime) {
    Destroy(task, runtime);
    if ((flags & 1u) != 0u) runtime.FreeTask(task);
    return task;
}

std::uint32_t ThrowPropertyTask::SetActorsAndProperty(
    const std::uint32_t task, const std::uint32_t source,
    const std::uint32_t target, const std::uint32_t property,
    IThrowPropertyTaskRuntime& runtime) {
    runtime.WriteWord(task + 60u, source);
    runtime.WriteWord(task + 64u, target);
    runtime.WriteWord(task + 68u, property);
    return property;
}

std::uint8_t ThrowPropertyTask::Process(
    const std::uint32_t task, IThrowPropertyTaskRuntime& runtime) {
    const auto property = runtime.ReadWord(task + 68u);
    const auto source = runtime.ReadWord(task + 60u);
    const auto target = runtime.ReadWord(task + 64u);
    if (!property || !source || !target) return 1u;

    auto effect = runtime.CreateActorEffect(source, "EFF13", kEffectHook);
    runtime.ResetEffect(effect);
    runtime.LoadScripts();
    const auto component = runtime.CreateComponent();
    runtime.WriteWord(task + 84u, component);
    runtime.LoadComponentClump(component, "palobject",
                               runtime.PropertyName(property, 84u));
    runtime.SetComponentTexture(
        runtime.ReadWord(task + 84u),
        runtime.PropertyName(runtime.ReadWord(task + 68u), 100u));

    auto spawn = runtime.ActorPosition(runtime.ReadWord(task + 60u));
    spawn[1] += runtime.ActorBoundingHeight(runtime.ReadWord(task + 60u)) *
                0.60000002f;
    runtime.SetComponentPosition(runtime.ReadWord(task + 84u), spawn);

    const auto destination = runtime.ActorPosition(
        runtime.ReadWord(task + 64u));
    const auto origin = runtime.ActorPosition(
        runtime.ReadWord(task + 60u));
    ThrowPropertyPosition difference{
        destination[0] - origin[0], destination[1] - origin[1],
        destination[2] - origin[2]};
    for (std::uint32_t index = 0u; index < 3u; ++index)
        runtime.WriteFloat(task + 72u + index * 4u, difference[index]);
    const auto length = runtime.VectorLength(difference);
    const auto direction = runtime.Normalize(difference);
    for (std::uint32_t index = 0u; index < 3u; ++index)
        runtime.WriteFloat(task + 72u + index * 4u,
                           direction[index] * 600.0f);
    runtime.WriteFloat(task + 20u, length * 0.0016666667f);
    runtime.CleanupScripts();
    return 1u;
}

std::uint8_t ThrowPropertyTask::UpdatePosition(
    const std::uint32_t task, const float delta,
    IThrowPropertyTaskRuntime& runtime) {
    const auto component = runtime.ReadWord(task + 84u);
    if (component) {
        ThrowPropertyPosition next{};
        for (std::uint32_t index = 0u; index < 3u; ++index)
            next[index] = runtime.ReadFloat(component + 172u + index * 4u) +
                          runtime.ReadFloat(task + 72u + index * 4u) * delta;
        runtime.SetComponentPosition(component, next);
    }
    return 1u;
}

std::uint8_t ThrowPropertyTask::Cleanup(
    const std::uint32_t task, IThrowPropertyTaskRuntime& runtime) {
    CleanupInternal(task, runtime);
    return 1u;
}

void ThrowPropertyTask::CleanupInternal(
    const std::uint32_t task, IThrowPropertyTaskRuntime& runtime) {
    const auto component = runtime.ReadWord(task + 84u);
    if (!component) return;
    runtime.DeleteComponent(component);
    runtime.WriteWord(task + 84u, 0u);
}

}  // namespace pal4
