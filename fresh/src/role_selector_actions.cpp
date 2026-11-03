#include "pal4/role_selector_actions.h"

#include <array>
#include <cstdint>

namespace pal4 {
std::uint8_t RoleSelectorActions::HandleNavigationKey(std::uint32_t navigation,std::uint32_t event,
    IRoleSelectorNavigationRuntime& runtime){
    if(runtime.ReadWord(event+16u)==1u){runtime.CloseRoleSelector(navigation-40u);return 1u;}
    return runtime.NavigateRoleSelector(navigation,event);
}

std::uint8_t RoleSelectorActions::HandleRoleClick(
    const std::uint32_t object, const std::uint32_t event,
    IRoleSelectorActionRuntime& runtime) {
    const auto clicked_window = runtime.ReadWord(event + 8u);
    const auto index = static_cast<std::int32_t>(
        runtime.ReadWord(clicked_window + 280u));
    if (index < 4) {
        const auto slot = runtime.ReadWord(object + 64u);
        if (slot != 0u) {
            const auto child_value = runtime.ReadWord(
                object + 120u +
                static_cast<std::uint32_t>(index) * 52u);
            const auto context = runtime.ReadWord(object + 68u);
            runtime.InvokeChoice(runtime.ReadWord(slot),
                                 {child_value, context});
        }
        runtime.LayoutButtons(object);
    }
    return 1u;
}

std::uint8_t RoleSelectorActions::HandleCloseClick(
    const std::uint32_t object,
    IRoleSelectorActionRuntime& runtime) {
    runtime.Close(object, 0u, 0u);
    return 1u;
}

std::uint8_t RoleSelectorActions::HandleKeyEvent(
    const std::uint32_t object, const std::uint32_t event,
    IRoleSelectorActionRuntime& runtime) {
    const auto navigation = object + 40u;
    runtime.ForwardKeyEvent(navigation, event);
    runtime.RefreshNavigation(navigation);
    return 1u;
}

}  // namespace pal4
