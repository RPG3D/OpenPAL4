#include "pal4/cylinder_object_config.h"

#include <bit>
#include <string>
#include <utility>

namespace pal4 {
namespace {

constexpr std::string_view kTexturePrefix = "gamedata\\effect\\tex\\";
constexpr std::uint32_t kTextureDefaultAddress = 0x8e425cu;

void WriteByte(CylinderObjectState& state,
               const std::uint32_t offset,
               const std::uint8_t value) {
    const auto shift = (offset % 4u) * 8u;
    const auto mask = 0xffu << shift;
    auto& word = state.words[offset / 4u];
    word = (word & ~mask) |
           (static_cast<std::uint32_t>(value) << shift);
}

std::uint8_t ReadByte(const CylinderObjectState& state,
                      const std::uint32_t offset) {
    return static_cast<std::uint8_t>(
        state.words[offset / 4u] >> (8u * (offset % 4u)));
}

std::array<float, 3> ReadVector(
    const std::uint32_t document,
    const std::array<std::string_view, 3>& names,
    const float default_value,
    ICylinderObjectConfigRuntime& runtime) {
    std::array<float, 3> result{};
    for (std::uint32_t i = 0u; i < 3u; ++i)
        result[i] = runtime.ReadFloat(document, names[i], default_value);
    return result;
}

void StoreVector(CylinderObjectState& state,
                 const std::uint32_t offset,
                 const std::array<float, 3>& value) {
    for (std::uint32_t i = 0u; i < 3u; ++i)
        state.words[offset / 4u + i] = std::bit_cast<std::uint32_t>(value[i]);
}

std::array<float, 3> CachedVector(
    const CylinderObjectState& state, const std::uint32_t offset) {
    return {std::bit_cast<float>(state.words[offset / 4u]),
            std::bit_cast<float>(state.words[offset / 4u + 1u]),
            std::bit_cast<float>(state.words[offset / 4u + 2u])};
}

}  // namespace

std::uint8_t CylinderObjectConfiguration::Load(
    CylinderObjectState& state, const std::uint32_t document,
    ICylinderObjectConfigRuntime& runtime) {
    if (document == 0u)
        return 0u;

    std::string path(kTexturePrefix);
    path += runtime.ReadTextureName(document, "Attr_Cylinder_Texture",
                                    kTextureDefaultAddress);
    try {
        CylinderVisualParameters parameters{};
        parameters.auto_alpha_blend = runtime.ReadInt(
            document, "Attr_Auto_AlphaBlend", 1) > 0;
        WriteByte(state, 10u, parameters.auto_alpha_blend ? 1u : 0u);
        parameters.border_count = runtime.ReadInt(
            document, "Attr_Cylinder_BorderNumb", 5);
        parameters.height = runtime.ReadFloat(
            document, "Attr_Cylinder_Height", 5.0f);
        parameters.bottom_radius = runtime.ReadFloat(
            document, "Attr_Cylinder_Bottom_Raidus", 6.0f);
        parameters.top_radius = runtime.ReadFloat(
            document, "Attr_Cylinder_Top_Raidus", 3.0f);
        parameters.angle = runtime.ReadInt(
            document, "Attr_Cylinder_Angel", 360);
        parameters.rasterize = runtime.ReadInt(
            document, "Attr_Cylinder_Asterialize", 1) > 0;
        parameters.shrink_size = runtime.ReadFloat(
            document, "Attr_Cylinder_Shrink_Size", 2.0f);

        std::uint32_t color = 0u;
        for (const auto& [index, key] : {
                 std::pair{0u, "Attr_Cylinder_Vertex_Col_Red"},
                 std::pair{1u, "Attr_Cylinder_Vertex_Col_Green"},
                 std::pair{2u, "Attr_Cylinder_Vertex_Col_Blue"},
                 std::pair{3u, "Attr_Cylinder_Vertex_Col_Alpha"}}) {
            color |= static_cast<std::uint32_t>(
                         static_cast<std::uint8_t>(
                             runtime.ReadInt(document, key, 255))) <<
                     (8u * index);
        }
        parameters.flip_uv = runtime.ReadInt(
            document, "Attr_Cylinder_FlipUV", 0) > 0;
        parameters.center = runtime.ReadInt(
            document, "Attr_Cylinder_Center", 1) > 0;

        const auto rotation = ReadVector(document, {
            "Attr_Cylinder_Rotate_x", "Attr_Cylinder_Rotate_y",
            "Attr_Cylinder_Rotate_z"}, 0.0f, runtime);
        const auto scale = ReadVector(document, {
            "Attr_Cylinder_Scale_x", "Attr_Cylinder_Scale_y",
            "Attr_Cylinder_Scale_z"}, 1.0f, runtime);
        const auto position = ReadVector(document, {
            "Attr_Cylinder_Position_x", "Attr_Cylinder_Position_y",
            "Attr_Cylinder_Position_z"}, 0.0f, runtime);

        auto texture = runtime.FindTexture(path);
        if (texture == 0u)
            texture = runtime.LoadTexture(path);
        const auto model = runtime.AllocateModel(0x34u);
        if (model != 0u)
            runtime.InitializeModel(model);
        state.words[22u] = model;
        if (runtime.CreateVisualModel(model, texture, parameters)) {
            runtime.SetPosition(model, position, 0u);
            runtime.SetRotation(model, rotation, 0u);
            runtime.SetScale(model, scale, 0u);
            runtime.SetColor(model, color);
            WriteByte(state, 4u, 1u);
        }

        if (ReadByte(state, 4u) != 0u) {
            state.words[23u] = runtime.CopyDocument(document);
        } else if (model != 0u) {
            runtime.DestroyModel(model);
            runtime.FreeModel(model);
            state.words[22u] = 0u;
        }
        state.words[25u] = color;
        WriteByte(state, 11u, 0u);
        StoreVector(state, 104u, rotation);
        StoreVector(state, 116u, scale);
        StoreVector(state, 128u, position);
        runtime.ReleaseTexturePath(path);
        return ReadByte(state, 4u);
    } catch (...) {
        runtime.ReleaseTexturePath(path);
        throw;
    }
}

std::uint8_t CylinderObjectConfiguration::Reset(
    CylinderObjectState& state, ICylinderObjectConfigRuntime& runtime) {
    if (ReadByte(state, 4u) == 0u)
        return 0u;
    const auto model = state.words[22u];
    runtime.SetColor(model, state.words[25u]);
    runtime.SetRotation(model, CachedVector(state, 104u), 0u);
    runtime.SetScale(model, CachedVector(state, 116u), 0u);
    runtime.SetPosition(model, CachedVector(state, 128u), 0u);
    WriteByte(state, 11u, 0u);
    return 1u;
}

}  // namespace pal4
