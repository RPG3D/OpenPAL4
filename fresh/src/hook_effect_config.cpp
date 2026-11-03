#include "pal4/hook_effect.h"

#include <array>
#include <string>

namespace pal4 {
namespace {

struct FloatField {
    std::string_view name;
    std::uint32_t offset;
    float fallback;
};

template <std::size_t N>
void ReadFloatFields(std::uint32_t config, std::uint32_t object,
                     const std::array<FloatField, N>& fields,
                     IHookEffectConfigRuntime& runtime) {
    for (const auto& field : fields)
        runtime.WriteFloat(object + field.offset,
                           runtime.ReadFloat(config, field.name,
                                             field.fallback));
}

constexpr auto kEmissionFields = std::to_array<FloatField>({
    {"Attr_Prt_EmitGap", 268u, 0.0f},
    {"Attr_Prt_EmitGap_Bias", 272u, 0.0f},
});
constexpr auto kInitialFields = std::to_array<FloatField>({
    {"Attr_PrtSize_x", 228u, 4.0f},
    {"Attr_PrtSize_y", 232u, 4.0f},
    {"Attr_Prt_2DRotate", 236u, 0.0f},
    {"Attr_Prt_PrtLife", 276u, 0.0f},
    {"Attr_Prt_PrtLife_Bias", 280u, 0.0f},
    {"Attr_Prt_InitVel", 284u, 0.0f},
    {"Attr_Prt_initVel_Bias", 288u, 0.0f},
    {"Attr_Prt_InitDir_x", 292u, 0.0f},
    {"Attr_Prt_InitDir_y", 296u, 0.0f},
    {"Attr_Prt_InitDir_z", 300u, 0.0f},
    {"Attr_Prt_InitDir_Bias_x", 304u, 0.0f},
    {"Attr_Prt_InitDir_Bias_y", 308u, 0.0f},
    {"Attr_Prt_InitDir_Bias_z", 312u, 0.0f},
    {"Attr_Prt_Pos_Bias_x", 76u, 0.0f},
    {"Attr_Prt_Pos_Bias_y", 80u, 0.0f},
    {"Attr_Prt_Pos_Bias_z", 84u, 0.0f},
});
constexpr auto kGravityFields = std::to_array<FloatField>({
    {"Attr_Prt_Gravitate", 244u, 0.0f},
    {"Attr_Prt_Gravitate_Pos_x", 248u, 0.0f},
    {"Attr_Prt_Gravitate_Pos_y", 252u, 0.0f},
    {"Attr_Prt_Gravitate_Pos_z", 256u, 0.0f},
});
constexpr auto kSizeFields = std::to_array<FloatField>({
    {"Attr_Prt_StartSize_x", 388u, 0.0f},
    {"Attr_Prt_StartSize_y", 392u, 0.0f},
    {"Attr_Prt_StartSizeBias_x", 396u, 0.0f},
    {"Attr_Prt_StartSizeBias_y", 400u, 0.0f},
    {"Attr_Prt_EndSize_x", 404u, 0.0f},
    {"Attr_Prt_EndSize_y", 408u, 0.0f},
    {"Attr_Prt_EndSizeBias_x", 412u, 0.0f},
    {"Attr_Prt_EndSizeBias_y", 416u, 0.0f},
});
constexpr auto kRotationFields = std::to_array<FloatField>({
    {"Attr_Prt_Start2DRotate", 424u, 0.0f},
    {"Attr_Prt_Start2DRotateBias", 428u, 0.0f},
    {"Attr_Prt_End2DRotate", 432u, 0.0f},
    {"Attr_Prt_End2DRotateBias", 436u, 0.0f},
});
constexpr auto kAtomicRotationFields = std::to_array<FloatField>({
    {"Attr_Prt_Atomic_Rotate_x", 88u, 0.0f},
    {"Attr_Prt_Atomic_Rotate_y", 92u, 0.0f},
    {"Attr_Prt_Atomic_Rotate_z", 96u, 0.0f},
});
constexpr auto kInitialColors = std::to_array<std::string_view>({
    "Attr_Prt_Color_Red", "Attr_Prt_Color_Green",
    "Attr_Prt_Color_Blue", "Attr_Prt_Color_Alpha",
});
constexpr auto kGradualColors = std::to_array<std::string_view>({
    "Attr_Prt_StartCol_Red", "Attr_Prt_StartCol_Green",
    "Attr_Prt_StartCol_Blue", "Attr_Prt_StartCol_Alpha",
    "Attr_Prt_StartColBias_Red", "Attr_Prt_StartColBias_Green",
    "Attr_Prt_StartColBias_Blue", "Attr_Prt_StartColBias_Alpha",
    "Attr_Prt_EndCol_Red", "Attr_Prt_EndCol_Green",
    "Attr_Prt_EndCol_Blue", "Attr_Prt_EndCol_Alpha",
    "Attr_Prt_EndColBias_Red", "Attr_Prt_EndColBias_Green",
    "Attr_Prt_EndColBias_Blue", "Attr_Prt_EndColBias_Alpha",
});

template <std::size_t N>
void ReadColors(std::uint32_t config, std::uint32_t object,
                const std::array<std::string_view, N>& keys,
                std::uint32_t base, std::int32_t fallback,
                IHookEffectConfigRuntime& runtime) {
    for (std::size_t index = 0; index < keys.size(); ++index) {
        const auto channel = static_cast<std::uint8_t>(
            runtime.ReadInt(config, keys[index], fallback));
        runtime.WriteFloat(
            object + base + static_cast<std::uint32_t>(index) * 4u,
            static_cast<float>(channel) * 0.0039215689f);
    }
}

std::uint8_t ReadFlag(std::uint32_t config, std::string_view key,
                      std::int32_t fallback,
                      IHookEffectConfigRuntime& runtime) {
    return runtime.ReadInt(config, key, fallback) > 0 ? 1u : 0u;
}

}  // namespace

bool HookEffect::InitializeFromConfig(
    std::uint32_t effect, std::uint32_t config,
    std::uint32_t frame, std::uint32_t owner,
    IHookEffectConfigRuntime& runtime) {
    runtime.Reset(effect);
    if (config == 0u || frame == 0u || owner == 0u) return false;
    if (runtime.ReadInt(config, "Hook_Tag", 0) != 0) return false;

    const auto object = runtime.CreateParticleObject(0x1b8u);
    if (object == 0u) return false;
    runtime.WriteWord(effect + 8u, object);
    runtime.WriteWord(object + 200u, static_cast<std::uint32_t>(
        runtime.ReadInt(config, "Attr_Prt_MaxPrt", 100)));
    runtime.WriteWord(object + 260u, static_cast<std::uint32_t>(
        runtime.ReadInt(config, "Attr_Prt_PrtEmit", 0)));
    runtime.WriteWord(object + 264u, static_cast<std::uint32_t>(
        runtime.ReadInt(config, "Attr_Prt_PrtEmit_Bias", 0)));
    ReadFloatFields(config, object, kEmissionFields, runtime);
    runtime.WriteByte(object + 2u, ReadFlag(
        config, "Attr_Prt_Alpha_AutoGradual", 1, runtime));
    ReadFloatFields(config, object, kInitialFields, runtime);
    ReadColors(config, object, kInitialColors, 212u, 255, runtime);
    ReadFloatFields(config, object, kGravityFields, runtime);
    runtime.WriteFloat(object + 240u, runtime.ReadFloat(
        config, "Attr_Prt_Force_x", 0.0f));
    // The native routine parses these attributes but discards both values.
    static_cast<void>(runtime.ReadFloat(config, "Attr_Prt_Force_y", 0.0f));
    static_cast<void>(runtime.ReadFloat(config, "Attr_Prt_Force_z", 0.0f));
    runtime.WriteByte(object + 316u, ReadFlag(
        config, "Attr_Prt_ColorGradual", 1, runtime));
    ReadColors(config, object, kGradualColors, 320u, 0, runtime);
    runtime.WriteByte(object + 384u, ReadFlag(
        config, "Attr_Prt_SizeGradual", 1, runtime));
    ReadFloatFields(config, object, kSizeFields, runtime);
    runtime.WriteByte(object + 420u, ReadFlag(
        config, "Attr_Prt_2DRotateGradual", 0, runtime));
    ReadFloatFields(config, object, kRotationFields, runtime);
    ReadFloatFields(config, object, kAtomicRotationFields, runtime);
    runtime.UpdateParticleFrame(object);

    const auto texture_path = std::string{"gamedata\\effect\\tex\\"} +
        runtime.ReadText(config, "Attr_Prt_Texture");
    auto texture = runtime.FindTexture(texture_path);
    if (texture == 0u) texture = runtime.LoadTexture(texture_path);
    const auto pre_render = runtime.ReadFloat(
        config, "Attr_PreRender_Time", 0.0f);
    const bool billboard = ReadFlag(
        config, "Attr_Prt_Is_Billboard", 0, runtime) != 0u;
    const bool point = ReadFlag(
        config, "Attr_Prt_Is_Point", 0, runtime) != 0u;
    const bool axis_xz = ReadFlag(
        config, "Attr_Prt_Axis_XZ", 0, runtime) != 0u;
    const bool face_point = ReadFlag(
        config, "Attr_Prt_Axis_FacePoint", 0, runtime) != 0u;
    const bool alpha_blend_enabled = ReadFlag(
        config, "Attr_Prt_AlphaBlend_off", 0, runtime) == 0u;
    runtime.WriteByte(object + 4u, ReadFlag(
        config, "Attr_prt_Inherit_Frame", 0, runtime));
    runtime.WriteByte(object + 53u, ReadFlag(
        config, "Attr_Prt_Strip", 0, runtime));
    runtime.WriteByte(object + 54u, ReadFlag(
        config, "Attr_Prt_Strip_Twice", 0, runtime));
    if (billboard) runtime.WriteWord(object + 124u, 1u);
    runtime.WriteWord(effect + 12u, owner);
    runtime.SetProperties(object, alpha_blend_enabled);
    const bool ready = runtime.SetupObject(
        object, !point, texture, frame, axis_xz);
    if (ready) {
        runtime.SetCollectionState(object, face_point);
        if (pre_render > 0.0f) {
            const auto step = static_cast<double>(pre_render) *
                              0.0020000000949949026;
            for (std::uint32_t index = 0u; index < 500u; ++index)
                runtime.Advance(object, step, runtime.ReadWord(owner + 48u));
        }
    } else {
        runtime.DestroyParticleObject(object);
        runtime.WriteWord(effect + 8u, 0u);
    }
    if (texture != 0u) runtime.DestroyTexture(texture);
    return ready;
}

}  // namespace pal4
