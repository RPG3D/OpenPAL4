#include "pal4/game_world_material_operations.h"

namespace pal4 {

std::uint32_t GameWorldMaterialOperations::SetAtomicFlags(
    const std::uint32_t atomic, const std::uint32_t flags,
    IGameWorldMaterialFlagRuntime& runtime) {
    const auto geometry = runtime.GeometryOfAtomic(atomic);
    const auto geometry_flags = runtime.GeometryFlags(geometry);
    const auto count = runtime.MaterialCount(geometry);
    if ((geometry_flags & 8u) != 0u && (geometry_flags & 0x40u) == 0u)
        runtime.SetGeometryFlags(geometry, geometry_flags | 0x40u);
    for (std::int32_t index = 0; index < count; ++index)
        runtime.SetMaterialFlags(runtime.MaterialAt(geometry, index), flags);
    return atomic;
}

std::uint32_t GameWorldMaterialOperations::SetClumpFlags(
    const std::uint32_t clump, const std::uint32_t flags,
    IGameWorldMaterialFlagRuntime& runtime) {
    return runtime.ForAllClumpAtomics(clump, flags);
}

std::uint32_t GameWorldMaterialOperations::CaptureAtomic(
    const std::uint32_t atomic, std::uint32_t& selected_atomic) {
    selected_atomic = atomic;
    return 0u;
}

std::uint32_t GameWorldMaterialOperations::SetTexture(
    const std::uint32_t material, const std::uint32_t texture,
    IGameWorldMaterialTextureRuntime& runtime) {
    runtime.SetMaterialTexture(material, texture);
    return material;
}

std::uint32_t GameWorldMaterialOperations::SetAtomicTexture(
    const std::uint32_t atomic, const std::uint32_t texture,
    IGameWorldMaterialTextureRuntime& runtime) {
    const auto geometry = runtime.GeometryOfAtomic(atomic);
    if (geometry != 0u) {
        auto value = texture;
        runtime.ForAllMaterials(
            geometry, GameWorldMaterialTraversal::SetTexture, value);
    }
    return atomic;
}

std::uint32_t GameWorldMaterialOperations::CaptureTexture(
    const std::uint32_t material, std::uint32_t& texture,
    IGameWorldMaterialTextureRuntime& runtime) {
    texture = runtime.MaterialTexture(material);
    return 0u;
}

std::uint32_t GameWorldMaterialOperations::GetAtomicTexturePath(
    const std::uint32_t atomic, std::string& destination,
    const std::uint32_t capacity,
    IGameWorldMaterialTextureRuntime& runtime) {
    if (atomic == 0u)
        return 0u;
    const auto geometry = runtime.GeometryOfAtomic(atomic);
    if (geometry == 0u)
        return 0u;
    std::uint32_t texture = 0u;
    runtime.ForAllMaterials(
        geometry, GameWorldMaterialTraversal::GetTexture, texture);
    if (texture == 0u)
        return 0u;
    return runtime.CopyTexturePath(texture + 16u, destination, capacity);
}

std::uint32_t GameWorldMaterialOperations::CheckAtomicTexturePath(
    const std::uint32_t atomic, std::uint32_t& match_flag,
    const std::string_view candidate,
    IGameWorldMaterialTextureRuntime& runtime) {
    const auto geometry = runtime.GeometryOfAtomic(atomic);
    const auto count = runtime.MaterialCount(geometry);
    for (std::int32_t index = 0; index < count; ++index) {
        const auto material = runtime.MaterialAt(geometry, index);
        const auto texture = runtime.MaterialTexture(material);
        if (texture == 0u)
            continue;
        std::string path;
        static_cast<void>(runtime.CopyTexturePath(texture + 16u, path, 256u));
        if (runtime.CompareTexturePath(candidate, path))
            match_flag = 0u;
    }
    return atomic;
}

std::uint32_t GameWorldMaterialOperations::CheckClumpTexturePath(
    const std::uint32_t clump, const std::uint32_t texture,
    IGameWorldMaterialTextureRuntime& runtime) {
    std::uint32_t first_atomic = 0u;
    runtime.CaptureFirstAtomic(clump, first_atomic);
    std::string path;
    static_cast<void>(GetAtomicTexturePath(
        first_atomic, path, 256u, runtime));
    if (path.empty())
        return 0u;
    std::uint32_t match_flag = 1u;
    runtime.CheckAllAtomicTextures(clump, path, match_flag);
    if (match_flag != 0u)
        static_cast<void>(SetClumpTexture(clump, texture, runtime));
    return match_flag;
}

std::uint32_t GameWorldMaterialOperations::CheckClumpTexturePathOnly(
    const std::uint32_t clump, IGameWorldMaterialTextureRuntime& runtime) {
    std::uint32_t first_atomic = 0u;
    runtime.CaptureFirstAtomic(clump, first_atomic);
    std::string path;
    static_cast<void>(GetAtomicTexturePath(
        first_atomic, path, 256u, runtime));
    if (path.empty())
        return 0u;
    std::uint32_t match_flag = 1u;
    runtime.CheckAllAtomicTextures(clump, path, match_flag);
    return match_flag;
}

std::uint32_t GameWorldMaterialOperations::SetClumpTexture(
    const std::uint32_t clump, const std::uint32_t texture,
    IGameWorldMaterialTextureRuntime& runtime) {
    return runtime.SetAllAtomicTextures(clump, texture);
}

}  // namespace pal4
