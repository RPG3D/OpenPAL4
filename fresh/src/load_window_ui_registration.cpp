#include "pal4/load_window_ui_registration.h"

namespace pal4 {

std::uint32_t LoadWindowUiRegistration::RegisterShowUi(
    ILoadWindowUiRuntime& runtime) {
    auto callback = runtime.Allocate(8u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 4u, runtime.WindowNameAddress());
        runtime.WriteWord(callback, kCallbackVtable);
    }
    try {
        const auto manager = runtime.FrameManager();
        runtime.InsertOrReplace(manager + 0x104u,
                                "show_load_ui", callback);
    } catch (...) {
        if (callback != 0u) runtime.DeleteCallback(callback, 1u);
        throw;
    }
    if (callback != 0u) runtime.DeleteCallback(callback, 1u);
    return kResultAddress;
}

std::uint32_t LoadWindowUiRegistration::RegisterShowUiThunk(
    ILoadWindowUiRuntime& runtime) {
    return RegisterShowUi(runtime);
}

}  // namespace pal4
