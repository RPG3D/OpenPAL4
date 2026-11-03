#include "pal4/equipment_list_events.h"

#include <cstdint>
#include <initializer_list>

namespace pal4 {
namespace {

void ResetNavigationButtons(EquipmentWindowState& state,
                            IEquipmentListEventsRuntime& runtime) {
    for (const auto offset : {0x48u, 0x4cu}) {
        const auto window = state.Read(offset);
        runtime.SetHoverFlag(window, false);
        runtime.RequestRedraw(window);
    }
}

}  // namespace

void EquipmentListEvents::SetHighlight(
    EquipmentWindowState& state, EquipmentListMemberState& member,
    bool highlighted, IEquipmentListEventsRuntime& runtime) {
    const auto left = state.Read(0x48u);
    const auto right = state.Read(0x4cu);
    member.equipment_highlighted = highlighted;
    if (!runtime.IsVisible(left)) member.equipment_highlighted = false;
    if (highlighted) {
        if (runtime.IsVisible(right)) {
            runtime.SetHoverFlag(right, true);
            runtime.RequestRedraw(right);
        }
    } else {
        runtime.SetHoverFlag(right, false);
        runtime.RequestRedraw(right);
        runtime.SetHoverFlag(left, false);
        runtime.RequestRedraw(left);
    }
}

bool EquipmentListEvents::ResetHighlight(
    EquipmentWindowState& state, EquipmentListMemberState& member,
    IEquipmentListEventsRuntime& runtime) {
    SetHighlight(state, member, false, runtime);
    return true;
}

bool EquipmentListEvents::Handle(
    EquipmentWindowState& state, EquipmentListMemberState& member,
    std::uint32_t event, IEquipmentListEventsRuntime& runtime) {
    const auto code = runtime.ReadWord(event + 0x10u);
    if (code == 1u) {
        runtime.RemovePrimaryMember();
        runtime.SetEquipmentVisible(state, false);
        return true;
    }
    if (code == 15u) {
        bool has_enabled_control = false;
        for (std::uint32_t index = 0u; index < 6u; ++index) {
            if (!runtime.IsDisabled(state.Read(0x98u + index * 4u))) {
                has_enabled_control = true;
                break;
            }
        }
        if (has_enabled_control) {
            runtime.InsertSecondaryMember();
            ResetNavigationButtons(state, runtime);
        }
        return true;
    }

    const auto list = state.Read(0x40u);
    if (runtime.RowCount(list) == 0u) return false;
    switch (code) {
        case 200u:
            if (member.equipment_highlighted ||
                runtime.HoveredItem(list) == 0u) return true;
            if (runtime.HoverRowIndex(list) != 0u)
                runtime.UpHoverItem(list);
            else
                SetHighlight(state, member, true, runtime);
            return true;
        case 208u: {
            if (member.equipment_highlighted) {
                if (runtime.RowCount(list) != 0u) {
                    SetHighlight(state, member, false, runtime);
                    runtime.SetHoverRowIndex(list, 0u);
                }
                return true;
            }
            if (runtime.HoveredItem(list) == 0u) return true;
            const auto row = runtime.HoverRowIndex(list);
            const auto count = runtime.RowCount(list);
            if (row < count - 1u)
                runtime.DownHoverItem(list);
            return true;
        }
        case 203u:
        case 205u:
            if (member.equipment_highlighted) {
                const auto reset = state.Read(code == 203u ? 0x4cu : 0x48u);
                const auto target = state.Read(code == 203u ? 0x48u : 0x4cu);
                runtime.SetHoverFlag(reset, false);
                runtime.RequestRedraw(reset);
                runtime.HandleMouseEvent(target);
                member.equipment_highlighted = true;
            }
            return true;
        case 28u:
            if (!member.equipment_highlighted)
                runtime.HandleItemClick(state, 400.0f, 100.0f);
            return true;
        default:
            return false;
    }
}

}  // namespace pal4
