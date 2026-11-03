#include "pal4/movie_play_window_registration.h"

namespace pal4 {

const char* MoviePlayWindowRegistration::RegisterName(
    const char*& destination) noexcept {
    destination = "MoviePlayWindow";
    return destination;
}

const char* MoviePlayWindowRegistration::RegisterNameThunk(
    const char*& destination) noexcept {
    return RegisterName(destination);
}

std::uint32_t MoviePlayWindowRegistration::CreateObject(
    std::uint32_t name, IMoviePlayWindowRegistrationRuntime& runtime) {
    const auto storage = runtime.Allocate(0x44u);
    if (storage == 0u) return 0u;
    try {
        return runtime.Construct(storage, name);
    } catch (...) {
        runtime.Free(storage);
        throw;
    }
}

std::uint32_t MoviePlayWindowRegistration::RegisterFactory(
    IMoviePlayWindowRegistrationRuntime& runtime) {
    const auto manager = runtime.FrameManager();
    const auto name = runtime.RegisteredName();
    return runtime.RegisterFactory(manager, name, &CreateObject);
}

std::uint32_t MoviePlayWindowRegistration::RegisterFactoryThunk(
    IMoviePlayWindowRegistrationRuntime& runtime) {
    return RegisterFactory(runtime);
}

std::uint32_t MoviePlayWindowRegistration::RegisterPlaybackCommand(
    IMoviePlayWindowRegistrationRuntime& runtime) {
    auto callback = runtime.Allocate(8u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 4u, kPlaybackHandler);
        runtime.WriteWord(callback, kCommandCallbackVtable);
    }
    try {
        const auto manager = runtime.FrameManager();
        runtime.InsertOrReplace(manager + 0x104u, "start_play_movie",
                                callback);
    } catch (...) {
        if (callback != 0u) runtime.DeleteCallback(callback, 1u);
        throw;
    }
    if (callback != 0u) runtime.DeleteCallback(callback, 1u);
    return kCommandResultAddress;
}

std::uint32_t MoviePlayWindowRegistration::RegisterPlaybackCommandThunk(
    IMoviePlayWindowRegistrationRuntime& runtime) {
    return RegisterPlaybackCommand(runtime);
}

}  // namespace pal4
