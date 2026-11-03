#include "pal4/smith_window.h"

#include <array>
#include <bit>
#include <cstdint>
#include <string>

namespace pal4 {
namespace {

const std::array<float, 2> kDialogSize{
    std::bit_cast<float>(0x3e99999au),
    std::bit_cast<float>(0x3e99999au),
};

}  // namespace

std::uint8_t SmithWindow::OnEquipmentSelectionPrompt(
    const std::uint32_t object, const std::uint32_t event,
    ISmithWindowEquipmentPromptRuntime& runtime) {
    if (event != 1u) return 1u;

    (void)runtime.PrescriptionHoverItemId(object);
    (void)runtime.EquipmentHoverItemId(object);
    runtime.CreateCommonDialog(runtime.UIFrameManager());
    auto callback = runtime.CreateSelectCallback(object);
    try {
        const auto id = runtime.EquipmentHoverItemId(object);
        const auto item = runtime.FindInventoryItem(
            static_cast<std::uint32_t>(id), 7u);
        runtime.AssignText(object + 0xd8u, item + 0x98u);
        const std::string message(runtime.DialogTemplate(0x5cu));
        runtime.ShowDialog(message, kDialogSize, 3u, callback);
    } catch (...) {
        if (callback != 0u) runtime.DestroyCallback(callback, 1u);
        throw;
    }
    if (callback != 0u) runtime.DestroyCallback(callback, 1u);
    return 1u;
}

std::uint8_t SmithWindow::OnPageChangePrompt(
    const std::uint32_t object, const std::uint32_t event,
    ISmithWindowEquipmentPromptRuntime& runtime) {
    if (event != 1u) return 1u;

    runtime.CreateCommonDialog(runtime.UIFrameManager());
    auto callback = runtime.CreatePageChangeCallback(object);
    try {
        const auto id = runtime.EquipmentHoverItemId(object);
        (void)runtime.FindInventoryItem(
            static_cast<std::uint32_t>(id), 7u);
        const std::string message(runtime.DialogTemplate(0x5eu));
        runtime.ShowDialog(message, kDialogSize, 3u, callback);
    } catch (...) {
        if (callback != 0u) runtime.DestroyCallback(callback, 1u);
        throw;
    }
    if (callback != 0u) runtime.DestroyCallback(callback, 1u);
    return 1u;
}

}  // namespace pal4
