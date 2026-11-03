#include "pal4/ol_info_commands.h"

namespace pal4 {

std::uint8_t OlInfoCommands::Close(
    std::uint32_t object_address, IOlInfoCommandRuntime& runtime) {
    runtime.HideWindow(object_address, 0u, 0u);
    OlInfoEventWords words{1u, 0u};
    const auto manager = runtime.FrameManager();
    runtime.ProcessCommand(manager + 0x104u, "show_logo_ui", words);
    return 1u;
}

std::uint8_t OlInfoCommands::OnMemberCloseEvent(
    std::uint32_t member_address, std::uint32_t event_address,
    IOlInfoCommandRuntime& runtime) {
    if (runtime.ReadWord(event_address + 0x10u) != 1u) return 0u;
    const auto base_event = runtime.ConstructBaseEvent();
    try {
        (void)Close(member_address - 0x28u, runtime);
    } catch (...) {
        runtime.DestroyBaseEvent(base_event);
        throw;
    }
    runtime.DestroyBaseEvent(base_event);
    return 1u;
}

}  // namespace pal4
