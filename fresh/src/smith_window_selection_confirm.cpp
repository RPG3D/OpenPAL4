#include "pal4/smith_window.h"

#include <array>
#include <bit>
#include <cstdint>

namespace pal4 {
namespace {

const std::array<float, 2> kDialogSize{
    std::bit_cast<float>(0x3e99999au),
    std::bit_cast<float>(0x3e99999au),
};

}  // namespace

std::uint8_t SmithWindow::OnSelectionConfirm(
    const std::uint32_t object, const std::uint32_t event,
    ISmithWindowSelectionConfirmRuntime& runtime) {
    if (event != 1u) return 1u;

    const auto prescription_id = runtime.PrescriptionHoverItemId(object);
    const auto equipment_id = runtime.EquipmentHoverItemId(object);
    runtime.RegisterSmithSystem();
    const auto property = runtime.ElementProperty(prescription_id);
    runtime.RegisterSmithSystem();
    if (runtime.CheckItemQuantity(property)) {
        runtime.CreateCommonDialog(runtime.UIFrameManager());
        std::uint32_t callback = 0u;
        runtime.ShowDialog(runtime.DialogTemplate(0x73u),
                           kDialogSize, 4u, callback);
        return 1u;
    }

    runtime.RegisterSmithSystem();
    if (runtime.ProcessCrafting(prescription_id, equipment_id) != -1) {
        runtime.CreateCommonDialog(runtime.UIFrameManager());
        runtime.RegisterSmithSystem();
        const auto current_id = runtime.PrescriptionHoverItemId(object);
        const auto current_property = runtime.ElementProperty(current_id);
        const auto name = runtime.EquipmentName(current_property);
        const auto message = runtime.FormatMessage(
            runtime.DialogTemplate(0x33u), name);
        auto callback = runtime.CreateCloseCallback(object);
        try {
            runtime.ShowDialog(message, kDialogSize, 4u, callback);
        } catch (...) {
            if (callback != 0u) runtime.DestroyCallback(callback, 1u);
            throw;
        }
        if (callback != 0u) runtime.DestroyCallback(callback, 1u);
    }
    runtime.Call49F080(object);
    runtime.Call49EF80(object);
    runtime.HideSelectionPanel(object);
    return 1u;
}

}  // namespace pal4
