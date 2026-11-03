#include "pal4/script_equipment_acquisition.h"

namespace pal4 {
namespace {

struct TextOwner {
    IScriptEquipmentAcquisitionRuntime& runtime;
    std::uint32_t initialized{};

    ~TextOwner() {
        while (initialized != 0u)
            (void)runtime.CleanupText(--initialized);
    }
    std::uint8_t Close() noexcept {
        (void)runtime.CleanupText(1u);
        initialized = 0u;
        return runtime.CleanupText(0u);
    }
};

}  // namespace

std::uint8_t ScriptEquipmentAcquisition::Add(
    std::uint32_t id, std::uint8_t show_hint,
    IScriptEquipmentAcquisitionRuntime& runtime) {
    const auto window = runtime.CreateHintWindow(runtime.UiFrameManager());
    if (window == 0u) return 0u;
    if (runtime.HasEquipment(runtime.GameDatabase() + 0x58u, id) == 0u)
        return 0u;

    const auto record = runtime.FindEquipment(
        runtime.GameDatabase() + 0x58u, id);
    runtime.AddToInventory(runtime.Inventory() + 0x7cu, id);
    if (show_hint == 0u) return 0u;

    TextOwner text{runtime};
    runtime.InitializeText(0u);
    text.initialized = 1u;
    runtime.InitializeText(1u);
    text.initialized = 2u;
    const auto name = runtime.EquipmentName(record);
    const auto format = runtime.GameString(runtime.GameDatabase() + 0x34u,
                                           0xfbu);
    runtime.FormatText(format, name, 1u);
    runtime.AppendText();
    runtime.ShowHint(window, runtime.HintText(), 0.5f, 0.15f, 1.0f,
                     0u, 1u);
    runtime.PlaySound(runtime.AudioManager(), show_hint,
                      "gamedata\\PALSound\\WA06.wav", 1u);
    return text.Close();
}

}  // namespace pal4
