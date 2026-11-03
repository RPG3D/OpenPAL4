#include "pal4/stored_transform_task.h"

namespace pal4 {

std::uint32_t StoredTransformTask::Construct(
    const std::uint32_t object, IStoredTransformTaskRuntime& runtime) {
    runtime.BaseConstruct(object);
    runtime.WriteWord(object, kVtable);
    runtime.WriteWord(object + 0x3cu, 0u);
    return object;
}

std::uint32_t StoredTransformTask::Destruct(
    const std::uint32_t object, IStoredTransformTaskRuntime& runtime) {
    runtime.WriteWord(object + 0x3cu, 0u);
    runtime.WriteWord(object, 0x8486fcu);
    return runtime.BaseDestruct(object);
}

std::uint8_t StoredTransformTask::Apply(
    const std::uint32_t object, IStoredTransformTaskRuntime& runtime) {
    runtime.ApplyStoredTransform(runtime.ReadWord(object + 0x3cu),
                                 {0u, 0u, 0u});
    return 1u;
}

bool StoredTransformTask::IsWaitComplete(
    const std::uint32_t object, IStoredTransformTaskRuntime& runtime) {
    return runtime.WaitComplete(object);
}

std::uint32_t StoredTransformTask::SetOwner(
    const std::uint32_t object, const std::uint32_t owner,
    IStoredTransformTaskRuntime& runtime) {
    runtime.WriteWord(object + 0x3cu, owner);
    return owner;
}

}  // namespace pal4
