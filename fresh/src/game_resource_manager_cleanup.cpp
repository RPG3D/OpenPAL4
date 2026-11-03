#include "pal4/game_resource_manager.h"

#include <algorithm>
#include <bit>

namespace pal4 {

std::uint32_t GameResourceManager::CleanupClump(
    const std::uint32_t texture_context, IGameResourceManagerRuntime& runtime,
    IRwDictSchemaRuntime& schema_runtime) {
    RwDictSchema schema;
    schema.dict = words[37];
    schema.list = words[38];
    schema.time_scale = std::bit_cast<float>(words[39]);
    (void)schema.Cleanup(schema_runtime);
    words[37] = schema.dict;
    words[38] = schema.list;
    (void)CleanupShadowClump(texture_context, runtime);

    for (const auto index : {74u, 75u}) {
        if (words[index] != 0) {
            runtime.DestroyIm3D(words[index]);
            words[index] = 0;
        }
    }
    if (words[5] != 0) {
        const auto clump = words[5];
        const auto world = runtime.ClumpWorld(clump);
        if (world != 0) runtime.RemoveClump(world, clump);
        runtime.DestroyClump(clump);
        words[5] = 0;

        const auto path = runtime.ReadString(words[2]) +
                          runtime.ReadString(words[3]) + ".dff";
        const auto temporary = runtime.Allocate(
            static_cast<std::uint32_t>(path.size() + 1));
        runtime.StoreString(temporary, path);
        runtime.RemoveTextureByName(texture_context, path);
        if (temporary != 0) (void)runtime.Free(temporary);
    }

    for (std::size_t index = 1; index <= 4; ++index) {
        (void)GameConfigState::FreeAndNull(words[index], runtime);
    }
    GameConfigState config;
    std::copy(words.begin() + 76, words.end(), config.words.begin());
    const auto result = config.Cleanup(runtime);
    std::copy(config.words.begin(), config.words.end(), words.begin() + 76);
    words[9] = 0;
    return result;
}

} // namespace pal4
