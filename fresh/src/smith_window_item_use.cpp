#include "pal4/smith_window.h"

#include <array>
#include <bit>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {
namespace {

const std::array<float, 2> kDialogSize{
    std::bit_cast<float>(0x3e99999au),
    std::bit_cast<float>(0x3e99999au),
};

template <typename AfterDialog>
void ShowWithCallback(const std::uint32_t object,
                      const std::string_view message,
                      ISmithWindowItemUseRuntime& runtime,
                      AfterDialog&& after_dialog) {
    auto callback = runtime.CreateCloseCallback(object);
    try {
        runtime.ShowDialog(message, kDialogSize, 4u, callback);
        after_dialog();
    } catch (...) {
        if (callback != 0u) runtime.DestroyCallback(callback, 1u);
        throw;
    }
    if (callback != 0u) runtime.DestroyCallback(callback, 1u);
}

}  // namespace

std::uint8_t SmithWindow::OnItemUseConfirm(
    const std::uint32_t object, const std::uint32_t event,
    ISmithWindowItemUseRuntime& runtime) {
    if (event != 1u) return 1u;

    const auto prescription_id = runtime.PrescriptionHoverItemId(object);
    const auto equipment_id = runtime.EquipmentHoverItemId(object);
    runtime.RegisterSmithSystem();
    const auto result = runtime.ApplyItemEffects(
        prescription_id, equipment_id);
    if (result == 0) {
        runtime.CreateCommonDialog(runtime.UIFrameManager());
        const auto id = runtime.EquipmentHoverItemId(object);
        const auto item = runtime.FindInventoryItem(
            static_cast<std::uint32_t>(id), 7u);
        const auto name = runtime.ReadText(
            runtime.ReadWord(item + 0x9cu));
        const auto message = runtime.FormatMessage(
            runtime.DialogTemplate(0x39u), name);
        ShowWithCallback(object, message, runtime, [&] {
            const auto selected_id = runtime.EquipmentHoverItemId(object);
            const auto selected = runtime.FindInventoryItem(
                static_cast<std::uint32_t>(selected_id), 7u);
            UpdateEquipmentItemName(object, selected, runtime);
            UpdateEquipmentItemDescription(object, selected, runtime);
        });
    } else if (result == -1) {
        runtime.CreateCommonDialog(runtime.UIFrameManager());
        const std::string message(runtime.DialogTemplate(0x38u));
        ShowWithCallback(object, message, runtime, [&] {
            const auto hover = runtime.HoverItem(
                runtime.ReadWord(object + 0x40u));
            const auto selected = hover == 0u ? 0u
                : runtime.FindInventoryItem(
                      runtime.ReadWord(hover + 0x138u), 7u);
            UpdateEquipmentItemName(object, selected, runtime);
            UpdateEquipmentItemDescription(object, selected, runtime);
        });
    }
    runtime.Call49F080(object);
    runtime.Call49EF80(object);
    runtime.HideSelectionPanel(object);
    return 1u;
}

}  // namespace pal4
