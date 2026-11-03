#include "pal4/combat_stunt_select_window.h"

#include <string>

namespace pal4 {

std::uint32_t CombatStuntSelectWindow::Construct(
    const std::uint32_t window, ICombatSelectConstructionRuntime& runtime) {
    runtime.LoadLayout(window, kLayoutName, kResourceGroup, 0u);
    runtime.WriteWord(window + 40u, 0x8435a4u);
    runtime.WriteWord(window + 44u, 0u);
    runtime.WriteWord(window + 48u, 0u);
    runtime.WriteWord(window + 52u, 0u);
    runtime.WriteWord(window, kVtable);
    runtime.WriteWord(window + 40u, kFacetVtable);
    (void)InitializeButtons(window, runtime);
    return window;
}

std::uint32_t CombatStuntSelectWindow::Setup(
    const std::uint32_t window, const std::uint32_t pair,
    const std::uint32_t actor, const std::uint32_t parent,
    ICombatStuntSelectWindowRuntime& runtime) {
    runtime.WriteWord(window + 52u, runtime.ReadWord(pair + 4u));
    runtime.WriteWord(window + 48u, runtime.ReadWord(pair));
    runtime.WriteWord(window + 44u, actor);
    runtime.InvokeVisibility(window, 1u, parent);
    runtime.SetText(runtime.ReadWord(window + 60u), "");
    UpdateStuntList(window, runtime);
    return runtime.InsertEventHandler(runtime.UiFrameManager() + 308u,
                                      window + 40u);
}

std::uint32_t CombatStuntSelectWindow::InitializeButtons(
    const std::uint32_t window, ICombatSelectConstructionRuntime& runtime) {
    const auto prefix_address = runtime.ReadWord(window + 24u);
    const auto prefix = prefix_address
        ? std::string(runtime.ReadCString(prefix_address)) : std::string();
    const auto list = runtime.GetWindowByName(window, prefix + "List");
    runtime.WriteWord(window + 56u, list);
    runtime.WriteWord(window + 60u,
                      runtime.GetWindowByName(window,
                                              prefix + "EditDescription"));
    runtime.SetSortColumn(list, 0u);
    runtime.SetSortDirection(list, 0u);
    runtime.SetUserColumnSizingEnabled(list, false);
    runtime.SetUserColumnDraggingEnabled(list, false);
    runtime.SetUserSortControlEnabled(list, false);

    const auto subscribe = [&](const std::uint32_t control,
                               const std::uint32_t function,
                               const bool hover) {
        const auto callback = runtime.Allocate(24u);
        if (callback) {
            runtime.WriteWord(callback, kEventVtable);
            runtime.WriteWord(callback + 8u, function);
            runtime.WriteWord(callback + 12u, 0u);
            runtime.WriteWord(callback + 16u, window);
        }
        const auto subscription = hover
            ? runtime.SubscribeListHover(control, callback)
            : runtime.SubscribeMouseClick(control, callback);
        if (subscription) runtime.ReleaseSubscription(subscription);
    };
    subscribe(list, kHoverCallback, true);
    subscribe(list, kListClickCallback, false);
    subscribe(runtime.GetWindow(window, prefix + "Root"),
              kRootClickCallback, false);
    return runtime.CleanupTemporaryString();
}

void CombatStuntSelectWindow::UpdateStuntList(
    const std::uint32_t window, ICombatStuntSelectWindowRuntime& runtime) {
    const auto actor = runtime.ReadWord(window + 44u);
    if (!actor) return;
    const auto list = runtime.ReadWord(window + 56u);
    runtime.ResetList(list);
    const auto columns = runtime.GetColumnCount(list);
    for (std::uint32_t index = 0u; index < columns; ++index)
        runtime.RemoveColumn(list, 0u);
    runtime.AddColumn(list,
        runtime.ConvertToUtf8(runtime.GetGameString(0u)), 0u, 0.6f);
    runtime.AddColumn(list,
        runtime.ConvertToUtf8(runtime.GetGameString(0x2au)), 1u, 0.3f);

    const auto iterator = runtime.GetStuntIterator(actor + 920u);
    while (!runtime.IteratorDone(iterator)) {
        const auto id = runtime.IteratorSkillId(iterator);
        const auto stunt = runtime.FindStunt(id);
        if (stunt) {
            const auto type = runtime.ReadWord(stunt + 4u);
            if (type != 3u && type != 6u) {
                runtime.AddRow(list, 0u);
                const auto row = runtime.GetRowCount(list) - 1u;
                const auto name_item = runtime.Allocate(0x214u);
                const auto name_address = runtime.ReadWord(stunt + 12u);
                runtime.ConstructNameItem(name_item, list,
                    runtime.ConvertToUtf8(name_address
                        ? runtime.ReadCString(name_address) : std::string_view{}),
                    true);
                const auto icon = type == 0u ? "skillattack"
                    : type == 2u ? "skillcure" : "skillbuff";
                runtime.WriteWord(name_item + 528u,
                                  runtime.GetImage("gy0", icon));
                runtime.WriteWord(name_item + 312u, id);
                runtime.SetListItem(list, name_item, row,
                                    runtime.GetColumnWithId(list, 0u));

                const auto cost_item = runtime.Allocate(0x210u);
                const auto cost = static_cast<std::int32_t>(
                    runtime.ReadWord(stunt + 116u));
                runtime.ConstructCostItem(cost_item, list,
                    runtime.ConvertToUtf8(std::to_string(cost)));
                runtime.WriteWord(cost_item + 312u, id);
                runtime.SetListItem(list, cost_item, row,
                                    runtime.GetColumnWithId(list, 1u));
                // IDA 554C3E: after correcting the public CEGUI call stack
                // deltas, actor virtual436 receives the Stunt DB record.
                if (!runtime.IsStuntUsable(actor, stunt)) {
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

std::uint32_t CombatStuntSelectWindow::GetHoveredStunt(
    const std::uint32_t window, ICombatStuntSelectWindowRuntime& runtime) {
    const auto item = runtime.GetHoverItem(runtime.ReadWord(window + 56u));
    if (!item || runtime.ReadByte(item + 421u)) return 0u;
    return runtime.FindStunt(runtime.ReadWord(item + 312u));
}

std::uint8_t CombatStuntSelectWindow::HandleListClick(
    const std::uint32_t window, ICombatStuntSelectWindowRuntime& runtime) {
    return HandleListClickInternal(window, runtime);
}

std::uint8_t CombatStuntSelectWindow::HandleListClickInternal(
    const std::uint32_t window, ICombatStuntSelectWindowRuntime& runtime) {
    const auto stunt = GetHoveredStunt(window, runtime);
    if (stunt) {
        (void)runtime.RemoveEventHandler(runtime.UiFrameManager() + 308u,
                                         window ? window + 40u : 0u);
        runtime.InvokeSelectionCallback(runtime.ReadWord(window + 52u),
                                        runtime.ReadWord(window + 48u), stunt);
        runtime.InvokeVisibility(window, 0u, 0u);
    }
    return 1u;
}

std::uint8_t CombatStuntSelectWindow::UpdateStuntDescription(
    const std::uint32_t window, ICombatStuntSelectWindowRuntime& runtime) {
    const auto item = runtime.GetHoverItem(runtime.ReadWord(window + 56u));
    if (!item) return 0u;
    const auto stunt = runtime.FindStunt(runtime.ReadWord(item + 312u));
    if (stunt) {
        const auto address = runtime.ReadWord(stunt + 44u);
        runtime.SetText(runtime.ReadWord(window + 60u),
            runtime.ConvertToUtf8(address
                ? runtime.ReadCString(address) : std::string_view{}));
    }
    return 1u;
}

std::uint32_t CombatStuntSelectWindow::Cleanup(
    const std::uint32_t window, ICombatStuntSelectWindowRuntime& runtime) {
    runtime.InvokeSelectionCallback(runtime.ReadWord(window + 52u),
                                    runtime.ReadWord(window + 48u), 0u);
    return runtime.RemoveEventHandler(runtime.UiFrameManager() + 308u,
                                      window ? window + 40u : 0u);
}

std::uint8_t CombatStuntSelectWindow::HandleRootClick(
    const std::uint32_t window, const std::uint32_t event,
    ICombatStuntSelectWindowRuntime& runtime) {
    if (runtime.ReadWord(event + 28u) == 1u)
        (void)Cleanup(window, runtime);
    return 1u;
}

std::uint8_t CombatStuntSelectWindow::HandleKeyDown(
    const std::uint32_t window, const std::uint32_t event,
    ICombatStuntSelectWindowRuntime& runtime,
    ICombatSelectListNavigationRuntime& navigation) {
    switch (runtime.ReadWord(event + 16u)) {
    case 0x01u: (void)Cleanup(window, runtime); return 1u;
    case 0xd0u:
        navigation.DownHoverItem(runtime.ReadWord(window + 56u)); return 1u;
    case 0xc8u:
        navigation.UpHoverItem(runtime.ReadWord(window + 56u)); return 1u;
    case 0x1cu: (void)HandleListClickInternal(window, runtime); return 1u;
    default: return 0u;
    }
}

}  // namespace pal4
