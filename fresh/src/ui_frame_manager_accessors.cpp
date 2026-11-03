#include "pal4/ui_frame_manager_singleton.h"

namespace pal4 {

std::uint8_t UiFrameManagerSingleton::SetMessageFlag(
    const std::uint32_t manager, const std::uint8_t flag,
    IUiFrameManagerAccessRuntime& runtime) {
    runtime.WriteByte(manager + 0x221u, flag);
    return flag;
}

std::uint32_t UiFrameManagerSingleton::GetMenuWindow(
    const std::uint32_t manager, IUiFrameManagerAccessRuntime& runtime) {
    const auto cached = runtime.ReadWord(manager + 0x1a8u);
    if (cached == 0u) {
        const auto memory = runtime.Allocate(0x58u);
        auto constructed = 0u;
        if (memory != 0u) {
            try {
                constructed = runtime.ConstructMenu(memory);
            } catch (...) {
                runtime.FreeAfterConstructionFailure(memory);
                throw;
            }
        }
        runtime.WriteWord(manager + 0x1a8u, constructed);
    }
    return runtime.ReadWord(manager + 0x1a8u);
}

}  // namespace pal4
