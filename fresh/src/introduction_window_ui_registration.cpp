#include "pal4/introduction_window_ui_registration.h"

namespace pal4 {

std::uint32_t IntroductionWindowUiRegistration::RegisterShowUi(
    IIntroductionWindowUiRuntime& runtime) {
    auto callback = runtime.Allocate(8u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 4u, runtime.WindowNameAddress());
        runtime.WriteWord(callback, kCallbackVtable);
    }
    try {
        const auto manager = runtime.FrameManager();
        runtime.InsertOrReplace(manager + 0x104u,
                                "show_introduction_ui", callback);
    } catch (...) {
        if (callback != 0u) runtime.DeleteCallback(callback, 1u);
        throw;
    }
    if (callback != 0u) runtime.DeleteCallback(callback, 1u);
    return kResultAddress;
}

std::uint32_t IntroductionWindowUiRegistration::RegisterShowUiThunk(
    IIntroductionWindowUiRuntime& runtime) {
    return RegisterShowUi(runtime);
}

}  // namespace pal4
