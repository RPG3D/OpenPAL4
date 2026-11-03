#include "pal4/ui_frame_manager_singleton.h"

#include <array>
#include <cstdint>

namespace pal4 {

std::uint32_t UiFrameManagerSingleton::Initialize(
    const std::uint32_t manager,
    IUiFrameManagerInitializeRuntime& runtime) {
    const auto logger = runtime.InitializeLogger();
    runtime.LogCursorLoading(
        logger,
        "D:\\VcProject\\Pal4\\project\\Sourcecode\\PAL4\\PAL4Game\\UiFrame\\uiFrameManager.cpp",
        110u,
        "FILE:  [%s]\nLINE:  [%d]\nINFO:  uiCursorManager load cursor resource...\n\n\n");
    runtime.ProcessFile3(runtime.PkManager());
    constexpr std::array<const char*, 4> cursors{
        "default.cur", "none.ico", "pickup.ani", "talk.ani"};
    for (const auto* name : cursors) {
        runtime.LoadCursor(manager, name);
    }
    return runtime.ProcessFile4(runtime.PkManager());
}

}  // namespace pal4
