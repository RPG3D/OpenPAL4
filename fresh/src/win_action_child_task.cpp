#include "pal4/win_action_child_task.h"

namespace pal4 {

std::uint32_t WinActionSecondChildTask::Create(
    IWinActionSimpleCreationRuntime& runtime) {
    const auto object = runtime.Allocate(kSize);
    if (object == 0u) return 0u;
    try {
        runtime.ConstructBase(object);
    } catch (...) {
        runtime.Free(object);
        throw;
    }
    try {
        runtime.WriteWord(object + 60u, 0u);
        runtime.WriteWord(object, kVtable);
    } catch (...) {
        runtime.DestroyBase(object);
        runtime.Free(object);
        throw;
    }
    return object;
}

std::uint32_t WinActionThirdChildTask::Create(
    IWinActionChildCreationRuntime& runtime) {
    const auto object = runtime.Allocate(kSize);
    if (object == 0u) return 0u;
    struct Guard {
        IWinActionChildCreationRuntime& runtime;
        std::uint32_t object;
        bool base_initialized = false;
        bool message_initialized = false;
        std::uint32_t effects = 0u;
        ~Guard() {
            if (object == 0u) return;
            if (message_initialized) {
                runtime.DestroyMessageBase(object);
            } else if (base_initialized) {
                for (auto count = effects; count > 0u; --count)
                    runtime.DestroyEffectSlot(object + 96u +
                                              24u * (count - 1u));
                runtime.DestroyBase(object);
            }
            runtime.Free(object);
        }
    } guard{runtime, object};

    runtime.ConstructBase(object);
    guard.base_initialized = true;
    runtime.WriteWord(object + 60u, 0u);
    // The native constructor copies an unwritten stack byte.
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
        ++guard.effects;
    }
    runtime.WriteWord(object, kVtable);
    guard.message_initialized = true;
    guard.object = 0u;
    return object;
}

}  // namespace pal4
