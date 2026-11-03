#include "pal4/combat_ending_window_destructor.h"

namespace pal4 {

std::uint32_t CombatEndingWindowDestructor::Destruct(
    const std::uint32_t window,
    ICombatEndingWindowDestructorRuntime& runtime) {
    runtime.WriteWord(window, kVtable);
    runtime.WriteWord(window + 40u, kEmbeddedVtable);

    for (std::uint32_t i = 0; i < 4u; ++i) {
        const auto hint = runtime.ReadWord(window + 684u + 4u * i);
        if (hint != 0u) runtime.DeleteWindow(hint, 1u);
    }

    (void)CombatResultDataLifecycle::Destruct(window + 420u, runtime);

    const auto head = runtime.ReadWord(window + 48u);
    for (auto node = runtime.ReadWord(head); node != head;) {
        const auto next = runtime.ReadWord(node);
        const auto previous = runtime.ReadWord(node + 4u);
        runtime.WriteWord(previous, next);
        runtime.WriteWord(next + 4u, previous);
        if (runtime.ReadByte(node + 8u) != 0u) {
            const auto value = runtime.ReadWord(node + 12u);
            if (value != 0u) runtime.DeleteWindow(value, 1u);
        }
        (void)runtime.Free(node);
        runtime.WriteWord(window + 52u,
                          runtime.ReadWord(window + 52u) - 1u);
        node = next;
    }
    (void)runtime.Free(head);
    runtime.WriteWord(window + 48u, 0u);
    runtime.WriteWord(window + 52u, 0u);
    return runtime.DestructBase(window);
}

std::uint32_t CombatEndingWindowDestructor::DeletingDestruct(
    const std::uint32_t window, const std::uint8_t flags,
    ICombatEndingWindowDestructorRuntime& runtime) {
    (void)Destruct(window, runtime);
    if ((flags & 1u) != 0u) (void)runtime.Free(window);
    return window;
}

}  // namespace pal4
