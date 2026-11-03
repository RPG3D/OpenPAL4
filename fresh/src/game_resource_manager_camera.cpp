#include "pal4/game_resource_manager.h"

#include <bit>

namespace pal4 {

bool GameResourceManager::CleanupCamera(IGameResourceManagerRuntime& runtime) {
    runtime.CleanupCamera();
    return true;
}

std::uint32_t GameResourceManager::RenderCamera(
    const std::uint32_t enabled, const std::uint32_t context,
    IGameResourceManagerRuntime& runtime) {
    if (words[12] == 0 || words[13] == 0) return 0;
    words[11] = enabled;
    (void)CleanupCamera(runtime);
    if (enabled == 0) return 0;
    const auto result = runtime.InitCamera(words.data() + 14, context,
        words[12], words[13], words.data() + 52, words.data() + 56);
    if (result != 0) {
        runtime.RenderCameraScene(words.data() + 14,
                                  std::bit_cast<float>(0x3dcccccd),
                                  words.data());
    }
    return result;
}

std::uint32_t GameResourceManager::SetClumpMaterialFlags(
    const std::uint32_t mode, IGameResourceManagerRuntime& runtime) {
    if (words[5] == 0) return 0;
    return runtime.SetClumpMaterialFlagsRaw(words[5], mode);
}

}  // namespace pal4
