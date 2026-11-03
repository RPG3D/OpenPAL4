#include "pal4/script_prescription_addition.h"

namespace pal4 {

std::uint8_t ScriptPrescriptionAddition::Add(
    const std::int32_t id, const std::uint8_t show_hint,
    IScriptPrescriptionAdditionRuntime& runtime) {
    const auto hint = runtime.CreateHintWindow(runtime.FrameManager());
    if (hint == 0u) return 0u;
    if (!runtime.HasRecord(runtime.GameDatabase() + 0x80u, id)) return 0u;
    const auto record = runtime.FindRecord(runtime.GameDatabase() + 0x80u, id);
    runtime.AddPrescription(runtime.Inventory() + 0x104u, id);
    if (show_hint == 0u) return 0u;

    const auto formatted = runtime.InitializeText();
    try {
        const auto display = runtime.InitializeText();
        try {
            auto name = runtime.ReadWord(record + 8u);
            if (name == 0u) name = runtime.EmptyText();
            const auto format_record = runtime.FindGameString(
                runtime.GameDatabase() + 0x34u, 0x7au);
            auto format = runtime.ReadWord(format_record + 4u);
            if (format == 0u) format = runtime.EmptyText();
            runtime.FormatText(formatted, format, name);
            runtime.AppendText(display, formatted);
            runtime.ShowHint(hint, display, 0.5f, 0.15f, 1.0f, 0u, 1u);
            runtime.PlayMusic(runtime.AudioManager(), show_hint,
                              "gamedata\\PALSound\\WA06.wav", 1u);
        } catch (...) {
            (void)runtime.CleanupText(display);
            throw;
        }
        (void)runtime.CleanupText(display);
    } catch (...) {
        (void)runtime.CleanupText(formatted);
        throw;
    }
    return runtime.CleanupText(formatted);
}

}  // namespace pal4
