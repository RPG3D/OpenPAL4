#include "pal4/rt2d_animation.h"

#include <cstring>

namespace pal4 {

char* Rt2dAnimation::NormalizePath(
    const char* source, IRt2dAnimationRuntime& runtime) {
    const auto size = std::strlen(source) + 1u;
    char* const path = runtime.Allocate(size);
    if (path == nullptr) return nullptr;

    std::memcpy(path, source, size);
    for (char* separator = std::strchr(path, '/'); separator != nullptr;
         separator = std::strchr(path, '/')) {
        *separator = '\\';
    }
    return path;
}

int Rt2dAnimation::ReleasePath(char* path,
                               IRt2dAnimationRuntime& runtime) {
    return path == nullptr ? 0 : runtime.Release(path);
}

int Rt2dAnimation::Initialize(IRt2dAnimationRuntime& runtime) {
    runtime.Open2D(0);
    runtime.OpenAnimation();
    runtime.SetFlatNear(runtime.GameDepth() + 1.0f);
    runtime.SetFlatFar(32.0f);
    char* const path = NormalizePath("gamedata/animation/font/", runtime);
    runtime.CachePath(path);
    return ReleasePath(path, runtime);
}

int Rt2dAnimation::SetCamera(std::uint32_t camera,
                             IRt2dAnimationRuntime& runtime) {
    return runtime.SetCamera(camera);
}

int Rt2dAnimation::Close(IRt2dAnimationRuntime& runtime) {
    runtime.CloseAnimation();
    return runtime.Close2D();
}

void Rt2dMaestroHandle::Destroy(Rt2dMaestroState& state,
                                IRt2dMaestroRuntime& runtime) {
    if (state.maestro == 0) return;
    const auto dictionary = runtime.InitializeTextureDictionary();
    runtime.SetCurrentTextureDictionary(dictionary);
    runtime.DestroyMaestro(state.maestro);
    state.maestro = 0;
    state.auxiliary = 0;
}

}  // namespace pal4
