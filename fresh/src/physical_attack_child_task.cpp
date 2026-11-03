#include "pal4/physical_attack_child_task.h"

namespace pal4 {

std::uint32_t PhysicalAttackChildTask::Create(
    IPhysicalAttackChildRuntime& runtime) {
    const auto object = runtime.Allocate(kSize);
    if (object == 0u) return 0u;
    struct Guard {
        IPhysicalAttackChildRuntime& runtime;
        std::uint32_t object;
        bool base_initialized = false;
        bool message_initialized = false;
        std::uint32_t effect_count = 0u;
        ~Guard() {
            if (object == 0u) return;
            if (message_initialized) {
                runtime.DestroyMessageBase(object);
            } else if (base_initialized) {
                for (auto count = effect_count; count > 0u; --count)
                    runtime.DestroyEffectSlot(object + 96u + 24u * (count - 1u));
                runtime.DestroyCombatBase(object);
            }
            runtime.Free(object);
        }
    } guard{runtime, object};

    runtime.ConstructBase(object);
    guard.base_initialized = true;
    runtime.WriteWord(object + 60u, 0u);
    // The native constructor copies an unwritten stack byte here.
    runtime.WriteByte(object + 64u, 0u);
    runtime.WriteWord(object + 68u, 0u);
    runtime.WriteWord(object + 72u, 0u);
    runtime.WriteWord(object + 76u, 0u);
    runtime.WriteWord(object, kConstructionVtable);
    for (std::uint32_t offset = 80u; offset < 96u; offset += 4u)
        runtime.WriteWord(object + offset, 0u);
    for (std::uint32_t index = 0u; index < 5u; ++index) {
        (void)CombatEffectSlot::Construct(object + 96u + index * 24u,
                                          runtime);
        ++guard.effect_count;
    }
    runtime.WriteWord(object, 0x8456c4u);
    guard.message_initialized = true;
    (void)NormalAttackTask::Construct(object + 216u, runtime);
    runtime.WriteWord(object, kVtable);
    guard.object = 0u;
    return object;
}

std::uint32_t PhysicalAttackChildTask::DestroyBase(
    const std::uint32_t object,
    IPhysicalAttackChildDestructionRuntime& runtime) {
    runtime.DestroyEmbeddedBase(object + 216u);
    return CombatProcessMessageTask::Destruct(object, runtime);
}

std::uint32_t CombatMessageTaskBase::Destroy(
    const std::uint32_t object, const std::uint8_t flags,
    ICombatMessageBaseDeletingRuntime& runtime) {
    runtime.DestroyBase(object);
    if ((flags & 1u) != 0u) runtime.Free(object);
    return object;
}

}  // namespace pal4
