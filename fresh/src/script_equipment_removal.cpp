#include "pal4/script_equipment_removal.h"

namespace pal4 {

std::uint8_t ScriptEquipmentRemoval::Remove(
    const std::int32_t id, const std::uint8_t show_hint,
    IScriptEquipmentRemovalRuntime& runtime) {
    const auto hint = runtime.CreateHintWindow(runtime.FrameManager());
    if (hint == 0u) return 0u;
    if (!runtime.HasRecord(runtime.GameDatabase() + 0x58u, id)) return 0u;
    const auto record = runtime.FindRecord(runtime.GameDatabase() + 0x58u, id);
    const auto category = runtime.ReadWord(record + 0x94u);
    const auto count = runtime.ElementCount(runtime.Inventory() + 0x7cu,
                                            category);
    if (count <= 0) return static_cast<std::uint8_t>(count);

    std::uint8_t last_result = 0u;
    for (std::int32_t index = 0; index < count; ++index) {
        const auto item_category = runtime.ReadWord(record + 0x94u);
        const auto element = runtime.ElementAt(runtime.Inventory() + 0x7cu,
            item_category, static_cast<std::uint32_t>(index));
        if (element == 0u) {
            last_result = 0u;
            continue;
        }
        const auto item_id = runtime.ReadWord(element + 0x90u);
        if (item_id != static_cast<std::uint32_t>(id)) {
            last_result = static_cast<std::uint8_t>(id);
            continue;
        }
        const auto condition = runtime.CheckPlayersCondition(element);
        last_result = static_cast<std::uint8_t>(condition);
        if (condition) continue;

        runtime.RemoveElement(runtime.Inventory() + 0x7cu, element);
        if (show_hint == 0u) return 0u;

        const auto source = runtime.InitializeText();
        try {
            const auto formatted = runtime.InitializeText();
            try {
                auto name = runtime.ReadWord(record + 0x9cu);
                if (name == 0u) name = runtime.EmptyText();
                const auto format_record = runtime.FindGameString(
                    runtime.GameDatabase() + 0x34u, 0x7eu);
                auto format = runtime.ReadWord(format_record + 4u);
                if (format == 0u) format = runtime.EmptyText();
                runtime.FormatText(formatted, format, name);
                runtime.AppendText(formatted, source);
                runtime.ShowHint(hint, formatted, 0.5f, 0.15f, 1.0f, 0u, 1u);
                runtime.PlayMusic(runtime.AudioManager(), show_hint,
                                  "gamedata\\PALSound\\WA07.wav", 1u);
            } catch (...) {
                (void)runtime.CleanupText(formatted);
                throw;
            }
            (void)runtime.CleanupText(formatted);
        } catch (...) {
            (void)runtime.CleanupText(source);
            throw;
        }
        return runtime.CleanupText(source);
    }
    return last_result;
}

}  // namespace pal4
