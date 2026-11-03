#include "pal4/main_window_ui_registration.h"

namespace pal4 {
namespace {

std::uint32_t Register(std::string_view command,
                       std::uint32_t vtable, std::uint32_t target,
                       std::uint32_t result,
                       IMainWindowUiRuntime& runtime) {
    auto callback = runtime.Allocate(8u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 4u, target);
        runtime.WriteWord(callback, vtable);
    }
    try {
        const auto manager = runtime.FrameManager();
        runtime.InsertOrReplace(manager + 0x104u, command, callback);
    } catch (...) {
        if (callback != 0u) runtime.DeleteCallback(callback, 1u);
        throw;
    }
    if (callback != 0u) runtime.DeleteCallback(callback, 1u);
    return result;
}

}  // namespace

std::uint32_t MainWindowUiRegistration::RegisterLogo(
    IMainWindowUiRuntime& runtime) {
    return Register("show_logo_ui", kLogoCallbackVtable,
                    runtime.WindowNameAddress(), kLogoResultAddress,
                    runtime);
}

std::uint32_t MainWindowUiRegistration::RegisterLogoThunk(
    IMainWindowUiRuntime& runtime) {
    return RegisterLogo(runtime);
}

std::uint32_t MainWindowUiRegistration::RegisterMain(
    IMainWindowUiRuntime& runtime) {
    return Register("show_main_ui", kMainCallbackVtable,
                    runtime.ProcessEventsAddress(), kMainResultAddress,
                    runtime);
}

std::uint32_t MainWindowUiRegistration::RegisterMainThunk(
    IMainWindowUiRuntime& runtime) {
    return RegisterMain(runtime);
}

std::uint32_t MainWindowUiRegistration::RegisterClose(
    IMainWindowUiRuntime& runtime) {
    return Register("close_main_ui", kMainCallbackVtable,
                    runtime.CloseMainAddress(), kCloseResultAddress,
                    runtime);
}

std::uint32_t MainWindowUiRegistration::RegisterCloseThunk(
    IMainWindowUiRuntime& runtime) {
    return RegisterClose(runtime);
}

}  // namespace pal4
