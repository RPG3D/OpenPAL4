#include "pal4/game_resource_manager.h"

#include <algorithm>
#include <bit>

namespace pal4 {

GameResourceManager* GameResourceManager::LoadClump(
    const std::string_view label, const std::string_view directory,
    const std::string_view model, const std::uint32_t texture_context,
    const std::uint32_t mode, const std::uint32_t extra,
    const bool load_effects, IGameResourceManagerRuntime& runtime,
    IRwDictSchemaRuntime& schema_runtime) {
    (void)CleanupClump(texture_context, runtime, schema_runtime);
    words[1] = runtime.Allocate(static_cast<std::uint32_t>(label.size() + 1));
    words[2] = runtime.Allocate(static_cast<std::uint32_t>(directory.size() + 1));
    words[3] = runtime.Allocate(static_cast<std::uint32_t>(model.size() + 1));
    if (words[1] == 0 || words[2] == 0 || words[3] == 0) return nullptr;

    const auto path_size = directory.size() + model.size() + 5;
    const auto uva = runtime.Allocate(static_cast<std::uint32_t>(path_size));
    const auto dff = runtime.Allocate(static_cast<std::uint32_t>(path_size));
    if (uva == 0 || dff == 0) return nullptr;

    runtime.StoreString(words[1], label);
    runtime.StoreString(words[2], directory);
    runtime.StoreString(words[3], model);
    const auto base = std::string(directory) + std::string(model);
    const auto uva_path = base + ".uva";
    const auto dff_path = base + ".dff";
    runtime.StoreString(uva, uva_path);
    runtime.StoreString(dff, dff_path);
    runtime.ImageReallocateAndCopy(words[2]);

    RwDictSchema schema;
    schema.dict = words[37];
    schema.list = words[38];
    schema.time_scale = std::bit_cast<float>(words[39]);
    (void)schema.LoadDict(uva_path.c_str(), schema_runtime);
    words[37] = schema.dict;
    words[38] = schema.list;

    words[5] = runtime.LoadWorldClump(dff_path, 0, texture_context);
    if (words[5] == 0) return nullptr;
    words[6] = mode;
    words[8] = extra;
    (void)schema.SetClump(words[5], schema_runtime);
    words[38] = schema.list;
    (void)SetClumpMaterialFlags(1, runtime);
    (void)CalculateBoundingBox(runtime);
    (void)RwGeometryFlags::SetClumpFlags45(words[5], runtime);
    (void)RwClumpOptimizer::OptimizeClump(words[5], runtime);

    if (runtime.CheckTexturePath(words[5])) {
        const auto atomic = runtime.FirstMaterialAtomic(words[5]);
        if (atomic != 0) {
            const auto filename = runtime.SplitFilename(runtime.TexturePath(atomic));
            words[4] = runtime.Allocate(
                static_cast<std::uint32_t>(filename.size() + 1));
            runtime.StoreString(words[4], filename);
        }
    }
    if (load_effects) LoadEffectData(runtime);
    (void)LoadExtSizeScl(runtime);
    (void)runtime.Free(uva);
    (void)runtime.Free(dff);

    FinalizeClumpLoad(runtime);
    return this;
}

}  // namespace pal4
