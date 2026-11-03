#include "smith_window_trade_dialog.h"

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

void ShowSmithTradeDialog(
    ISmithWindowDialogRuntime& runtime, const std::uint32_t object,
    const std::uint32_t template_id, const std::uint32_t style,
    const std::optional<std::string_view> name,
    const std::optional<SmithWindowTradeCallback> callback_kind) {
    runtime.CreateCommonDialog();
    std::uint32_t callback = callback_kind
                                 ? runtime.CreateCallback(object, *callback_kind)
                                 : 0u;
    try {
        const std::string format(runtime.DialogTemplate(template_id));
        const std::string message = name
                                        ? runtime.FormatMessage(format, *name)
                                        : format;
        runtime.ShowDialog(message, kDialogSize, style, callback);
    } catch (...) {
        if (callback != 0u) runtime.DestroyCallback(callback, 1u);
        throw;
    }
    if (callback != 0u) runtime.DestroyCallback(callback, 1u);
}

}  // namespace pal4
