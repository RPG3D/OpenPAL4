#include "pal4/dynamic_strip_object_config.h"

#include <array>
#include <bit>
#include <string>

namespace pal4 {
namespace {

constexpr std::string_view kTexturePrefix = "gamedata\\effect\\tex\\";
constexpr std::uint32_t kTextureDefaultAddress = 0x8e425cu;

void WriteByte(DynamicStripObjectState& state,
               const std::uint32_t offset,
               const std::uint8_t value) {
    const auto shift = (offset % 4u) * 8u;
    const auto mask = 0xffu << shift;
    auto& word = state.words[offset / 4u];
    word = (word & ~mask) |
           (static_cast<std::uint32_t>(value) << shift);
}

std::array<float, 3> ReadVector(
    const std::uint32_t document,
    const std::array<std::string_view, 3>& names,
    const float default_value,
    IDynamicStripObjectConfigRuntime& runtime) {
    std::array<float, 3> result{};
    for (std::uint32_t i = 0u; i < result.size(); ++i)
        result[i] = runtime.ReadFloat(document, names[i], default_value);
    return result;
}

void StoreVector(DynamicStripObjectState& state,
                 const std::uint32_t offset,
                 const std::array<float, 3>& value) {
    for (std::uint32_t i = 0u; i < value.size(); ++i)
        state.words[offset / 4u + i] = std::bit_cast<std::uint32_t>(value[i]);
}

}  // namespace

std::uint8_t DynamicStripObjectConfiguration::Load(
    DynamicStripObjectState& state, const std::uint32_t document,
    IDynamicStripObjectConfigRuntime& runtime) {
    if (document == 0u)
        return 0u;

    std::string path(kTexturePrefix);
    path += runtime.ReadTextureName(document, "Attr_StripSystem_Texture",
                                    kTextureDefaultAddress);
    try {
        const auto alpha_blend = runtime.ReadInt(
            document, "Attr_StripSystem_AlphaBlend", 0) > 0;
        const auto life = runtime.ReadFloat(
            document, "Attr_StripSystem_Life", 0.0f);
        const auto width = runtime.ReadFloat(
            document, "Attr_StripSystem_Width", 0.0f);
        const auto position = ReadVector(document, {
            "Attr_StripSystem_Position_x", "Attr_StripSystem_Position_y",
            "Attr_StripSystem_Position_z"}, 0.0f, runtime);
        const auto scale = ReadVector(document, {
            "Attr_StripSystem_Scale_x", "Attr_StripSystem_Scale_y",
            "Attr_StripSystem_Scale_z"}, 1.0f, runtime);
        const auto rotation = ReadVector(document, {
            "Attr_StripSystem_Rotate_x", "Attr_StripSystem_Rotate_y",
            "Attr_StripSystem_Rotate_z"}, 0.0f, runtime);
        std::uint32_t color = 0u;
        constexpr std::array<std::string_view, 4> color_names{
            "Attr_StripSystem_Color_Red",
            "Attr_StripSystem_Color_Green",
            "Attr_StripSystem_Color_Blue",
            "Attr_StripSystem_Color_Alpha"};
        for (std::uint32_t i = 0u; i < color_names.size(); ++i) {
            color |= static_cast<std::uint32_t>(static_cast<std::uint8_t>(
                         runtime.ReadInt(document, color_names[i], 255)))
                     << (8u * i);
        }
        const auto fps = static_cast<std::uint32_t>(runtime.ReadInt(
            document, "Attr_StripSystem_Fps", 0));
        const auto sample = runtime.ReadInt(
            document, "Attr_StripSystem_Sample", 0) > 0;

        auto texture = runtime.FindTexture(path);
        if (texture == 0u)
            texture = runtime.LoadTexture(path);
        StoreVector(state, 68u, scale);
        runtime.UpdateFrameProperties(state);
        StoreVector(state, 80u, rotation);
        runtime.UpdateFrameProperties(state);
        StoreVector(state, 92u, position);
        runtime.UpdateFrameProperties(state);
        state.words[26u] = color;
        WriteByte(state, 53u, alpha_blend ? 1u : 0u);
        state.words[16u] = 0u;
        state.words[15u] = fps == 0u ? 0u :
            std::bit_cast<std::uint32_t>(
                static_cast<float>(1.0 / static_cast<double>(fps)));
        WriteByte(state, 56u, sample ? 1u : 0u);

        const auto setup = runtime.VisualSetup(state, texture, width, life);
        if (texture != 0u)
            runtime.ReleaseTexture(texture);
        if (setup) {
            state.words[47u] = color;
            StoreVector(state, 192u, rotation);
            StoreVector(state, 204u, scale);
            StoreVector(state, 216u, position);
            WriteByte(state, 11u, 0u);
            WriteByte(state, 4u, 1u);
        }
        runtime.ReleaseTexturePath(path);
        return 1u;
    } catch (...) {
        runtime.ReleaseTexturePath(path);
        throw;
    }
}

std::uint8_t DynamicStripObjectConfiguration::Reset(
    DynamicStripObjectState& state,
    IDynamicStripObjectConfigRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return 0u;
    state.words[26u] = state.words[47u];
    for (std::uint32_t i = 0u; i < 3u; ++i)
        state.words[20u + i] = state.words[48u + i];
    runtime.UpdateFrameProperties(state);
    for (std::uint32_t i = 0u; i < 3u; ++i)
        state.words[17u + i] = state.words[51u + i];
    runtime.UpdateFrameProperties(state);
    for (std::uint32_t i = 0u; i < 3u; ++i)
        state.words[23u + i] = state.words[54u + i];
    runtime.UpdateFrameProperties(state);
    WriteByte(state, 11u, 0u);
    runtime.CleanupCollection(state);
    return 1u;
}

}  // namespace pal4
