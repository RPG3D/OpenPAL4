#include "pal4/dialog_layout_base.h"

#include <cstdint>

namespace pal4 {

namespace {

void ReleaseBuffer(const std::uint32_t record,
                   IDialogLayoutLifecycleRuntime& runtime) {
    const auto pointer = runtime.ReadWord(record + 4u);
    if (pointer != 0u) {
        const auto reference = runtime.ReadByte(pointer - 1u);
        if (reference == 0u || reference == 0xffu) {
            runtime.Free(pointer - 1u);
        } else {
            runtime.WriteByte(pointer - 1u,
                              static_cast<std::uint8_t>(
                                  reference - 1u));
        }
    }
    runtime.WriteWord(record + 4u, 0u);
    runtime.WriteWord(record + 8u, 0u);
    runtime.WriteWord(record + 12u, 0u);
}

}  // namespace

std::uint32_t DialogLayoutBase::DeletingDestruct(
    const std::uint32_t object, const std::uint8_t flags,
    IDialogLayoutLifecycleRuntime& runtime) {
    Destruct(object, runtime);
    if ((flags & 1u) != 0u) runtime.Free(object);
    return object;
}

void DialogLayoutBase::Destruct(
    const std::uint32_t object,
    IDialogLayoutLifecycleRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    SetParent(object, false, 0u, runtime);
    runtime.DestroyWindow(runtime.ReadWord(object + 36u));
    ReleaseBuffer(object + 20u, runtime);
    ReleaseBuffer(object + 4u, runtime);
}

void DialogLayoutBase::SetParent(
    const std::uint32_t object, const bool attach,
    std::uint32_t parent,
    IDialogLayoutParentRuntime& runtime) {
    const auto root = runtime.ReadWord(object + 36u);
    runtime.BuildUtf8Buffer(root + 0x37cu);
    if (parent == 0u) parent = runtime.DesktopWindow();
    if (attach) {
        if (runtime.IsChild(parent, root)) return;
        const auto old_parent = runtime.ReadWord(root + 68u);
        if (old_parent != 0u) runtime.RemoveChild(old_parent, root);
        runtime.AddChild(parent, root);
    } else if (runtime.IsChild(parent, root)) {
        runtime.RemoveChild(parent, root);
    }
}

bool DialogLayoutBase::IsVisible(
    const std::uint32_t object,
    IDialogLayoutParentRuntime& runtime) {
    const auto root = runtime.ReadWord(object + 36u);
    return runtime.ReadWord(root + 68u) != 0u;
}

}  // namespace pal4
