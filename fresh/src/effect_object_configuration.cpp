#include "pal4/effect_object_configuration.h"

#include <array>
#include <bit>

namespace pal4 {
namespace {

struct FloatField final {
    std::string_view key;
    std::uint32_t offset;
    float default_value;
};

constexpr std::array kFields{
    FloatField{"Attr_EffectObject_Position_x", 76u, 0.0f},
    FloatField{"Attr_EffectObject_Position_y", 80u, 0.0f},
    FloatField{"Attr_EffectObject_Position_z", 84u, 0.0f},
    FloatField{"Attr_EffectObject_Scale_x", 52u, 1.0f},
    FloatField{"Attr_EffectObject_Scale_y", 56u, 1.0f},
    FloatField{"Attr_EffectObject_Scale_z", 60u, 1.0f},
    FloatField{"Attr_EffectObject_Rotate_x", 64u, 0.0f},
    FloatField{"Attr_EffectObject_Rotate_y", 68u, 0.0f},
    FloatField{"Attr_EffectObject_Rotate_z", 72u, 0.0f}};

void WriteByte(EffectObjectState& state, const std::uint32_t offset,
               const std::uint8_t value) {
    const auto shift = (offset % 4u) * 8u;
    const auto mask = 0xffu << shift;
    auto& word = state.words[offset / 4u];
    word = (word & ~mask) | (static_cast<std::uint32_t>(value) << shift);
}

}  // namespace

std::uint8_t EffectObjectConfiguration::Load(
    EffectObjectState& state, const std::uint32_t document,
    const std::uint32_t context,
    IEffectObjectConfigurationRuntime& runtime) {
    if (document == 0u)
        return 0u;

    for (const auto& field : kFields)
        state.words[field.offset / 4u] = std::bit_cast<std::uint32_t>(
            runtime.ReadFloat(document, field.key, field.default_value));

    const auto child = runtime.GetChildByIndex(document, 0u);
    const auto instance = runtime.CreateInstance();
    state.words[23u] = instance;
    if (child == 0u || !runtime.InitializeInstance(instance, child)) {
        runtime.CleanupInstance(instance, context);
        runtime.DestroyInstance(instance, context);
        state.words[23u] = 0u;
        return 0u;
    }

    runtime.WriteWord(instance + 12u, state.words[4u]);
    if (!runtime.UpdateInstance(instance)) {
        runtime.CleanupInstance(instance, context);
        runtime.DestroyInstance(instance, context);
        state.words[23u] = 0u;
        return 0u;
    }

    for (std::uint32_t i = 0u; i != 3u; ++i) {
        state.words[33u + i] = state.words[13u + i];
        state.words[36u + i] = state.words[16u + i];
        state.words[39u + i] = state.words[19u + i];
    }
    state.words[24u] = runtime.CreateFrame();
    runtime.UpdateFrameProperties(state);
    if (state.words[3u] != 0u)
        runtime.WriteByte(instance + 83u,
                          runtime.ReadByte(state.words[3u] + 83u));
    WriteByte(state, 11u, 0u);
    WriteByte(state, 4u, 1u);
    return 1u;
}

}  // namespace pal4
