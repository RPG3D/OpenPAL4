#include "pal4/game_world_asset_loader.h"

#include "pal4/game_world_resource_name.h"

namespace pal4 {
namespace {

enum class AssetKind { Clump, Atomic };

std::uint32_t Load(const std::string_view path, const std::uint32_t image,
                   const std::uint32_t dictionary,
                   const AssetKind kind, IGameWorldAssetRuntime& runtime) {
    if (image != 0u)
        runtime.ReallocateImage(image);
    if (dictionary == 0u)
        return 0u;

    const auto previous = runtime.GetCurrentDictionary();
    const auto name = GameWorldResourceName::Build(path);
    std::uint32_t created = 0u;
    if (runtime.TextureExists(dictionary, name)) {
        runtime.SelectTexture(dictionary, name);
    } else {
        created = runtime.CreateDictionary();
        runtime.SetCurrentDictionary(created);
    }

    const auto stream = runtime.OpenStream(2u, 1u, path);
    if (stream == 0u) {
        if (created != 0u)
            runtime.DestroyDictionary(created);
        else
            runtime.RemoveTexture(dictionary, name);
        runtime.SetCurrentDictionary(previous);
        return 0u;
    }

    std::uint32_t asset = 0u;
    const auto chunk = kind == AssetKind::Clump ? 16u : 11u;
    if (runtime.FindChunk(stream, chunk, 0u, 0u)) {
        asset = kind == AssetKind::Clump ? runtime.ReadClump(stream)
                                         : runtime.ReadAtomic(stream);
        if (asset == 0u) {
            if (created != 0u)
                runtime.DestroyDictionary(created);
            else
                runtime.RemoveTexture(dictionary, name);
            runtime.SetCurrentDictionary(previous);
            runtime.CloseStream(stream, 0u);
            return 0u;
        }
        if (created != 0u &&
            runtime.AddTexture(dictionary, name, created))
            runtime.SelectTexture(dictionary, name);
    }
    runtime.CloseStream(stream, 0u);
    return asset;
}

}  // namespace

std::uint32_t GameWorldAssetLoader::LoadClump(
    const std::string_view path, const std::uint32_t image,
    const std::uint32_t dictionary, IGameWorldAssetRuntime& runtime) {
    return Load(path, image, dictionary, AssetKind::Clump, runtime);
}

std::uint32_t GameWorldAssetLoader::LoadAtomic(
    const std::string_view path, const std::uint32_t image,
    const std::uint32_t dictionary, IGameWorldAssetRuntime& runtime) {
    return Load(path, image, dictionary, AssetKind::Atomic, runtime);
}

}  // namespace pal4
