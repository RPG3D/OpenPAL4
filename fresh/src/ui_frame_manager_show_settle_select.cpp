#include "pal4/ui_frame_manager_show_settle_select.h"

namespace pal4 {

std::uint32_t UiFrameManagerShowSettleSelect::GetHandler(
    IShowSettleSelectRuntime& runtime) {
    return RegisterHandler(runtime);
}

std::uint32_t UiFrameManagerShowSettleSelect::RegisterHandler(
    IShowSettleSelectRuntime& runtime) {
    const auto handler = runtime.Allocate(8u);
    if (handler != 0u) {
        runtime.WriteWord(handler + 4u, kSetVisible);
        runtime.WriteWord(handler, kHandlerVtable);
    }
    try {
        runtime.InsertOrReplace(runtime.FrameManager() + 0x104u,
                                kKey, handler);
    } catch (...) {
        if (handler != 0u) {
            runtime.ReleaseHandler(handler, 1u);
        }
        throw;
    }
    if (handler != 0u) {
        runtime.ReleaseHandler(handler, 1u);
    }
    return kHandlerStorage;
}

}  // namespace pal4
