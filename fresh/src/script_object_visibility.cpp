#include "pal4/script_object_visibility.h"

namespace pal4 {

std::uint32_t ScriptObjectVisibility::SetVisible(
    const char* name, std::uint8_t visible,
    IScriptObjectVisibilityRuntime& runtime) {
    const std::string_view object_name = name == nullptr ? "" : name;
    const auto object = runtime.FindByName(runtime.ObjectManager(), object_name);
    if (object == 0u) return 0u;

    runtime.RemoveAnimatedObject(runtime.ArenaManager(), object);
    if (visible != 0u) {
        runtime.SetObjectState(object, 1u);
        runtime.AddAnimatedObject(runtime.ArenaManager(), object);
    }
    runtime.RefreshDialog(runtime.MinimapManager());
    const auto word_e0 = runtime.ReadWord(runtime.ArenaManager() + 0xe0u);
    const auto word8 = runtime.ReadWord(runtime.ArenaManager() + 8u);
    return runtime.LogVisibility(0x356u, object_name, word8, word_e0, visible);
}

}  // namespace pal4
