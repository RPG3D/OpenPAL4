#include "pal4/main_window_registration.h"

namespace pal4 {

const char* MainWindowRegistration::RegisterName(
    const char*& destination) noexcept {
    destination = "MainWindow";
    return destination;
}

const char* MainWindowRegistration::RegisterNameThunk(
    const char*& destination) noexcept {
    return RegisterName(destination);
}

std::uint32_t MainWindowFactory::CreateObject(
    std::uint32_t name, IMainWindowFactoryRuntime& runtime) {
    const auto storage = runtime.Allocate(0x6cu);
    if (storage == 0u) return 0u;
    try {
        return runtime.Construct(storage, name);
    } catch (...) {
        runtime.Free(storage);
        throw;
    }
}

std::uint32_t MainWindowFactory::RegisterCreateObject(
    IMainWindowFactoryRuntime& runtime) {
    const auto manager = runtime.FrameManager();
    const auto name = runtime.RegisteredName();
    return runtime.RegisterFactory(manager, name, &CreateObject);
}

std::uint32_t MainWindowFactory::RegisterCreateObjectThunk(
    IMainWindowFactoryRuntime& runtime) {
    return RegisterCreateObject(runtime);
}

}  // namespace pal4
