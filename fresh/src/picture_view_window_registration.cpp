#include "pal4/picture_view_window_registration.h"

namespace pal4 {

const char* PictureViewWindowRegistration::RegisterName(
    const char*& destination) noexcept {
    destination = "PictureViewWindow";
    return destination;
}

const char* PictureViewWindowRegistration::RegisterNameThunk(
    const char*& destination) noexcept {
    return RegisterName(destination);
}

std::uint32_t PictureViewWindowRegistration::CreateObject(
    std::uint32_t name, IPictureViewWindowRegistrationRuntime& runtime) {
    const auto storage = runtime.Allocate(0x64u);
    if (storage == 0u) return 0u;
    try {
        return runtime.Construct(storage, name);
    } catch (...) {
        runtime.Free(storage);
        throw;
    }
}

std::uint32_t PictureViewWindowRegistration::RegisterFactory(
    IPictureViewWindowRegistrationRuntime& runtime) {
    const auto manager = runtime.FrameManager();
    const auto name = runtime.RegisteredName();
    return runtime.RegisterFactory(manager, name, &CreateObject);
}

std::uint32_t PictureViewWindowRegistration::RegisterFactoryThunk(
    IPictureViewWindowRegistrationRuntime& runtime) {
    return RegisterFactory(runtime);
}

std::uint32_t PictureViewWindowRegistration::RegisterViewCommand(
    IPictureViewWindowRegistrationRuntime& runtime) {
    auto callback = runtime.Allocate(8u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 4u, kViewHandler);
        runtime.WriteWord(callback, kCommandCallbackVtable);
    }
    try {
        const auto manager = runtime.FrameManager();
        runtime.InsertOrReplace(manager + 0x104u, "view_picture",
                                callback);
    } catch (...) {
        if (callback != 0u) runtime.DeleteCallback(callback, 1u);
        throw;
    }
    if (callback != 0u) runtime.DeleteCallback(callback, 1u);
    return kCommandResultAddress;
}

std::uint32_t PictureViewWindowRegistration::RegisterViewCommandThunk(
    IPictureViewWindowRegistrationRuntime& runtime) {
    return RegisterViewCommand(runtime);
}

}  // namespace pal4
