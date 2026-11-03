#include "pal4/property_window_selection.h"

namespace pal4 {

void PropertyWindowSelection::ShowChoices(
    PropertyWindowState& state, std::uint32_t event_data,
    IPropertyWindowSelectionRuntime& runtime) {
    const auto dialog = runtime.Dialog();
    runtime.HideAndCleanup(dialog);
    const auto item = runtime.HoveredItem(state);
    if (!item) return;

    bool can_use = runtime.CheckComponent(item->definition);
    if (can_use && item->id == 3067u) {
        const auto object = runtime.ArenaObjectName();
        const auto scene = runtime.ArenaSceneName();
        const auto key = runtime.BuildSpecialKey(scene, object);
        const auto properties = runtime.SpecialProperties();
        bool matched = false;
        bool first_earthball = false;
        for (const auto& property : properties) {
            if (runtime.KeysEqual(key, property.key) && !matched) {
                first_earthball = property.earthball > 0;
                matched = true;
            }
        }
        if (matched) can_use = first_earthball;
    }

    const bool can_discard = item->type != 4u && item->id != 3067u;
    const auto use_label = runtime.ConvertToUtf8(runtime.TriggerText(22u));
    runtime.CreateButton(dialog, "use_property", item->address,
                         use_label, can_use);
    const auto discard_label = runtime.ConvertToUtf8(
        runtime.TriggerText(23u));
    runtime.CreateButton(dialog, "discard_property", item->address,
                         discard_label, can_discard);
    runtime.ShowDialog(dialog, event_data);
}

}  // namespace pal4
