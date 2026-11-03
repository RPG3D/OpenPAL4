#include "pal4/script_inventory_change.h"

namespace pal4 {
namespace {

struct TextOwner {
    IScriptInventoryChangeRuntime& runtime;
    bool active{true};
    ~TextOwner() {
        if (active) (void)runtime.CleanupTextBuffer();
    }
    std::uint8_t Close() noexcept {
        active = false;
        return runtime.CleanupTextBuffer();
    }
};

std::uint8_t Change(std::uint32_t id, std::int32_t count,
                    std::uint8_t show_hint, bool adding,
                    IScriptInventoryChangeRuntime& runtime) {
    const auto window = runtime.CreateHintWindow(runtime.UiFrameManager());
    if (window == 0u) return 0u;
    const auto present = runtime.HasProperty(runtime.GameDatabase(), id);
    if (present == 0u) return present;

    const auto property = runtime.FindProperty(runtime.GameDatabase(), id);
    const auto inventory = runtime.Inventory();
    if (adding) runtime.AddItem(inventory, id, count);
    else runtime.RemoveItem(inventory, id, count);
    if (show_hint == 0u) return show_hint;

    runtime.InitializeTextBuffer();
    TextOwner text{runtime};
    const auto name = runtime.PropertyName(property);
    const auto format = runtime.GameString(runtime.GameDatabase() + 0x34u,
                                           adding ? 0xfbu : 0xfcu);
    runtime.FormatText(format, name, count);
    runtime.ShowHint(window, runtime.TextBuffer(), 0.5f, 0.15f, 1.0f,
                     0u, 1u);
    runtime.PlaySound(runtime.AudioManager(), show_hint,
                      adding ? "gamedata\\PALSound\\WA06.wav"
                             : "gamedata\\PALSound\\WA07.wav",
                      1u);
    return text.Close();
}

}  // namespace

std::uint8_t ScriptInventoryChange::Add(
    std::uint32_t id, std::int32_t count, std::uint8_t show_hint,
    IScriptInventoryChangeRuntime& runtime) {
    return Change(id, count, show_hint, true, runtime);
}

std::uint8_t ScriptInventoryChange::Remove(
    std::uint32_t id, std::int32_t count, std::uint8_t show_hint,
    IScriptInventoryChangeRuntime& runtime) {
    return Change(id, count, show_hint, false, runtime);
}

}  // namespace pal4
