#include "pal4/base_object_material_texture.h"

namespace pal4 {
namespace {

std::uint32_t Set(BaseObjectState& state, std::string_view texture_name,
                  std::uint32_t material_offset,
                  IBaseObjectMaterialRuntime& runtime) {
    auto path = runtime.CreatePath(state.Read(8u));
    bool path_cleaned = false;
    const auto cleanup_path = [&] {
        path_cleaned = true;
        runtime.CleanupPath(path);
    };
    try {
        path.append(texture_name);
        const auto material = state.Read(material_offset);
        if (material == 0u) {
            cleanup_path();
            return 0u;
        }
        runtime.AddAndProcessScripts();
        const auto dictionary = runtime.TextureDictionary();
        const auto texture = runtime.ResolveTexture(dictionary, path);
        if (texture != 0u) {
            runtime.SetMaterialTexture(material, texture);
        }
        runtime.CleanupScripts();
        cleanup_path();
        return texture != 0u ? 1u : 0u;
    } catch (...) {
        if (!path_cleaned) cleanup_path();
        throw;
    }
}

std::uint32_t Apply(BaseObjectState& state, std::uint32_t string_offset,
                    std::uint32_t material_offset,
                    IBaseObjectMaterialRuntime& runtime) {
    const auto material = state.Read(material_offset);
    if (material == 0u) return 0u;
    if (static_cast<std::int32_t>(state.Read(string_offset + 4u)) > 0) {
        runtime.AddAndProcessScripts();
        const auto path = state.Read(string_offset);
        const auto dictionary = runtime.TextureDictionary();
        const auto texture = runtime.ResolveStoredTexture(dictionary, path);
        if (texture != 0u) {
            runtime.SetMaterialTexture(material, texture);
        }
        runtime.CleanupScripts();
    }
    return 1u;
}

}  // namespace

std::uint32_t BaseObjectMaterialTexture::SetFace(
    BaseObjectState& state, std::string_view texture_name,
    IBaseObjectMaterialRuntime& runtime) {
    return Set(state, texture_name, 0x550u, runtime);
}

std::uint32_t BaseObjectMaterialTexture::SetLeg(
    BaseObjectState& state, std::string_view texture_name,
    IBaseObjectMaterialRuntime& runtime) {
    return Set(state, texture_name, 0x560u, runtime);
}

std::uint32_t BaseObjectMaterialTexture::ApplyFace(
    BaseObjectState& state, IBaseObjectMaterialRuntime& runtime) {
    return Apply(state, 0x544u, 0x550u, runtime);
}

std::uint32_t BaseObjectMaterialTexture::ApplyLeg(
    BaseObjectState& state, IBaseObjectMaterialRuntime& runtime) {
    return Apply(state, 0x554u, 0x560u, runtime);
}

}  // namespace pal4
