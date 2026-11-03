#include "pal4/role_selector_window.h"

namespace pal4 {

std::uint32_t RoleSelectorWindow::CleanupCallback(
    const std::uint32_t object,
    IRoleSelectorLifecycleRuntime& runtime) {
    const auto slot = runtime.ReadWord(object + 64u);
    std::uint32_t result;
    if (slot != 0u) {
        const auto callback = runtime.ReadWord(slot);
        if (callback != 0u)
            runtime.DestroyCallback(callback, 1u);
        result = runtime.Free(slot);
    } else {
        result = runtime.EmptyCallbackResult();
    }
    runtime.WriteWord(object + 64u, 0u);
    runtime.WriteWord(object + 68u, 0u);
    return result;
}

std::uint32_t RoleSelectorWindow::Destruct(
    const std::uint32_t object,
    IRoleSelectorLifecycleRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    runtime.WriteWord(object + 40u, kNavigationVtable);
    (void)CleanupCallback(object, runtime);
    (void)runtime.Free(runtime.ReadWord(object + 48u));
    runtime.WriteWord(object + 48u, 0u);
    runtime.WriteWord(object + 52u, 0u);
    runtime.WriteWord(object + 56u, 0u);
    return runtime.DestructBase(object);
}

std::uint32_t RoleSelectorWindow::ShowOrHide(
    const std::uint32_t visible,
    const std::uint32_t callback_source,
    const std::uint32_t context,
    IRoleSelectorVisibilityRuntime& runtime) {
    constexpr std::uint32_t kName = 0x8b1a60u;
    if ((visible & 0xffu) == 0u)
        return runtime.Dispatch(kName, 0u, 0u);

    (void)runtime.Dispatch(kName, 1u, 0u);
    const auto frame = runtime.FindFrame(kName);
    (void)CleanupCallback(frame, runtime);
    if (callback_source != 0u) {
        const auto slot = runtime.Allocate(4u);
        if (slot != 0u) {
            try {
                runtime.WriteWord(slot, runtime.CloneCallback(
                    runtime.ReadWord(callback_source)));
            } catch (...) {
                (void)runtime.Free(slot);
                throw;
            }
        }
        runtime.WriteWord(frame + 64u, slot);
        runtime.WriteWord(frame + 68u, context);
    }
    return runtime.LayoutButtons(frame);
}

void RoleSelectorWindow::SetModalState(
    const std::uint32_t object, const std::uint32_t visible,
    const std::uint32_t parent,
    IRoleSelectorVisibilityRuntime& runtime) {
    if ((visible & 0xffu) == 0u)
        (void)CleanupCallback(object, runtime);
    runtime.SetModal(runtime.ReadWord(object + 36u),
                     (visible & 0xffu) != 0u);
    runtime.SetParent(object, visible, parent);
}

}  // namespace pal4
