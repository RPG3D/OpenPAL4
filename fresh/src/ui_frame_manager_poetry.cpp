#include "pal4/ui_frame_manager_singleton.h"

namespace pal4 {

std::uint32_t UiFrameManagerSingleton::CreatePoetryWindow(
    const std::uint32_t manager, const std::int32_t index,
    IUiFrameManagerPoetryRuntime& runtime) {
    const auto field = manager + 0x1d0u;
    const auto previous = runtime.ReadWord(field);
    if (previous != 0u) {
        runtime.DestroyPoetry(previous, 1u);
        runtime.WriteWord(field, 0u);
    }

    const auto name_buffer = runtime.InitializeName();
    auto created = 0u;
    try {
        const auto name = runtime.FormatName(
            name_buffer, "PoetryWindow%d", index);
        const auto memory = runtime.Allocate(0xb8u);
        if (memory != 0u) {
            try {
                created = runtime.ConstructPoetry(memory, name);
            } catch (...) {
                runtime.FreeAfterConstructionFailure(memory);
                throw;
            }
        }
        runtime.WriteWord(field, created);
    } catch (...) {
        runtime.CleanupName(name_buffer);
        throw;
    }
    runtime.CleanupName(name_buffer);
    return created;
}

std::uint32_t UiFrameManagerSingleton::CreateAndHidePoetryWindow(
    const std::uint32_t manager, const std::int32_t index,
    IUiFrameManagerPoetryRuntime& runtime) {
    const auto poetry = CreatePoetryWindow(manager, index, runtime);
    return runtime.HidePoetry(poetry);
}

std::uint32_t UiFrameManagerSingleton::GetPoetryWindow(
    const std::uint32_t manager, IUiFrameManagerPoetryRuntime& runtime) {
    return runtime.ReadWord(manager + 0x1d0u);
}

}  // namespace pal4
