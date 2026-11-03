#include "pal4/ui_frame_manager_singleton.h"

#include <array>

namespace pal4 {

std::uint32_t UiFrameManagerSingleton::Cleanup(
    const std::uint32_t manager, IUiFrameManagerCleanupRuntime& runtime) {
    runtime.DisableEffects(manager);

    constexpr std::array<std::uint32_t, 11> kFrameOffsets{
        0x1c0u, 0x1c4u, 0x1a8u, 0x1acu, 0x1b0u, 0x1ccu,
        0x1d0u, 0x1b4u, 0x1b8u, 0x1bcu, 0x1c8u};
    for (const auto offset : kFrameOffsets) {
        const auto frame = runtime.ReadWord(manager + offset);
        if (frame == 0u) continue;
        runtime.DestroyFrame(frame, 1u);
        runtime.WriteWord(manager + offset, 0u);
    }

    const auto begin = runtime.ReadWord(manager + 0x18cu);
    auto end = runtime.ReadWord(manager + 0x190u);
    for (auto slot = begin; slot != end; slot += 4u) {
        const auto frame = runtime.ReadWord(slot);
        if (frame != 0u) runtime.DestroyFrame(frame, 1u);
        end = runtime.ReadWord(manager + 0x190u);
    }
    (void)runtime.ReadWord(manager + 0x190u);
    runtime.WriteWord(manager + 0x190u,
                      runtime.ReadWord(manager + 0x18cu));

    const auto collection = runtime.ReadWord(manager + 0x19cu);
    if (collection != 0u) {
        runtime.CleanupCollectionElements(collection);
        runtime.CleanupGameObjects(runtime.ReadWord(manager + 0x19cu));
        const auto current = runtime.ReadWord(manager + 0x19cu);
        if (current != 0u) {
            runtime.DestructCollection(current);
            (void)runtime.Free(current);
        }
        runtime.WriteWord(manager + 0x19cu, 0u);
    }

    const auto effects = runtime.ReadWord(manager + 0x1a0u);
    if (effects != 0u) {
        runtime.CleanupTimedEffects(effects);
        (void)runtime.Free(effects);
        runtime.WriteWord(manager + 0x1a0u, 0u);
    }

    runtime.DestructEmbeddedTree(manager + 0x114u);
    auto result = runtime.ClearSpinnerPool();
    const auto gui = runtime.ReadWord(manager + 0x198u);
    if (gui != 0u) {
        runtime.DestructGui(gui);
        result = runtime.Free(gui);
        runtime.WriteWord(manager + 0x198u, 0u);
    }
    return result;
}

}  // namespace pal4
