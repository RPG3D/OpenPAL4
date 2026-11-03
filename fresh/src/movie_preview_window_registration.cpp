#include "pal4/movie_preview_window_registration.h"

namespace pal4 {

const char* MoviePreviewWindowRegistration::RegisterName(
    const char*& destination) noexcept {
    destination = "moviePreviewWindow";
    return destination;
}

const char* MoviePreviewWindowRegistration::RegisterNameThunk(
    const char*& destination) noexcept {
    return RegisterName(destination);
}

std::uint32_t MoviePreviewWindowRegistration::CreateObject(
    std::uint32_t name, IMoviePreviewWindowRegistrationRuntime& runtime) {
    const auto storage = runtime.Allocate(0x70u);
    if (storage == 0u) return 0u;
    try {
        return runtime.Construct(storage, name);
    } catch (...) {
        runtime.Free(storage);
        throw;
    }
}

std::uint32_t MoviePreviewWindowRegistration::RegisterFactory(
    IMoviePreviewWindowRegistrationRuntime& runtime) {
    const auto manager = runtime.FrameManager();
    return runtime.RegisterFactory(manager, runtime.RegisteredName(),
                                   &CreateObject);
}

std::uint32_t MoviePreviewWindowRegistration::RegisterFactoryThunk(
    IMoviePreviewWindowRegistrationRuntime& runtime) {
    return RegisterFactory(runtime);
}

std::uint32_t MoviePreviewWindowRegistration::RegisterShowCommand(
    IMoviePreviewWindowRegistrationRuntime& runtime) {
    const auto name = runtime.LiteralName();
    auto callback = runtime.Allocate(8u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 4u, name);
        runtime.WriteWord(callback, kCommandCallbackVtable);
    }
    try {
        const auto manager = runtime.FrameManager();
        runtime.InsertOrReplace(manager + 0x104u, "show_movie_preview",
                                callback);
    } catch (...) {
        if (callback != 0u) runtime.DeleteCallback(callback, 1u);
        throw;
    }
    if (callback != 0u) runtime.DeleteCallback(callback, 1u);
    return kCommandResultAddress;
}

std::uint32_t MoviePreviewWindowRegistration::RegisterShowCommandThunk(
    IMoviePreviewWindowRegistrationRuntime& runtime) {
    return RegisterShowCommand(runtime);
}

}  // namespace pal4
