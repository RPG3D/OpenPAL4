#include "pal4/thunder_configuration.h"

#include <array>
#include <bit>

namespace pal4 {
namespace {

constexpr std::string_view kTexturePrefix = "gamedata\\effect\\tex\\";

void StoreByte(ThunderObjectState& state, const std::uint32_t offset,
               const std::uint8_t value) {
    const auto shift = (offset % 4u) * 8u;
    auto& word = state.words[offset / 4u];
    word = (word & ~(0xffu << shift)) |
           (static_cast<std::uint32_t>(value) << shift);
}

void StoreFloats(ThunderObjectState& state, const std::uint32_t offset,
                 const std::array<float, 3u>& values) {
    for (std::uint32_t i = 0u; i < 3u; ++i)
        state.words[offset / 4u + i] = std::bit_cast<std::uint32_t>(values[i]);
}

void CacheWords(ThunderObjectState& state, const std::uint32_t source,
                const std::uint32_t destination, const std::uint32_t count) {
    for (std::uint32_t i = 0u; i < count; ++i)
        state.words[destination / 4u + i] = state.words[source / 4u + i];
}

std::array<float, 3u> ReadTriple(
    const std::uint32_t document,
    const std::array<std::string_view, 3u>& names,
    const float fallback, IThunderConfigurationRuntime& runtime) {
    std::array<float, 3u> values{};
    for (std::uint32_t i = 0u; i < values.size(); ++i)
        values[i] = runtime.ReadFloat(document, names[i], fallback);
    return values;
}

}  // namespace

std::uint8_t ThunderConfiguration::Load(
    ThunderObjectState& state, const std::uint32_t document,
    IThunderConfigurationRuntime& runtime) {
    if (document == 0u)
        return 0u;

    std::string path(kTexturePrefix);
    std::uint32_t texture = 0u;
    try {
        runtime.SetTargetName(state, "Default");
        path += runtime.ReadString(document, "Attr_Thunder_Texture", "");
        const auto target_name = runtime.ReadString(
            document, "Attr_Thunder_Target_Name", "");
        runtime.SetTargetName(state, target_name);

        const bool alpha_blend = runtime.ReadInt(
            document, "Attr_Thunder_AlphaBlend", 0) > 0;
        const bool local_model = runtime.ReadInt(
            document, "Attr_Thunder_Local_Model", 0) > 0;
        StoreByte(state, 232u, static_cast<std::uint8_t>(local_model));
        const bool lock_target = runtime.ReadInt(
            document, "Attr_Thunder_Lock_Target", 0) > 0;
        const bool size_gradual = runtime.ReadInt(
            document, "Attr_Thunder_Size_Gradual", 0) > 0;
        const bool range_gradual = runtime.ReadInt(
            document, "Attr_Thunder_Range_Gradual", 0) > 0;
        const bool move_target = runtime.ReadInt(
            document, "Attr_Thunder_Move_Target", 0) > 0;
        StoreByte(state, 56u, static_cast<std::uint8_t>(move_target));

        auto target = ReadTriple(document, {
            "Attr_Thunder_Target_x", "Attr_Thunder_Target_y",
            "Attr_Thunder_Target_z"}, 0.0f, runtime);
        if (!local_model && state.words[3u] != 0u)
            runtime.ResolveTargetPosition(state.words[3u], target_name, target);

        const auto fps = runtime.ReadFloat(document, "Attr_Thunder_Fps", 0.0f);
        const auto min_range = runtime.ReadFloat(
            document, "Attr_Thunder_MinRange", 0.0f);
        const auto max_range = runtime.ReadFloat(
            document, "Attr_Thunder_MaxRange", 0.0f);
        const auto section = std::array<float, 4u>{
            runtime.ReadFloat(document, "Attr_Thunder_Section_Min_Width", 0.0f),
            runtime.ReadFloat(document, "Attr_Thunder_Section_Max_Width", 0.0f),
            runtime.ReadFloat(document, "Attr_Thunder_Section_Min_Height", 0.0f),
            runtime.ReadFloat(document, "Attr_Thunder_Section_Max_Height", 0.0f)};
        const auto position = ReadTriple(document, {
            "Attr_Thunder_Position_x", "Attr_Thunder_Position_y",
            "Attr_Thunder_Position_z"}, 0.0f, runtime);
        const auto scale = ReadTriple(document, {
            "Attr_Thunder_Scale_x", "Attr_Thunder_Scale_y",
            "Attr_Thunder_Scale_z"}, 1.0f, runtime);
        const auto rotation = ReadTriple(document, {
            "Attr_Thunder_Rotate_x", "Attr_Thunder_Rotate_y",
            "Attr_Thunder_Rotate_z"}, 0.0f, runtime);

        constexpr std::array<std::string_view, 4u> kColorNames{
            "Attr_Thunder_Color_Red", "Attr_Thunder_Color_Green",
            "Attr_Thunder_Color_Blue", "Attr_Thunder_Color_Alpha"};
        std::uint32_t color = 0u;
        for (std::uint32_t i = 0u; i < kColorNames.size(); ++i) {
            const auto fallback = i == 3u ? 180 : 255;
            const auto value = static_cast<std::uint8_t>(
                runtime.ReadInt(document, kColorNames[i], fallback));
            color |= static_cast<std::uint32_t>(value) << (i * 8u);
        }
        const bool sample = runtime.ReadInt(
            document, "Attr_Thunder_Sample", 0) > 0;
        const auto sample_coefficient = runtime.ReadInt(
            document, "Attr_Thunder_Sample_Coeff", 4);

        texture = runtime.FindTexture(path);
        if (texture == 0u)
            texture = runtime.LoadTexture(path);
        const auto initialized = runtime.InitializeVisual(state, texture, fps, 0u);
        if (initialized) {
            StoreFloats(state, 72u, scale);
            runtime.UpdateFrame(state);
            StoreFloats(state, 84u, rotation);
            runtime.UpdateFrame(state);
            StoreFloats(state, 96u, position);
            runtime.UpdateFrame(state);

            state.words[27u] = color;
            StoreByte(state, 61u, static_cast<std::uint8_t>(alpha_blend));
            StoreByte(state, 62u, static_cast<std::uint8_t>(local_model));
            StoreFloats(state, 148u, target);
            state.words[40u] = std::bit_cast<std::uint32_t>(min_range);
            state.words[41u] = std::bit_cast<std::uint32_t>(max_range);
            for (std::uint32_t i = 0u; i < section.size(); ++i)
                state.words[42u + i] = std::bit_cast<std::uint32_t>(section[i]);
            StoreByte(state, 63u, static_cast<std::uint8_t>(lock_target));
            StoreByte(state, 64u, static_cast<std::uint8_t>(size_gradual));
            StoreByte(state, 65u, static_cast<std::uint8_t>(range_gradual));
            StoreByte(state, 66u, static_cast<std::uint8_t>(sample));
            state.words[17u] = static_cast<std::uint32_t>(sample_coefficient);

            if (texture != 0u) {
                const auto released = texture;
                texture = 0u;
                runtime.DestroyTexture(released);
            }
            StoreByte(state, 4u, 1u);
            StoreByte(state, 11u, 0u);
            state.words[60u] = color;
            CacheWords(state, 72u, 244u, 9u);
            CacheWords(state, 148u, 280u, 3u);
        } else if (texture != 0u) {
            const auto released = texture;
            texture = 0u;
            runtime.DestroyTexture(released);
        }
        runtime.ReleaseTexturePath(path);
        return 1u;
    } catch (...) {
        if (texture != 0u)
            runtime.DestroyTexture(texture);
        runtime.ReleaseTexturePath(path);
        throw;
    }
}

}  // namespace pal4
