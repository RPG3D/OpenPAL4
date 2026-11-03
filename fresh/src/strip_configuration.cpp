#include "pal4/strip_configuration.h"

#include <array>
#include <bit>
#include <string>
#include <utility>

namespace pal4 {
namespace {

constexpr std::string_view kTexturePrefix = "gamedata\\effect\\tex\\";
constexpr std::uint32_t kTextureDefaultAddress = 0x8e425cu;

void WriteByte(StripObjectState& state, const std::uint32_t offset,
               const std::uint8_t value) {
    const auto shift = (offset % 4u) * 8u;
    auto& word = state.words[offset / 4u];
    word = (word & ~(0xffu << shift)) |
           (static_cast<std::uint32_t>(value) << shift);
}

void WriteFloat(StripObjectState& state, const std::uint32_t offset,
                const float value) {
    state.words[offset / 4u] = std::bit_cast<std::uint32_t>(value);
}

void CacheTriple(StripObjectState& state, const std::uint32_t from,
                 const std::uint32_t to) {
    for (std::uint32_t i = 0u; i < 3u; ++i)
        state.words[to / 4u + i] = state.words[from / 4u + i];
}

}  // namespace

std::uint8_t StripConfiguration::Load(
    StripObjectState& state, const std::uint32_t document,
    IStripConfigurationRuntime& runtime) {
    if (document == 0u)
        return 0u;

    std::string path(kTexturePrefix);
    path += runtime.ReadTextureName(document, "Attr_Strip_Texture",
                                    kTextureDefaultAddress);
    std::uint32_t texture = 0u;
    try {
        StripVisualParameters parameters{};
        parameters.width = runtime.ReadFloat(document, "Attr_Strip_Wdith", 0.0f);
        parameters.height = runtime.ReadFloat(document, "Attr_Strip_Height", 0.0f);
        parameters.flip_uv = runtime.ReadInt(document, "Attr_Strip_FlipUV", 0) > 0;
        parameters.alpha_blend = runtime.ReadInt(
            document, "Attr_Auto_AlphaBlend", 1) > 0;
        WriteByte(state, 10u, parameters.alpha_blend ? 1u : 0u);

        for (std::uint32_t i = 0u; i < 3u; ++i) {
            constexpr std::array<std::string_view, 3> names{
                "Attr_Strip_Position_x", "Attr_Strip_Position_y",
                "Attr_Strip_Position_z"};
            WriteFloat(state, 88u + 4u * i,
                       runtime.ReadFloat(document, names[i], 0.0f));
        }
        for (std::uint32_t i = 0u; i < 3u; ++i) {
            constexpr std::array<std::string_view, 3> names{
                "Attr_Strip_Scale_x", "Attr_Strip_Scale_y",
                "Attr_Strip_Scale_z"};
            WriteFloat(state, 64u + 4u * i,
                       runtime.ReadFloat(document, names[i], 0.0f));
        }
        for (std::uint32_t i = 0u; i < 3u; ++i) {
            constexpr std::array<std::string_view, 3> names{
                "Attr_Strip_Rotate_x", "Attr_Strip_Rotate_y",
                "Attr_Strip_Rotate_z"};
            WriteFloat(state, 76u + 4u * i,
                       runtime.ReadFloat(document, names[i], 0.0f));
        }
        constexpr std::array<std::string_view, 4> color_names{
            "Attr_Strip_Vertex_Col_Red", "Attr_Strip_Vertex_Col_Green",
            "Attr_Strip_Vertex_Col_Blue", "Attr_Strip_Vertex_Col_Alpha"};
        for (std::uint32_t i = 0u; i < color_names.size(); ++i)
            WriteByte(state, 100u + i, static_cast<std::uint8_t>(
                runtime.ReadInt(document, color_names[i], 0)));

        parameters.cylinder = runtime.ReadInt(
            document, "Attr_Strip_Cylinder_Creator", 1) > 0;
        parameters.cylinder_borders = runtime.ReadInt(
            document, "Attr_Strip_Cylinder_Border_Numb", 10);
        parameters.cylinder_top_radius = runtime.ReadFloat(
            document, "Attr_Strip_Cylinder_TRadius", 5.0f);
        parameters.cylinder_bottom_radius = runtime.ReadFloat(
            document, "Attr_Strip_Cylinder_BRadius", 10.0f);
        parameters.cylinder_loops = runtime.ReadInt(
            document, "Attr_Strip_Cylinder_Loop_Numb", 1);
        parameters.straight = runtime.ReadInt(
            document, "Attr_Strip_Straight_Creator", 0) > 0;
        parameters.straight_faces = runtime.ReadInt(
            document, "Attr_Strip_Straight_Face_Numb", 1);
        parameters.sequence = runtime.ReadFloat(
            document, "Attr_Strip_Straight_Sequence", 0.0f) > 0.0f;
        parameters.sequence_width = runtime.ReadFloat(
            document, "Attr_Strip_Straight_Seq_Width", 16.0f);
        parameters.sequence_height = runtime.ReadFloat(
            document, "Attr_Strip_Straight_Seq_Height", 16.0f);
        parameters.sequence_fps = runtime.ReadFloat(
            document, "Attr_Strip_Straight_Seq_Fps", 30.0f);
        parameters.center = runtime.ReadInt(document, "Attr_Center", 1) > 0;
        parameters.render_last = runtime.ReadInt(
            document, "Attr_Render_Last", 1) > 0;
        WriteByte(state, 8u, parameters.render_last ? 1u : 0u);
        parameters.render_state = runtime.ReadInt(
            document, "Attr_Strip_RS", 0) > 0;
        WriteByte(state, 60u, parameters.render_state ? 1u : 0u);
        parameters.cull_mode = runtime.ReadInt(
            document, "Attr_Strip_Cullmode", 0);

        texture = runtime.FindTexture(path);
        if (texture == 0u)
            texture = runtime.LoadTexture(path);
        WriteFloat(state, 220u, parameters.sequence_width);
        WriteFloat(state, 224u, parameters.sequence_height);
        state.words[48u] = 0u;
        if (texture == 0u) {
            runtime.ReleaseTexturePath(path);
            return 0u;
        }

        if (parameters.sequence && parameters.straight) {
            WriteFloat(state, 164u, 1.0f / parameters.sequence_fps);
            std::int32_t texture_width = 0;
            std::int32_t texture_height = 0;
            runtime.TextureSize(texture, texture_width, texture_height);
            WriteFloat(state, 212u, static_cast<float>(texture_width));
            WriteFloat(state, 216u, static_cast<float>(texture_height));
            const auto columns = runtime.FloatToInt(
                static_cast<float>(texture_width) / parameters.sequence_width);
            const auto rows = runtime.FloatToInt(
                static_cast<float>(texture_height) / parameters.sequence_height);
            state.words[42u] = static_cast<std::uint32_t>(columns);
            state.words[43u] = static_cast<std::uint32_t>(rows);
            state.words[44u] = 0u;
            state.words[45u] = static_cast<std::uint32_t>(rows - 1);
            WriteByte(state, 160u, 1u);
            const auto tile_u = parameters.sequence_width /
                                static_cast<float>(texture_width);
            const auto tile_v = parameters.sequence_height /
                                static_cast<float>(texture_height);
            WriteFloat(state, 184u, tile_u);
            WriteFloat(state, 188u, tile_v);
            WriteFloat(state, 196u, tile_u * 0.0f);
            WriteFloat(state, 200u, static_cast<float>(rows - 1) * tile_v);
            WriteFloat(state, 204u, tile_u * 0.0f + tile_u);
            WriteFloat(state, 208u, static_cast<float>(rows) * tile_v);
        } else {
            WriteByte(state, 160u, 0u);
        }

        if (parameters.cylinder) {
            WriteByte(state, 140u, 0u);
            runtime.CreateCylinder(state, texture, parameters);
            if ((state.words[25u] >> 24u) == 0xffu)
                WriteByte(state, 103u, 0xfeu);
        } else if (parameters.straight) {
            WriteByte(state, 140u, 1u);
            runtime.CreateStraight(state, texture, parameters);
            if ((state.words[25u] >> 24u) == 0xffu)
                WriteByte(state, 103u, 0xfeu);
        }
        runtime.ReleaseTexture(texture);
        texture = 0u;
        state.words[39u] = runtime.CopyDocument(document);
        state.words[57u] = state.words[25u];
        CacheTriple(state, 76u, 244u);
        CacheTriple(state, 64u, 232u);
        CacheTriple(state, 88u, 256u);
        for (std::uint32_t i = 0u; i < 4u; ++i)
            state.words[67u + i] = state.words[49u + i];
        WriteByte(state, 4u, 1u);
        WriteByte(state, 128u, 0u);
        WriteByte(state, 11u, 0u);
        runtime.ReleaseTexturePath(path);
        return 1u;
    } catch (...) {
        if (texture != 0u)
            runtime.ReleaseTexture(texture);
        runtime.ReleaseTexturePath(path);
        throw;
    }
}

}  // namespace pal4
