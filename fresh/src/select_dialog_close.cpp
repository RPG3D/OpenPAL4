#include "pal4/select_dialog.h"

#include <cstdint>

namespace pal4 {

std::uint32_t SelectDialog::HideAndClearList(
    const std::uint32_t object, ISelectDialogCloseRuntime& runtime) {
    const auto root = runtime.ReadWord(object + 36u);
    runtime.WriteWord(object + 160u, 0u);
    runtime.SetModalState(root, false);
    const auto parent = runtime.ReadWord(root + 68u);
    if (parent != 0u) runtime.RemoveChild(parent, root);

    const auto active_begin = runtime.ReadWord(object + 104u);
    const auto active_end = runtime.ReadWord(object + 108u);
    for (auto cursor = active_begin; cursor != active_end;
         cursor += 4u) {
        const auto button = runtime.ReadWord(cursor);
        runtime.RemoveChild(runtime.ReadWord(object + 152u), button);
        runtime.AppendAvailableButton(object + 84u,
                                      runtime.ReadWord(object + 92u),
                                      button);
    }
    runtime.WriteWord(object + 108u, active_begin);

    const auto begin = runtime.ReadWord(object + 72u);
    const auto end = runtime.ReadWord(object + 76u);
    auto result = end;
    for (auto record = begin; record != end; record += 16u) {
        const auto pointer = runtime.ReadWord(record + 4u);
        if (pointer != 0u) {
            const auto reference = runtime.ReadByte(pointer - 1u);
            if (reference == 0u || reference == 0xffu) {
                runtime.FreeString(
                    record, pointer - 1u,
                    runtime.ReadWord(record + 12u) + 2u);
            } else {
                runtime.WriteByte(pointer - 1u,
                                  static_cast<std::uint8_t>(
                                      reference - 1u));
            }
        }
        runtime.WriteWord(record + 4u, 0u);
        runtime.WriteWord(record + 8u, 0u);
        runtime.WriteWord(record + 12u, 0u);
        result = record;
    }
    runtime.WriteWord(object + 76u, begin);
    return result;
}

std::uint32_t SelectDialog::CleanupCallback(
    const std::uint32_t object,
    ISelectDialogCallbackRuntime& runtime) {
    const auto allocation = runtime.ReadWord(object + 64u);
    if (allocation == 0u) return 0u;
    const auto callback = runtime.ReadWord(allocation);
    if (callback != 0u) runtime.DestroyCallback(callback, 1u);
    const auto result = runtime.Free(allocation);
    runtime.WriteWord(object + 64u, 0u);
    return result;
}

std::uint32_t SelectDialog::ConstructCallback(
    const std::uint32_t allocation,
    const std::uint32_t handler,
    const std::uint32_t argument,
    const std::uint32_t owner,
    ISelectDialogCallbackRuntime& runtime) {
    runtime.WriteWord(allocation + 8u, handler);
    runtime.WriteWord(allocation + 12u, argument);
    runtime.WriteWord(allocation + 16u, owner);
    runtime.WriteWord(allocation, kCallbackVtable);
    return allocation;
}

}  // namespace pal4
