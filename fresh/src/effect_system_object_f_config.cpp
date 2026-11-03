#include "pal4/effect_system_object_f_config.h"

#include <array>
#include <bit>
#include <cstdint>
#include <string>

namespace pal4 {
namespace {

constexpr std::string_view kUnsetTexture = "\xCE\xB4\xC9\xE8\xD6\xC3";

void SetStateByte(EffectSystemObjectFState& state,
                  std::uint32_t byte_offset, std::uint8_t value) {
    auto& word = state.words[byte_offset / 4u];
    const auto shift = (byte_offset % 4u) * 8u;
    word = (word & ~(0xffu << shift)) |
           (static_cast<std::uint32_t>(value) << shift);
}

std::uint32_t FloatBits(float value) {
    return std::bit_cast<std::uint32_t>(value);
}

}  // namespace

std::uint8_t EffectSystemObjectF::LoadConfiguration(
    EffectSystemObjectFState& state, const std::uint32_t document,
    IEffectSystemObjectFConfigRuntime& runtime) {
    if (document == 0u)
        return 0u;

    const std::string texture_path =
        std::string("gamedata\\effect\\tex\\") +
        runtime.ReadString(document, "Attr_BillBord_Texture", kUnsetTexture);
    const auto alpha_blend = static_cast<std::uint8_t>(
        runtime.ReadInt(document, "Attr_Auto_AlphaBlend", 1) > 0);
    SetStateByte(state, 10u, alpha_blend);

    const std::array<float, 2> size{
        runtime.ReadFloat(document, "Attr_BillBord_Width", 16.0f),
        runtime.ReadFloat(document, "Attr_BillBord_Height", 16.0f)};
    const auto scale_width = runtime.ReadFloat(
        document, "Attr_BillBord_Scale_Width", 1.0f);
    const auto scale_height = runtime.ReadFloat(
        document, "Attr_BillBord_Scale_Height", 1.0f);
    const auto rotation = runtime.ReadFloat(
        document, "Attr_BillBord_Rotate_x", 0.0f);
    const std::array<float, 3> position{
        runtime.ReadFloat(document, "Attr_BillBord_Position_x", 0.0f),
        runtime.ReadFloat(document, "Attr_BillBord_Position_y", 0.0f),
        runtime.ReadFloat(document, "Attr_BillBord_Position_z", 0.0f)};
    const auto axis = static_cast<std::uint8_t>(
        runtime.ReadInt(document, "Attr_BillBord_AxisBillbord", 0) > 0);

    std::uint32_t rgba = 0u;
    constexpr std::array<std::string_view, 4> color_keys{
        "Attr_BillBord_Color_Red", "Attr_BillBord_Color_Green",
        "Attr_BillBord_Color_Blue", "Attr_BillBord_Color_Alpha"};
    for (std::uint32_t index = 0u; index < color_keys.size(); ++index) {
        const auto component = static_cast<std::uint8_t>(
            runtime.ReadInt(document, color_keys[index], 255));
        rgba |= static_cast<std::uint32_t>(component) << (8u * index);
    }
    const auto render_last = static_cast<std::uint8_t>(
        runtime.ReadInt(document, "Attr_Render_Last", 1) > 0);
    SetStateByte(state, 8u, render_last);
    const auto center = static_cast<std::uint8_t>(
        runtime.ReadInt(document, "Attr_Center", 1) > 0);

    auto texture = runtime.FindTexture(texture_path);
    if (texture == 0u)
        texture = runtime.LoadTexture(texture_path);

    try {
        const auto object = runtime.AllocateObject(0x34u);
        if (object != 0u) {
            runtime.WriteByte(object + 12u, 0u);
            runtime.WriteByte(object + 13u, 1u);
            runtime.WriteByte(object + 14u, 0u);
            runtime.WriteByte(object + 15u, 1u);
            runtime.WriteByte(object + 16u, 0u);
            runtime.WriteWord(object + 20u, 0u);
            runtime.WriteWord(object + 24u, 0u);
            runtime.WriteWord(object + 48u, 0u);
        }
        state.words[29u] = object;
        if (runtime.InitializeObject(object, texture, size,
                                     alpha_blend, axis, center)) {
            runtime.SetColor(runtime.ReadWord(object + 48u), rgba);
            for (std::uint32_t i = 0u; i < position.size(); ++i)
                runtime.WriteWord(object + 28u + 4u * i,
                                  FloatBits(position[i]));
            runtime.InitializeTransform(object);
            runtime.WriteWord(object + 24u, FloatBits(rotation));
            runtime.WriteWord(object + 40u, FloatBits(scale_width));
            runtime.WriteWord(object + 44u, FloatBits(scale_height));
            runtime.AssignDocument(state, document);
            state.words[31u] = rgba;
            state.words[32u] = FloatBits(rotation);
            state.words[33u] = FloatBits(scale_width);
            state.words[34u] = FloatBits(scale_height);
            for (std::uint32_t i = 0u; i < position.size(); ++i)
                state.words[35u + i] = FloatBits(position[i]);
            SetStateByte(state, 4u, 1u);
        }
    } catch (...) {
        if (texture != 0u)
            runtime.DestroyTexture(texture);
        throw;
    }
    if (texture != 0u)
        runtime.DestroyTexture(texture);
    SetStateByte(state, 11u, 0u);
    return static_cast<std::uint8_t>(state.words[1u]);
}

std::uint8_t EffectSystemObjectF::ReapplyCachedConfiguration(
    EffectSystemObjectFState& state,
    IEffectSystemObjectFConfigRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return 0u;

    const auto object = state.words[29u];
    runtime.SetColor(runtime.ReadWord(object + 48u), state.words[31u]);
    runtime.WriteWord(object + 24u, state.words[32u]);
    runtime.WriteWord(object + 40u, state.words[33u]);
    runtime.WriteWord(object + 44u, state.words[34u]);
    for (std::uint32_t i = 0u; i < 3u; ++i)
        runtime.WriteWord(object + 28u + 4u * i, state.words[35u + i]);
    runtime.InitializeTransform(object);
    SetStateByte(state, 11u, 0u);
    return 1u;
}

}  // namespace pal4
