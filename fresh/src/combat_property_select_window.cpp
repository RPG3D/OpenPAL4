#include "pal4/combat_property_select_window.h"

#include <array>
#include <string>
#include <vector>

namespace pal4 {

std::uint32_t CombatPropertySelectWindow::Construct(
    const std::uint32_t window, ICombatSelectConstructionRuntime& runtime) {
    (void)CombatMagicSelectBase::Construct(window, kLayoutName,
                                           kResourceGroup, runtime);
    runtime.WriteWord(window + 48u, 0x8435a4u);
    runtime.WriteWord(window + 52u, 0u);
    runtime.WriteWord(window + 56u, 0u);
    runtime.WriteWord(window + 60u, 0u);
    runtime.WriteWord(window + 64u, 0u);
    runtime.WriteWord(window, kVtable);
    runtime.WriteWord(window + 48u, kFacetVtable);
    (void)InitializeButtons(window, runtime);
    runtime.WriteWord(window + 80u, 0u);
    runtime.WriteWord(window + 84u, 0u);
    runtime.WriteWord(window + 88u, 0u);
    return window;
}

std::uint32_t CombatPropertySelectWindow::Setup(
    const std::uint32_t window, const std::uint32_t pair,
    const std::uint32_t actor, const std::uint32_t parent,
    ICombatSelectWindowRuntime& runtime) {
    runtime.WriteWord(window + 64u, runtime.ReadWord(pair + 4u));
    runtime.WriteWord(window + 60u, runtime.ReadWord(pair));
    runtime.WriteWord(window + 52u, actor);
    runtime.InvokeVisibility(window, 1u, parent);
    runtime.SetText(runtime.ReadWord(window + 44u), "");
    SelectButton(window, 0u, runtime);
    const auto next = FindNextEnabledButton(window, -1, runtime);
    if (next != -1)
        SelectButton(window, static_cast<std::uint32_t>(next), runtime);
    return runtime.InsertEventHandler(runtime.UiFrameManager() + 308u,
                                      window + 48u);
}

std::uint32_t CombatPropertySelectWindow::InitializeButtons(
    const std::uint32_t window, ICombatSelectConstructionRuntime& runtime) {
    const auto list = runtime.ReadWord(window + 40u);
    runtime.SetSortColumn(list, 0u);
    runtime.SetSortDirection(list, 0u);
    const auto subscribe = [&](const std::uint32_t control,
                               const std::uint32_t function,
                               const bool hover, const bool button) {
        const auto callback = runtime.Allocate(24u);
        if (callback) {
            runtime.WriteWord(callback, kEventVtable);
            runtime.WriteWord(callback + 8u, function);
            runtime.WriteWord(callback + 12u, 0u);
            runtime.WriteWord(callback + 16u, window);
        }
        const auto subscription = hover
            ? runtime.SubscribeListHover(control, callback)
            : button ? runtime.SubscribeButtonClick(control, callback)
                     : runtime.SubscribeMouseClick(control, callback);
        if (subscription) runtime.ReleaseSubscription(subscription);
    };
    subscribe(list, kHoverCallback, true, false);
    subscribe(list, kListClickCallback, false, false);

    constexpr std::array<std::string_view, 3> names{{
        "BtnCure", "BtnAuxiliary", "BtnAttackable",
    }};
    const auto prefix_address = runtime.ReadWord(window + 24u);
    const auto prefix = prefix_address
        ? std::string(runtime.ReadCString(prefix_address)) : std::string();
    for (std::uint32_t index = 0u; index < names.size(); ++index) {
        const auto button = runtime.GetWindow(window, prefix +
                                                       std::string(names[index]));
        runtime.WriteWord(window + 68u + 4u * index, button);
    }
    for (std::uint32_t index = 0u; index < names.size(); ++index) {
        const auto button = runtime.ReadWord(window + 68u + 4u * index);
        runtime.SetButtonEnabled(button, false);
        runtime.WriteWord(button + 280u, index);
        subscribe(button, kButtonClickCallback, false, true);
    }
    return runtime.CleanupTemporaryString();
}

void CombatPropertySelectWindow::UpdatePropertyList(
    const std::uint32_t window, ICombatSelectWindowRuntime& runtime) {
    if (!runtime.ReadWord(window + 52u)) return;

    const auto list = runtime.ReadWord(window + 40u);
    runtime.ResetList(list);
    const auto columns = runtime.GetColumnCount(list);
    for (std::uint32_t index = 0u; index < columns; ++index)
        runtime.RemoveColumn(list, 0u);
    runtime.AddColumn(list,
        runtime.ConvertToUtf8(runtime.GetGameString(0u)), 0u, 0.6f);
    runtime.AddColumn(list,
        runtime.ConvertToUtf8(runtime.GetGameString(1u)), 1u, 0.3f);

    const auto group = runtime.ReadWord(window + 56u);
    const auto preferred_id = runtime.ReadWord(window + 80u + 4u * group);
    std::vector<std::uint32_t> records;
    const auto count = runtime.GetInventoryCount(group);
    for (std::int32_t index = 0; index < count; ++index) {
        const auto record = runtime.GetInventoryElement(
            group, static_cast<std::uint32_t>(index));
        const auto property = runtime.ReadWord(record);
        if (runtime.ReadWord(property + 8u) >= 6u) continue;
        if (runtime.ReadWord(property) == preferred_id)
            records.insert(records.begin(), record);
        else
            records.push_back(record);
    }

    for (const auto record : records) {
        const auto property = runtime.ReadWord(record);
        const auto id = runtime.ReadWord(property);
        runtime.AddRow(list, 0u);
        const auto row = runtime.GetRowCount(list) - 1u;
        const auto name_item = runtime.Allocate(0x210u);
        const auto name_address = runtime.ReadWord(property + 20u);
        const auto name = name_address
            ? runtime.ReadCString(name_address) : std::string_view{};
        runtime.ConstructCostItem(name_item, list,
                                  runtime.ConvertToUtf8(name));
        runtime.WriteWord(name_item + 312u, id);
        runtime.SetListItem(list, name_item, row,
                            runtime.GetColumnWithId(list, 0u));

        const auto count_item = runtime.Allocate(0x210u);
        const auto quantity = static_cast<std::int32_t>(
            runtime.ReadWord(record + 8u));
        runtime.ConstructCostItem(count_item, list,
            runtime.ConvertToUtf8(std::to_string(quantity)));
        runtime.WriteWord(count_item + 312u, id);
        runtime.SetListItem(list, count_item, row,
                            runtime.GetColumnWithId(list, 1u));
    }
    if (runtime.GetRowCount(list)) runtime.SetHoverRowIndex(list, 0u);
}

std::uint32_t CombatPropertySelectWindow::GetHoveredProperty(
    const std::uint32_t window, ICombatSelectWindowRuntime& runtime) {
    const auto item = runtime.GetHoverItem(runtime.ReadWord(window + 40u));
    if (!item || runtime.ReadByte(item + 421u)) return 0u;
    return runtime.FindProperty(runtime.ReadWord(item + 312u));
}

void CombatPropertySelectWindow::SelectButton(
    const std::uint32_t window, const std::uint32_t selection,
    ICombatSelectWindowRuntime& runtime) {
    if (selection == 0u) runtime.WriteWord(window + 56u, 0u);
    if (selection == 1u) runtime.WriteWord(window + 56u, 2u);
    if (selection == 2u) runtime.WriteWord(window + 56u, 1u);
    for (std::uint32_t index = 0u; index < 3u; ++index) {
        const auto button = runtime.ReadWord(window + 68u + 4u * index);
        runtime.SetButtonSelected(button, false);
        runtime.WriteWord(button + 280u, index);
        runtime.SetButtonEnabled(button, CheckEnabled(index, runtime));
        runtime.SetButtonVisible(button, true);
    }
    runtime.SetButtonSelected(
        runtime.ReadWord(window + 68u + 4u * selection), true);
    UpdatePropertyList(window, runtime);
}

std::uint8_t CombatPropertySelectWindow::HandleButtonClick(
    const std::uint32_t window, const std::uint32_t event,
    ICombatSelectWindowRuntime& runtime) {
    const auto control = runtime.ReadWord(event + 8u);
    SelectButton(window, runtime.ReadWord(control + 280u), runtime);
    return 1u;
}

std::uint8_t CombatPropertySelectWindow::HandleListClick(
    const std::uint32_t window, ICombatSelectWindowRuntime& runtime) {
    return HandleListClickInternal(window, runtime);
}

std::uint8_t CombatPropertySelectWindow::HandleListClickInternal(
    const std::uint32_t window, ICombatSelectWindowRuntime& runtime) {
    const auto property = GetHoveredProperty(window, runtime);
    if (property) {
        const auto group = runtime.ReadWord(window + 56u);
        runtime.WriteWord(window + 80u + 4u * group,
                          runtime.ReadWord(property));
        (void)runtime.RemoveEventHandler(runtime.UiFrameManager() + 308u,
                                         window + 48u);
        runtime.InvokeSelectionCallback(runtime.ReadWord(window + 64u),
                                        runtime.ReadWord(window + 60u),
                                        property);
        runtime.InvokeVisibility(window, 0u, 0u);
    }
    return 1u;
}

std::uint8_t CombatPropertySelectWindow::UpdatePropertyDescription(
    const std::uint32_t window, ICombatSelectWindowRuntime& runtime) {
    const auto property = GetHoveredProperty(window, runtime);
    if (property) {
        const auto address = runtime.ReadWord(property + 52u);
        const auto text = address
            ? runtime.ReadCString(address) : std::string_view{};
        runtime.SetText(runtime.ReadWord(window + 44u),
                        runtime.ConvertToUtf8(text));
    }
    return 1u;
}

bool CombatPropertySelectWindow::CheckEnabled(
    const std::uint32_t selection, ICombatSelectWindowRuntime& runtime) {
    if (selection > 2u) return false;
    const auto group = selection == 1u ? 2u : selection == 2u ? 1u : 0u;
    const auto count = runtime.GetInventoryCount(group);
    for (std::int32_t index = 0; index < count; ++index) {
        const auto record = runtime.GetInventoryElement(
            group, static_cast<std::uint32_t>(index));
        const auto property = runtime.ReadWord(record);
        if (runtime.ReadWord(property + 8u) < 6u) return true;
    }
    return false;
}

std::int32_t CombatPropertySelectWindow::FindNextEnabledButton(
    const std::uint32_t window, const std::int32_t current,
    ICombatSelectWindowRuntime& runtime) {
    for (auto index = current + 1; index < 3; ++index)
        if (!runtime.IsButtonDisabled(runtime.ReadWord(
                window + 68u + 4u * static_cast<std::uint32_t>(index))))
            return index;
    return current;
}

std::uint8_t CombatPropertySelectWindow::HandleKeyDown(std::uint32_t window,std::uint32_t event,
    ICombatMagicSelectWindowRuntime& runtime,ICombatSelectListNavigationRuntime& navigation){
    const auto group=runtime.ReadWord(window+56u);
    const auto current=group==0u?0:group==1u?2:group==2u?1:3;
    const auto key=runtime.ReadWord(event+16u);
    switch(key){
    case 1u:runtime.InvokeVirtualSelection(window,24u);return 1u;
    case 203u:case 205u:{
        const auto next=key==203u?CombatMagicSelectWindow::FindPreviousEnabledButton(window,current,runtime):FindNextEnabledButton(window,current,runtime);
        if(next!=current)SelectButton(window,static_cast<std::uint32_t>(next),runtime);return 1u;
    }
    case 208u:navigation.DownHoverItem(runtime.ReadWord(window+40u));return 1u;
    case 200u:navigation.UpHoverItem(runtime.ReadWord(window+40u));return 1u;
    case 28u:(void)HandleListClickInternal(window,runtime);return 1u;
    default:return 0u;
    }
}
}  // namespace pal4
