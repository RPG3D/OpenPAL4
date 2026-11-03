#include "pal4/system_dialog_actions.h"
#include "pal4/system_dialog_visibility.h"

namespace pal4 {
namespace {

template <typename Hide>
std::uint8_t HandleKey(const std::uint32_t frame,
                       const std::uint32_t event,
                       ISystemDialogActionsRuntime& runtime,
                       Hide hide) {
    const auto key = runtime.ReadWord(event + 16u);
    if (key == 1u || key == 28u) {
        hide();
        return 1u;
    }
    if (key != 200u && key != 208u) {
        return 0u;
    }
    const auto text = runtime.ReadWord(frame + 40u);
    const auto scrollbar = runtime.ReadWord(text + 1256u);
    const auto position = runtime.ReadFloat(scrollbar + 1216u);
    runtime.SetScrollPosition(scrollbar,
                              position + (key == 200u ? -10.0f : 10.0f));
    return 1u;
}

}  // namespace

std::uint8_t SystemDialogActions::OnOkClick(
    const std::uint32_t frame, const std::uint32_t event,
    ISystemDialogActionsRuntime& runtime) {
    (void)event;
    runtime.UpdateVisibility(frame, 0u, 0u, 0u);
    return 1u;
}

std::uint8_t SystemDialogActions::OnOkClick(
    const std::uint32_t frame, const std::uint32_t event,
    ISystemDialogVisibilityRuntime& visibility) {
    (void)event;
    SystemDialogVisibility::Update(frame, 0u, "", 0u, visibility);
    return 1u;
}

std::uint32_t SystemDialogActions::VirtualCall(
    const std::uint32_t object, ISystemDialogActionsRuntime& runtime) {
    const auto vtable = runtime.ReadWord(object);
    return runtime.CallVirtual(object, runtime.ReadWord(vtable + 24u));
}

std::uint8_t SystemDialogActions::HandleKeyInput(
    const std::uint32_t frame, const std::uint32_t event,
    ISystemDialogActionsRuntime& runtime) {
    return HandleKey(frame, event, runtime, [&]() {
        runtime.UpdateVisibility(frame, 0u, 0u, 0u);
    });
}

std::uint8_t SystemDialogActions::HandleKeyInput(
    const std::uint32_t frame, const std::uint32_t event,
    ISystemDialogActionsRuntime& runtime,
    ISystemDialogVisibilityRuntime& visibility) {
    return HandleKey(frame, event, runtime, [&]() {
        SystemDialogVisibility::Update(frame, 0u, "", 0u, visibility);
    });
}

}  // namespace pal4
