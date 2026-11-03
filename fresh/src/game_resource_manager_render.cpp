#include "pal4/game_resource_manager.h"


namespace pal4 {

bool GameResourceManager::RenderClump(
    const std::uint32_t camera, IGameResourceManagerRuntime& runtime) {
    if (words[9] == 0) return false;
    if (words[6] == 0) return true;
    if (runtime.FrustumTestSphere(camera, words.data() + 56)) {
        words[7] = 1;
        runtime.RenderClumpRaw(words[5]);
        return true;
    }
    words[7] = 0;
    return false;
}

bool GameResourceManager::RenderClump2(
    const std::uint32_t camera, IGameResourceManagerRuntime& runtime) {
    if (words[9] == 0) return false;
    if (words[6] == 0 ||
        runtime.FrustumTestSphere(camera, words.data() + 56)) {
        if (words[11] != 0 && words[13] != 0 && words[12] != 0) {
            runtime.RenderCameraRaw(words.data() + 14, camera);
        }
    }
    return true;
}


}  // namespace pal4
