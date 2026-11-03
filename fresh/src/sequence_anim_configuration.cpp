#include "pal4/sequence_anim_configuration.h"

#include <bit>

namespace pal4 {
namespace {

constexpr std::string_view kTextureRoot = "gamedata\\effect\\tex\\";
constexpr std::string_view kUnsetGbk = "\xCE\xB4\xC9\xE8\xD6\xC3";

void StoreByte(SequenceAnimObjectState& state,
               const std::uint32_t offset, const std::uint8_t value) {
    const auto shift = (offset % 4u) * 8u;
    auto& word = state.words[offset / 4u];
    word = (word & ~(0xffu << shift)) |
           (static_cast<std::uint32_t>(value) << shift);
}

void StoreFloat(SequenceAnimObjectState& state,
                const std::uint32_t offset, const float value) {
    state.words[offset / 4u] = std::bit_cast<std::uint32_t>(value);
}

}  // namespace

bool SequenceAnimConfiguration::Load(
    SequenceAnimObjectState& state, const std::uint32_t document,
    ISequenceAnimConfigurationRuntime& runtime) {
    if (document == 0u)
        return false;

    const auto ri = [&](const std::string_view key, const std::int32_t fallback) {
        return runtime.ReadInt(document, key, fallback);
    };
    const auto rf = [&](const std::string_view key, const float fallback) {
        return runtime.ReadFloat(document, key, fallback);
    };

    const auto fps = ri("Attr_SequenceAnim_FPS", 60);
    const auto texture_name = runtime.ReadString(
        document, "Attr_SequenceAnim_Texture", kUnsetGbk);
    StoreByte(state, 10u, static_cast<std::uint8_t>(
        ri("Attr_Auto_AlphaBlend", 1) > 0));
    const auto axis_billboard = ri("Attr_SequenceAnim_AxisBillbord", 0) > 0;
    StoreByte(state, 8u, static_cast<std::uint8_t>(
        ri("Attr_Render_Last", 1) > 0));

    const auto width = rf("Attr_SequenceAnim_SubTexWidth", 16.0f);
    const auto height = rf("Attr_SequenceAnim_SubTexHeight", 16.0f);
    const auto rotation = rf("Attr_SequenceAnim_Rotate_x", 0.0f);
    const auto scale_width = rf("Attr_SequenceAnim_Scale_SubTexWidth", 1.0f);
    const auto scale_height = rf("Attr_SequenceAnim_Scale_SubTexHeight", 1.0f);
    const auto position_x = rf("Attr_SequenceAnim_Position_X", 0.0f);
    const auto position_y = rf("Attr_SequenceAnim_Position_Y", 0.0f);
    const auto position_z = rf("Attr_SequenceAnim_Position_Z", 0.0f);

    const auto red = static_cast<std::uint8_t>(
        ri("Attr_SequenceAnim_Color_Red", 255));
    const auto green = static_cast<std::uint8_t>(
        ri("Attr_SequenceAnim_Color_Green", 255));
    const auto blue = static_cast<std::uint8_t>(
        ri("Attr_SequenceAnim_Color_Blue", 255));
    const auto alpha = static_cast<std::uint8_t>(
        ri("Attr_SequenceAnim_Color_Alpha", 255));
    const auto color = static_cast<std::uint32_t>(red) |
                       (static_cast<std::uint32_t>(green) << 8u) |
                       (static_cast<std::uint32_t>(blue) << 16u) |
                       (static_cast<std::uint32_t>(alpha) << 24u);
    const auto center = ri("Attr_Center", 1) > 0;

    const std::string texture_path = std::string(kTextureRoot) + texture_name;
    auto texture = runtime.FindTexture(texture_path);
    if (texture == 0u)
        texture = runtime.LoadTexture(texture_path);

    StoreFloat(state, 108u, width);
    StoreFloat(state, 112u, height);
    state.words[20u] = 0u;
    if (texture != 0u) {
        StoreFloat(state, 52u, static_cast<float>(1.0 / static_cast<double>(fps)));
        const auto texture_width = static_cast<float>(runtime.TextureWidth(texture));
        const auto texture_height = static_cast<float>(runtime.TextureHeight(texture));
        StoreFloat(state, 100u, texture_width);
        StoreFloat(state, 104u, texture_height);
        const auto columns = runtime.Ftol(texture_width / width);
        const auto rows = runtime.Ftol(texture_height / height);
        state.words[14u] = static_cast<std::uint32_t>(columns);
        state.words[15u] = static_cast<std::uint32_t>(rows);
        state.words[16u] = 0u;
        state.words[17u] = static_cast<std::uint32_t>(rows - 1);
        const auto u_step = width / texture_width;
        const auto v_step = height / texture_height;
        StoreFloat(state, 72u, u_step);
        StoreFloat(state, 76u, v_step);
        StoreFloat(state, 84u, 0.0f);
        StoreFloat(state, 88u, static_cast<float>(rows - 1) * v_step);
        StoreFloat(state, 92u, u_step);
        StoreFloat(state, 96u,
                   static_cast<float>(rows - 1) * v_step + v_step);
    } else {
        StoreFloat(state, 52u, -1.0f);
        state.words[25u] = 0u;
        state.words[26u] = 0u;
    }

    const auto resource = runtime.AllocateResource(0x34u);
    if (resource != 0u) {
        runtime.WriteResourceByte(resource + 12u, 0u);
        runtime.WriteResourceByte(resource + 13u, 1u);
        runtime.WriteResourceByte(resource + 14u, 0u);
        runtime.WriteResourceByte(resource + 15u, 1u);
        runtime.WriteResourceByte(resource + 16u, 0u);
        runtime.WriteResourceWord(resource + 20u, 0u);
        runtime.WriteResourceWord(resource + 24u, 0u);
        runtime.WriteResourceWord(resource + 48u, 0u);
    }
    state.words[30u] = resource;
    if (runtime.InitializeResource(resource, texture, width, height,
                                   (state.words[2u] >> 16u & 0xffu) != 0u,
                                   axis_billboard, center)) {
        runtime.WriteResourceWord(resource + 24u,
                                  std::bit_cast<std::uint32_t>(rotation));
        runtime.WriteResourceWord(resource + 40u,
                                  std::bit_cast<std::uint32_t>(scale_width));
        runtime.WriteResourceWord(resource + 44u,
                                  std::bit_cast<std::uint32_t>(scale_height));
        runtime.WriteResourceWord(resource + 28u,
                                  std::bit_cast<std::uint32_t>(position_x));
        runtime.WriteResourceWord(resource + 32u,
                                  std::bit_cast<std::uint32_t>(position_y));
        runtime.WriteResourceWord(resource + 36u,
                                  std::bit_cast<std::uint32_t>(position_z));
        runtime.InitializeTransform(resource);
        const auto geometry = runtime.Geometry(resource);
        runtime.SetColor(geometry, color);
        runtime.UpdateGeometry(geometry, {
            state.words[21u], state.words[22u],
            state.words[23u], state.words[24u]});
        StoreByte(state, 4u, 1u);
    } else if (resource != 0u) {
        runtime.DeinitializeResource(resource);
        runtime.FreeResource(resource);
        state.words[30u] = 0u;
    }

    const auto active = (state.words[1u] & 0xffu) != 0u;
    if (active) {
        for (std::uint32_t i = 0u; i < 4u; ++i)
            state.words[31u + i] = state.words[21u + i];
        state.words[35u] = color;
        StoreFloat(state, 144u, rotation);
        StoreFloat(state, 148u, scale_width);
        StoreFloat(state, 152u, scale_height);
        StoreFloat(state, 156u, position_x);
        StoreFloat(state, 160u, position_y);
        StoreFloat(state, 164u, position_z);
    }
    StoreByte(state, 11u, 0u);
    return active;
}

}  // namespace pal4
