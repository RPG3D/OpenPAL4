#include "pal4/game_resource_manager.h"

#include <algorithm>

namespace pal4 {

bool GameResourceManager::LoadTexture(
    const std::string_view name, const std::uint32_t mode,
    IGameResourceManagerRuntime& runtime) {
    if (words[10] == 0) return false;
    (void)GameConfigState::FreeAndNull(words[85], runtime);
    words[85] = runtime.Allocate(static_cast<std::uint32_t>(name.size() + 1));
    runtime.StoreString(words[85], name);
    runtime.PrepareTextureErrorString();
    const auto texture = runtime.ReadTexture(name);
    if (texture != 0) runtime.SetTextureBytes(texture, 2, 17);
    std::array<std::uint32_t, 3> arguments{texture, mode, words[73]};
    runtime.ApplyMaterialPipeline(words[5], arguments);
    if (arguments[0] != 0) runtime.DestroyTexture(arguments[0]);
    words[76] = 1;
    return true;
}

bool GameResourceManager::SetTexture(
    const std::uint32_t texture, IGameResourceManagerRuntime& runtime) {
    if (texture == 0) words[76] = 0;
    if (words[77] == 0) return false;
    words[77] = texture;
    runtime.ApplyMaterialPipeline3(words[5], 0, texture);
    return true;
}

bool GameResourceManager::LoadTextures(
    const std::string_view first, const std::string_view second,
    const std::uint32_t variant, const std::uint32_t fourth,
    const std::uint32_t fifth, const std::uint32_t sixth,
    const std::uint32_t seventh, IGameResourceManagerRuntime& runtime) {
    if (words[10] == 0) return false;
    (void)GameConfigState::FreeAndNull(words[85], runtime);
    (void)GameConfigState::FreeAndNull(words[86], runtime);
    words[85] = runtime.Allocate(static_cast<std::uint32_t>(first.size() + 1));
    runtime.StoreString(words[85], first);
    words[86] = runtime.Allocate(static_cast<std::uint32_t>(second.size() + 1));
    runtime.StoreString(words[86], second);
    runtime.PrepareTextureErrorString();
    const auto first_texture = runtime.ReadTexture(first);
    if (first_texture != 0) runtime.SetTextureBytes(first_texture, 2, 17);
    const auto second_texture = runtime.ReadTexture(second);
    if (second_texture != 0) {
        runtime.SetTextureBytes(second_texture, 2, variant != 0 ? 51 : 17);
    }
    std::array<std::uint32_t, 8> arguments{
        variant, second_texture, sixth, fourth, fifth, seventh,
        first_texture, words[73]};
    runtime.ApplyMaterialPipeline2(words[5], arguments);
    if (arguments[1] != 0) runtime.DestroyTexture(arguments[1]);
    if (arguments[6] != 0) runtime.DestroyTexture(arguments[6]);
    words[77] = 1;
    return true;
}

bool GameResourceManager::LoadTexture2(
    const std::string_view name, const std::uint32_t third,
    const std::uint32_t fourth, IGameResourceManagerRuntime& runtime) {
    (void)GameConfigState::FreeAndNull(words[86], runtime);
    words[86] = runtime.Allocate(static_cast<std::uint32_t>(name.size() + 1));
    runtime.StoreString(words[86], name);
    runtime.PrepareTextureErrorString();
    const auto texture = runtime.ReadTexture(name);
    if (texture != 0) runtime.SetTextureBytes(texture, 2, 17);
    std::array<std::uint32_t, 3> arguments{fourth, texture, third};
    runtime.ApplyMaterialPipeline4(words[5], arguments);
    if (arguments[1] != 0) runtime.DestroyTexture(arguments[1]);
    words[78] = arguments[1] != 0 ? 1u : 0u;
    return words[78] != 0;
}

bool GameResourceManager::LoadTexture3(
    const std::uint32_t second, const std::string_view name,
    const std::uint32_t fourth, const std::uint32_t fifth,
    IGameResourceManagerRuntime& runtime) {
    (void)GameConfigState::FreeAndNull(words[88], runtime);
    words[88] = runtime.Allocate(static_cast<std::uint32_t>(name.size() + 1));
    runtime.StoreString(words[88], name);
    words[89] = second;
    runtime.PrepareTextureErrorString();
    const auto texture = runtime.ReadTexture(name);
    if (texture != 0) runtime.SetTextureBytes(texture, 2, 17);
    if (words[74] == 0) words[74] = runtime.CreateIm3D();
    const auto first_im3d = words[74];
    std::uint32_t second_im3d;
    if (words[75] != 0) {
        second_im3d = words[74];
    } else {
        words[75] = runtime.CreateIm3D();
        second_im3d = words[75];
    }
    std::array<std::uint32_t, 6> arguments{
        second, texture, fourth, fifth, first_im3d, second_im3d};
    runtime.ApplyMaterialPipeline5(words[5], arguments);
    if (arguments[1] != 0) runtime.DestroyTexture(arguments[1]);
    words[81] = arguments[1] != 0 ? 1u : 0u;
    return words[81] != 0;
}

bool GameResourceManager::CleanupTexture(IGameResourceManagerRuntime& runtime) {
    if (words[5] == 0) return false;
    runtime.ApplyMaterialPipeline6(words[5]);
    GameConfigState config;
    std::copy(words.begin() + 76, words.end(), config.words.begin());
    (void)config.Cleanup(runtime);
    std::copy(config.words.begin(), config.words.end(), words.begin() + 76);
    return true;
}

bool GameResourceManager::LoadTexture4(
    const std::string_view name, const std::uint32_t target,
    IGameResourceManagerRuntime& runtime) {
    (void)GameConfigState::FreeAndNull(words[4], runtime);
    if (words[5] == 0 || name.empty() || target == 0) return false;
    if (runtime.CheckTexturePathTarget(words[5], target) == 0) return false;
    words[4] = runtime.Allocate(static_cast<std::uint32_t>(name.size() + 1));
    runtime.StoreString(words[4], name);
    (void)CleanupTexture(runtime);
    LoadEffectData(runtime);
    return true;
}

}  // namespace pal4
