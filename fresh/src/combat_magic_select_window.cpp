#include "pal4/combat_magic_select_window.h"

#include <array>
#include <string>

namespace pal4 {

std::uint32_t CombatSelectEventRecord::ConstructMagic(
    const std::uint32_t record, const std::uint32_t function,
    const std::uint32_t context, const std::uint32_t owner,
    ICombatSelectWindowRuntime& runtime) {
    runtime.WriteWord(record + 8u, function);
    runtime.WriteWord(record + 12u, context);
    runtime.WriteWord(record + 16u, owner);
    runtime.WriteWord(record, 0x845610u);
    return record;
}

std::uint32_t CombatSelectEventRecord::ConstructProperty(
    const std::uint32_t record, const std::uint32_t function,
    const std::uint32_t context, const std::uint32_t owner,
    ICombatSelectWindowRuntime& runtime) {
    runtime.WriteWord(record + 8u, function);
    runtime.WriteWord(record + 12u, context);
    runtime.WriteWord(record + 16u, owner);
    runtime.WriteWord(record, 0x84564cu);
    return record;
}

std::uint32_t CombatMagicSelectBase::Construct(
    const std::uint32_t window, const std::uint32_t layout,
    const std::uint32_t group, ICombatSelectConstructionRuntime& runtime) {
    runtime.LoadLayout(window, layout, group, 0u);
    runtime.WriteWord(window, kVtable);
    (void)Initialize(window, runtime);
    return window;
}

std::uint32_t CombatMagicSelectBase::Initialize(
    const std::uint32_t window, ICombatSelectConstructionRuntime& runtime) {
    const auto prefix_address = runtime.ReadWord(window + 24u);
    const auto prefix = prefix_address
        ? std::string(runtime.ReadCString(prefix_address)) : std::string();
    const auto list = runtime.GetWindowByName(window, prefix + "List");
    runtime.WriteWord(window + 40u, list);
    runtime.WriteWord(window + 44u,
                      runtime.GetWindowByName(window,
                                              prefix + "EditDescription"));
    const auto root = runtime.GetWindow(window, prefix + "Root");
    const auto callback = runtime.Allocate(12u);
    if (callback) {
        runtime.WriteWord(callback, kClickCallbackVtable);
        runtime.WriteWord(callback + 4u, kClickCallback);
        runtime.WriteWord(callback + 8u, window);
    }
    const auto subscription = runtime.SubscribeMouseClick(root, callback);
    if (subscription) runtime.ReleaseSubscription(subscription);
    runtime.SetSortColumn(list, 0u);
    runtime.SetSortDirection(list, 0u);
    runtime.SetUserColumnSizingEnabled(list, false);
    runtime.SetUserColumnDraggingEnabled(list, false);
    runtime.SetUserSortControlEnabled(list, false);
    return runtime.CleanupTemporaryString();
}

std::uint8_t CombatMagicSelectBase::HandleRootClick(
    const std::uint32_t window, const std::uint32_t event,
    ICombatMagicSelectWindowRuntime& runtime) {
    if (runtime.ReadWord(event + 28u) == 1u)
        runtime.InvokeVirtualSelection(window, 24u);
    return 1u;
}

std::uint32_t CombatMagicSelectWindow::Construct(
    const std::uint32_t window, ICombatSelectConstructionRuntime& runtime) {
    (void)CombatMagicSelectBase::Construct(window, kLayoutName,
                                           kResourceGroup, runtime);
    runtime.WriteWord(window + 48u, 0x8435a4u);
    runtime.WriteWord(window + 52u, 0u);
    runtime.WriteWord(window + 56u, 5u);
    runtime.WriteWord(window + 60u, 0u);
    runtime.WriteWord(window + 64u, 0u);
    runtime.WriteWord(window, kVtable);
    runtime.WriteWord(window + 48u, kFacetVtable);
    (void)InitializeButtons(window, runtime);
    return window;
}

std::uint32_t CombatMagicSelectWindow::InitializeButtons(
    const std::uint32_t window, ICombatSelectConstructionRuntime& runtime) {
    const auto list = runtime.ReadWord(window + 40u);
    runtime.SetSortColumn(list, 0u);
    runtime.SetSortDirection(list, 0u);
    const auto subscribe = [&](const std::uint32_t control,
                               const std::uint32_t function,
                               const bool hover,
                               const bool button) {
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

    constexpr std::array<std::string_view, 5> names{{
        "BtnWater", "BtnFire", "BtnThunder", "BtnAir", "BtnEarth",
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
        runtime.WriteWord(button + 280u, index);
        subscribe(button, kButtonClickCallback, false, true);
    }
    return runtime.CleanupTemporaryString();
}

std::uint32_t CombatMagicSelectWindow::Setup(
    const std::uint32_t window, const std::uint32_t pair,
    const std::uint32_t mode, const std::uint32_t parent,
    ICombatMagicSelectWindowRuntime& runtime) {
    runtime.WriteWord(window + 64u, runtime.ReadWord(pair + 4u));
    runtime.WriteWord(window + 60u, runtime.ReadWord(pair));
    runtime.WriteWord(window + 52u, mode);
    runtime.InvokeVisibility(window, 1u, parent);
    runtime.SetText(runtime.ReadWord(window + 44u), "");
    SelectButton(window, 0u, runtime);
    const auto next = FindNextEnabledButton(window, -1, runtime);
    if (next != -1)
        SelectButton(window, static_cast<std::uint32_t>(next), runtime);
    return runtime.InsertEventHandler(runtime.UiFrameManager() + 308u,
                                      window + 48u);
}

void CombatMagicSelectWindow::UpdateMagicList(
    const std::uint32_t window, ICombatMagicSelectWindowRuntime& runtime) {
    const auto actor = runtime.ReadWord(window + 52u);
    const auto element = runtime.ReadWord(window + 56u);
    if (!actor || element > 4u) return;

    const auto list = runtime.ReadWord(window + 40u);
    runtime.ResetList(list);
    runtime.ResetList(list);
    const auto columns = runtime.GetColumnCount(list);
    for (std::uint32_t index = 0; index < columns; ++index)
        runtime.RemoveColumn(list, 0u);

    runtime.AddColumn(list,
        runtime.ConvertToUtf8(runtime.GetGameString(0u)), 0u, 0.6f);
    runtime.AddColumn(list,
        runtime.ConvertToUtf8(runtime.GetGameString(0x134u)), 1u, 0.3f);

    const auto iterator = runtime.GetSkillIterator(actor + 920u, element);
    while (!runtime.IteratorDone(iterator)) {
        const auto magic = runtime.FindMagic(runtime.IteratorSkillId(iterator));
        if (magic) {
            const auto type = runtime.ReadWord(magic + 8u);
            if (type != 3u && type != 6u) {
                runtime.AddRow(list, 0u);
                const auto name_item = runtime.Allocate(0x214u);
                const auto name_address = runtime.ReadWord(magic + 16u);
                const auto name = name_address
                    ? runtime.ReadCString(name_address) : std::string_view{};
                runtime.ConstructNameItem(name_item, list,
                                          runtime.ConvertToUtf8(name), true);
                const auto icon = type == 0u ? "skillattack"
                    : type == 2u ? "skillcure" : "skillbuff";
                runtime.WriteWord(name_item + 528u,
                                  runtime.GetImage("gy0", icon));
                runtime.WriteWord(name_item + 312u,
                                  runtime.GetMagicKey(magic));
                const auto row = runtime.GetRowCount(list) - 1u;
                runtime.SetListItem(list, name_item, row,
                                    runtime.GetColumnWithId(list, 0u));

                const auto cost_item = runtime.Allocate(0x210u);
                const auto cost = static_cast<std::int32_t>(
                    runtime.ReadWord(magic + 76u));
                runtime.ConstructCostItem(cost_item, list,
                    runtime.ConvertToUtf8(std::to_string(cost)));
                runtime.WriteWord(cost_item + 312u,
                                  runtime.GetMagicKey(magic));
                runtime.SetListItem(list, cost_item,
                    runtime.GetRowCount(list) - 1u,
                    runtime.GetColumnWithId(list, 1u));
                if (cost > runtime.GetActorStat(actor, 9u)) {
                    runtime.WriteByte(name_item + 421u, 1u);
                    runtime.WriteByte(cost_item + 421u, 1u);
                }
            }
        }
        runtime.AdvanceIterator(iterator);
    }
    if (runtime.GetRowCount(list)) runtime.SetHoverRowIndex(list, 0u);
    if (iterator) runtime.ReleaseIterator(iterator);
}

std::uint32_t CombatMagicSelectWindow::GetHoveredMagic(
    const std::uint32_t window, ICombatMagicSelectWindowRuntime& runtime) {
    const auto item = runtime.GetHoverItem(runtime.ReadWord(window + 40u));
    if (!item || runtime.ReadByte(item + 421u)) return 0u;
    return runtime.FindMagic(runtime.ReadWord(item + 312u));
}

void CombatMagicSelectWindow::SelectButton(
    const std::uint32_t window, const std::uint32_t selection,
    ICombatMagicSelectWindowRuntime& runtime) {
    runtime.WriteWord(window + 56u, selection);
    const auto actor = runtime.ReadWord(window + 52u);
    for (std::uint32_t index = 0u; index < 5u; ++index) {
        const auto button = runtime.ReadWord(window + 68u + 4u * index);
        runtime.SetButtonSelected(button, false);
        const auto iterator = runtime.GetSkillIterator(actor + 920u, index);
        runtime.SetButtonEnabled(button, runtime.IteratorSize(iterator) > 0);
        runtime.SetButtonVisible(button, true);
        if (iterator) runtime.ReleaseIterator(iterator);
    }
    runtime.SetButtonSelected(
        runtime.ReadWord(window + 68u + 4u * selection), true);
    UpdateMagicList(window, runtime);
}

std::uint8_t CombatMagicSelectWindow::HandleButtonClick(
    const std::uint32_t window, const std::uint32_t event,
    ICombatMagicSelectWindowRuntime& runtime) {
    const auto control = runtime.ReadWord(event + 8u);
    SelectButton(window, runtime.ReadWord(control + 280u), runtime);
    return 1u;
}

std::uint8_t CombatMagicSelectWindow::HandleListClick(
    const std::uint32_t window, ICombatMagicSelectWindowRuntime& runtime) {
    return HandleListClickInternal(window, runtime);
}

std::uint8_t CombatMagicSelectWindow::HandleListClickInternal(
    const std::uint32_t window, ICombatMagicSelectWindowRuntime& runtime) {
    const auto magic = GetHoveredMagic(window, runtime);
    if (magic) {
        (void)runtime.RemoveEventHandler(runtime.UiFrameManager() + 308u,
                                         window ? window + 48u : 0u);
        runtime.InvokeSelectionCallback(runtime.ReadWord(window + 64u),
                                        runtime.ReadWord(window + 60u),
                                        magic);
        runtime.InvokeVisibility(window, 0u, 0u);
    }
    return 1u;
}

std::int32_t CombatMagicSelectWindow::FindNextEnabledButton(
    const std::uint32_t window, const std::int32_t current,
    ICombatMagicSelectWindowRuntime& runtime) {
    for (auto index = current + 1; index < 5; ++index)
        if (!runtime.IsButtonDisabled(runtime.ReadWord(
                window + 68u + 4u * static_cast<std::uint32_t>(index))))
            return index;
    return current;
}

std::int32_t CombatMagicSelectWindow::FindPreviousEnabledButton(
    const std::uint32_t window, const std::int32_t current,
    ICombatMagicSelectWindowRuntime& runtime) {
    for (auto index = current - 1; index >= 0; --index)
        if (!runtime.IsButtonDisabled(runtime.ReadWord(
                window + 68u + 4u * static_cast<std::uint32_t>(index))))
            return index;
    return current;
}

std::uint8_t CombatMagicSelectWindow::UpdateMagicDescription(
    const std::uint32_t window, ICombatMagicSelectWindowRuntime& runtime) {
    const auto item = runtime.GetHoverItem(runtime.ReadWord(window + 40u));
    if (item) {
        const auto magic = runtime.FindMagic(runtime.ReadWord(item + 312u));
        if (magic) {
            const auto address = runtime.ReadWord(magic + 48u);
            const auto text = address
                ? runtime.ReadCString(address) : std::string_view{};
            runtime.SetText(runtime.ReadWord(window + 44u),
                            runtime.ConvertToUtf8(text));
        }
    }
    return 1u;
}

std::uint32_t CombatMagicSelectWindow::Cleanup(
    const std::uint32_t window, ICombatMagicSelectWindowRuntime& runtime) {
    runtime.InvokeSelectionCallback(runtime.ReadWord(window + 64u),
                                    runtime.ReadWord(window + 60u), 0u);
    return runtime.RemoveEventHandler(runtime.UiFrameManager() + 308u,
                                      window ? window + 48u : 0u);
}

std::uint8_t CombatMagicSelectWindow::HandleKeyDown(
    const std::uint32_t window, const std::uint32_t event,
    ICombatMagicSelectWindowRuntime& runtime,
    ICombatSelectListNavigationRuntime& navigation) {
    switch (runtime.ReadWord(event + 16u)) {
    case 0x01u:
        runtime.InvokeVirtualSelection(window, 24u);
        return 1u;
    case 0xcbu:
    case 0xcdu: {
        const auto current = static_cast<std::int32_t>(
            runtime.ReadWord(window + 56u));
        const auto next = runtime.ReadWord(event + 16u) == 0xcbu
            ? FindPreviousEnabledButton(window, current, runtime)
            : FindNextEnabledButton(window, current, runtime);
        if (next != current)
            SelectButton(window, static_cast<std::uint32_t>(next), runtime);
        return 1u;
    }
    case 0xd0u:
        navigation.DownHoverItem(runtime.ReadWord(window + 40u));
        return 1u;
    case 0xc8u:
        navigation.UpHoverItem(runtime.ReadWord(window + 40u));
        return 1u;
    case 0x1cu:
        (void)HandleListClickInternal(window, runtime);
        return 1u;
    default:
        return 0u;
    }
}

}  // namespace pal4
