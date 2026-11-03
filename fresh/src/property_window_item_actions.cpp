#include "pal4/property_window_item_actions.h"

namespace pal4 {

bool PropertyWindowItemActions::HandleHover(
    PropertyWindowState& state,
    IPropertyWindowItemActionsRuntime& runtime) {
    runtime.UpdateDetails(state, runtime.HoveredItem(state));
    return true;
}

bool PropertyWindowItemActions::ProcessClick(
    PropertyWindowState& state, std::uint32_t event,
    IPropertyWindowItemActionsRuntime& runtime) {
    const auto item = runtime.ReadWord(event + 4u);
    if (item == 0u) return true;

    const auto definition = runtime.ReadWord(item);
    const auto id = runtime.ReadWord(definition);
    if (id == 3067u) {
        runtime.SetProperty(state.Read(108u), runtime.ReadWord(item + 4u));
        runtime.ExecuteEarthballSkip(state);
    } else if (id == 3182u) {
        runtime.HandleMagicItem(state);
    } else {
        const auto callback = runtime.Allocate(0x18u);
        if (callback != 0u) {
            runtime.WriteWord(callback + 8u, kUseCallback);
            runtime.WriteWord(callback + 12u, 0u);
            runtime.WriteWord(callback + 16u,
                              runtime.ObjectAddress(state));
            runtime.WriteWord(callback, kCallbackVtable);
        }
        try {
            runtime.ShowOrHideDialog(1u, callback, item);
        } catch (...) {
            if (callback != 0u) runtime.ReleaseCallback(callback, 1u);
            throw;
        }
        if (callback != 0u) runtime.ReleaseCallback(callback, 1u);
    }
    return true;
}

bool PropertyWindowItemActions::UseItem(
    PropertyWindowState& state, std::uint32_t choice,
    IPropertyWindowItemActionsRuntime& runtime) {
    runtime.PlayMusic("WD001.WAV", 0u);
    const auto item_pointer = runtime.ReadWord(choice + 4u);
    const auto item = runtime.ReadWord(item_pointer);
    const auto target_type = runtime.ItemTargetType(item);
    const auto caster = runtime.PlayerByIndex(runtime.ReadWord(choice));
    if (caster == 0u) return true;

    const auto effect = runtime.ReadWord(item + 0x94u);
    if (target_type == 3u) {
        for (std::uint32_t index = 0u; index < 4u; ++index) {
            const auto target = runtime.PlayerByIndex(index);
            if (target != 0u &&
                runtime.ReadWord(target + 0xb08u) != 0u) {
                runtime.ApplyItemEffect(caster + 0x654u, effect,
                                        target + 0x654u);
            }
        }
    } else {
        runtime.ApplyItemEffect(caster + 0x654u, effect,
                                caster + 0x654u);
    }

    const auto item_id = runtime.ReadWord(item);
    const auto remaining = runtime.RemoveItem(item_id, 1u);
    if (remaining == 0u) {
        runtime.SetProperty(state.Read(108u), 0u);
        runtime.ShowOrHideDialog(0u, 0u, 0u);
    } else {
        runtime.SetProperty(state.Read(108u),
                            runtime.ReadWord(remaining + 4u));
    }
    runtime.PopulateList(state, state.Read(108u), false);
    return true;
}

}  // namespace pal4
