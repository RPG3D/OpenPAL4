#include "pal4/equipment_category_events.h"

#include <cstdint>

namespace pal4 {

bool EquipmentCategoryEvents::ClearHighlight(
    EquipmentWindowState& state,
    IEquipmentCategoryEventsRuntime& runtime) {
    for (std::uint32_t index = 0u; index < 6u; ++index) {
        const auto window = state.Read(0x98u + 4u * index);
        if (!runtime.IsDisabled(window)) {
            runtime.SetHoverFlag(window, false);
            runtime.RequestRedraw(window);
        }
    }
    return true;
}

bool EquipmentCategoryEvents::SelectFirstEnabled(
    EquipmentWindowState& state, EquipmentCategoryMemberState& member,
    IEquipmentCategoryEventsRuntime& runtime) {
    (void)ClearHighlight(state, runtime);
    for (std::uint32_t index = 0u; index < 6u; ++index) {
        const auto window = state.Read(0x98u + 4u * index);
        if (!runtime.IsDisabled(window)) {
            runtime.SetHoverFlag(window, true);
            runtime.RequestRedraw(window);
            member.selected_index = index;
            return true;
        }
    }
    runtime.RemoveCategoryMember();
    runtime.InsertPrimaryMember();
    return true;
}

bool EquipmentCategoryEvents::Handle(
    EquipmentWindowState& state, EquipmentCategoryMemberState& member,
    std::uint32_t event, IEquipmentCategoryEventsRuntime& runtime) {
    const auto code = runtime.ReadWord(event + 0x10u);
    if (code == 1u) {
        runtime.RemoveCategoryMember();
        runtime.RemovePrimaryMember();
        runtime.SetEquipmentVisible(state, false);
        return true;
    }
    if (code == 15u) {
        (void)ClearHighlight(state, runtime);
        runtime.RemoveCategoryMember();
        return true;
    }

    std::uint32_t target = member.selected_index;
    switch (code) {
        case 203u:
            if (target < 3u || target > 5u) return true;
            target -= 3u;
            break;
        case 205u:
            if (target > 2u) return true;
            target += 3u;
            break;
        case 28u:
            runtime.HandleMouseEvent(
                runtime.ControlWindow(state, member.selected_index));
            (void)SelectFirstEnabled(state, member, runtime);
            return true;
        default:
            return runtime.HandleBaseKey(member, event);
    }

    const auto destination = runtime.NavigationWindow(state, target);
    if (!runtime.IsDisabled(destination)) {
        const auto source = runtime.NavigationWindow(state,
                                                   member.selected_index);
        runtime.SetHoverFlag(source, false);
        runtime.RequestRedraw(source);
        member.selected_index = target;
        runtime.SetHoverFlag(destination, true);
        runtime.RequestRedraw(destination);
    }
    return true;
}

}  // namespace pal4
