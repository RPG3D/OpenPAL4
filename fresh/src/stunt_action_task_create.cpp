#include "pal4/stunt_action_task.h"

namespace pal4 {

std::uint32_t StuntActionTask::Create(IStuntActionCreationRuntime& runtime) {
    const auto object = runtime.Allocate(kSize);
    if (object == 0u) return 0u;
    struct Guard {
        IStuntActionCreationRuntime& runtime;
        std::uint32_t object;
        bool base_initialized = false;
        bool combat_initialized = false;
        bool message_initialized = false;
        bool embedded_initialized = false;
        std::uint32_t effects = 0u;
        ~Guard() {
            if (object == 0u) return;
            if (embedded_initialized)
                runtime.DestroyEmbeddedBase(object + 256u);
            if (message_initialized) {
                runtime.DestroyMessageBase(object);
            } else if (combat_initialized) {
                for (auto count = effects; count > 0u; --count)
                    runtime.DestroyEffectSlot(object + 96u +
                                              24u * (count - 1u));
                runtime.DestroyCombatBase(object);
            } else if (base_initialized) {
                runtime.DestroyGenericBase(object);
            }
            runtime.Free(object);
        }
    } guard{runtime, object};

    runtime.ConstructBase(object);
    guard.base_initialized = true;
    runtime.WriteWord(object + 60u, 0u);
    // The native constructor copies an unwritten stack byte.
    runtime.WriteByte(object + 64u, 0u);
    runtime.SetTextErrorFlag(object + 64u, 0u);
    runtime.WriteWord(object, 0x845a24u);
    guard.combat_initialized = true;
    for (std::uint32_t offset = 80u; offset < 96u; offset += 4u)
        runtime.WriteWord(object + offset, 0u);
    for (std::uint32_t index = 0u; index < 5u; ++index) {
        (void)CombatEffectSlot::Construct(object + 96u + index * 24u,
                                          runtime);
        ++guard.effects;
    }
    runtime.WriteWord(object, 0x8456c4u);
    guard.message_initialized = true;
    for (std::uint32_t offset = 216u; offset < 256u; offset += 4u)
        runtime.WriteWord(object + offset, 0u);
    runtime.ConstructBase(object + 256u);
    guard.embedded_initialized = true;
    runtime.WriteWord(object + 256u, StuntActionEmbeddedTask::kVtable);
    runtime.WriteWord(object, kVtable);
    guard.object = 0u;
    return object;
}

std::uint32_t StuntActionEmbeddedTask::Create(
    IStuntActionEmbeddedCreationRuntime& runtime) {
    const auto object = runtime.Allocate(kSize);
    if (object == 0u) return 0u;
    try {
        runtime.ConstructBase(object);
        runtime.WriteWord(object, kVtable);
    } catch (...) {
        runtime.Free(object);
        throw;
    }
    return object;
}

}  // namespace pal4
