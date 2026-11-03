#include "pal4/script_event_volume_visibility.h"

namespace pal4 {

std::uint32_t ScriptEventVolumeVisibility::Set(
    std::uint32_t name_payload, std::uint8_t visible,
    IScriptEventVolumeVisibilityRuntime& runtime) {
    auto name = runtime.ReadWord(name_payload + 4u);
    if (name == 0u) name = runtime.EmptyStringAddress();
    const auto volume = runtime.FindVolume(runtime.EventVolumeManager(), name);
    if (volume == 0u) return 0u;

    const auto arena = runtime.ArenaManager();
    if (visible == 0u) return runtime.Deactivate(arena, volume);
    (void)runtime.Deactivate(arena, volume);
    return runtime.Activate(runtime.ArenaManager(), volume);
}

}  // namespace pal4
