#include "pal4/gob_obstacle.h"

namespace pal4 {

std::uint32_t GobObstacle::Create(IGobObstacleRuntime& runtime) {
    const auto object = runtime.Allocate(kSize);
    if (object == 0u) return 0u;
    try {
        return Construct(object, runtime);
    } catch (...) {
        runtime.Free(object);
        throw;
    }
}

std::uint32_t GobObstacle::Construct(
    const std::uint32_t object, IGobObstacleRuntime& runtime) {
    runtime.ConstructMachine(object);
    runtime.InitializeObstacleHelper(object + kHelperOffset);
    runtime.InitializeStringRecord(object + 0x308u);
    runtime.InitializeStringRecord(object + 0x320u);
    runtime.InitializeEffectHandle(object + kEffectHandleOffset);
    runtime.WriteWord(object, kVtable);
    runtime.WriteWord(object + kStateOffset, kStateVtable);
    runtime.WriteWord(object + 0x174u, 1u);
    runtime.WriteWord(object + 0x2f4u, 0u);
    return object;
}

std::uint32_t GobObstacle::GetObstacleRuntimeHelper(
    const std::uint32_t object) {
    return object + kHelperOffset;
}

std::uint32_t GobObstacle::TypeKey() { return 10u; }

void GobObstacle::Destruct(const std::uint32_t object,
                           IGobObstacleRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    runtime.WriteWord(object + kStateOffset, kStateVtable);
    runtime.ResetEffectHandle(object + kEffectHandleOffset);
    runtime.ReleaseStringRecord(object + 0x320u);
    runtime.ReleaseStringRecord(object + 0x308u);
    runtime.DestructObstacleHelper(object + kHelperOffset);
    runtime.DestructMachine(object);
}

std::uint32_t GobObstacle::UpdateDialogAndCleanup(
    const std::uint32_t object, IGobObstacleRuntime& runtime) {
    const auto handle = object + kEffectHandleOffset;
    if (runtime.DialogNeedsUpdate(handle))
        runtime.UpdateDialogContent(handle);
    return runtime.CleanupMachineResources(object);
}

}  // namespace pal4
