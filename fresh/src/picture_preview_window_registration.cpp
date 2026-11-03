#include "pal4/picture_preview_window_registration.h"

namespace pal4 {

const char* PicturePreviewWindowRegistration::RegisterName(
    const char*& destination) noexcept {
    destination = "picturePreviewWindow";
    return destination;
}

const char* PicturePreviewWindowRegistration::RegisterNameThunk(
    const char*& destination) noexcept {
    return RegisterName(destination);
}

std::uint32_t PicturePreviewWindowRegistration::CreateObject(
    std::uint32_t name, IPicturePreviewWindowRegistrationRuntime& runtime) {
    const auto storage = runtime.Allocate(0x64u);
    if (storage == 0u) return 0u;
    try {
        return runtime.Construct(storage, name);
    } catch (...) {
        runtime.Free(storage);
        throw;
    }
}

std::uint32_t PicturePreviewWindowRegistration::RegisterFactory(
    IPicturePreviewWindowRegistrationRuntime& runtime) {
    const auto manager = runtime.FrameManager();
    return runtime.RegisterFactory(manager, runtime.RegisteredName(),
                                   &CreateObject);
}

std::uint32_t PicturePreviewWindowRegistration::RegisterFactoryThunk(
    IPicturePreviewWindowRegistrationRuntime& runtime) {
    return RegisterFactory(runtime);
}

std::uint32_t PicturePreviewWindowRegistration::RegisterShowCommand(
    IPicturePreviewWindowRegistrationRuntime& runtime) {
    const auto name = runtime.LiteralName();
    auto callback = runtime.Allocate(8u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 4u, name);
        runtime.WriteWord(callback, kCommandCallbackVtable);
    }
    try {
        const auto manager = runtime.FrameManager();
        runtime.InsertOrReplace(manager + 0x104u, "show_picture_preview",
                                callback);
    } catch (...) {
        if (callback != 0u) runtime.DeleteCallback(callback, 1u);
        throw;
    }
    if (callback != 0u) runtime.DeleteCallback(callback, 1u);
    return kCommandResultAddress;
}

std::uint32_t PicturePreviewWindowRegistration::RegisterShowCommandThunk(
    IPicturePreviewWindowRegistrationRuntime& runtime) {
    return RegisterShowCommand(runtime);
}

}  // namespace pal4
