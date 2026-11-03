#include "pal4/rtk_prt_std_atomic_configuration.h"

#include <array>
#include <bit>

namespace pal4 {
namespace {

constexpr std::string_view kEmitterLong = "\xB3\xA4\xB7\xBD\xCC\xE5";
constexpr std::string_view kEmitterSphere = "\xD4\xB2\xC7\xF2\xCC\xE5";
constexpr std::string_view kEmitterCylinder = "\xD4\xB2\xD6\xF9\xCC\xE5";

void StoreByte(RtkPrtStdAtomicObjectState& state,
               const std::uint32_t offset, const std::uint8_t value) {
    const auto shift = (offset % 4u) * 8u;
    auto& word = state.words[offset / 4u];
    word = (word & ~(0xffu << shift)) |
           (static_cast<std::uint32_t>(value) << shift);
}

void StoreFloat(RtkPrtStdAtomicObjectState& state,
                const std::uint32_t offset, const float value) {
    state.words[offset / 4u] = std::bit_cast<std::uint32_t>(value);
}

float NormalizedColor(const std::int32_t value) {
    //612F1C and613416 use the single-precision coefficient at84258C.
    return static_cast<float>(static_cast<std::uint8_t>(value)) *
        std::bit_cast<float>(0x3b808081u);
}

bool Positive(const std::int32_t value) { return value > 0; }

}  // namespace

bool RtkPrtStdAtomicConfiguration::Load(
    RtkPrtStdAtomicObjectState& state, const std::uint32_t document,
    IRtkPrtStdAtomicConfigurationRuntime& runtime) {
    if (document == 0u)
        return false;

    const auto memory = runtime.AllocateResource(0x1b8u);
    if (memory == 0u)
        return false;
    const auto resource = runtime.ConstructResource(memory);
    state.words[23u] = resource;
    if (resource == 0u) {
        runtime.FreeResource(memory);
        return false;
    }

    const auto ri = [&](const std::string_view key, const std::int32_t fallback) {
        return runtime.ReadInt(document, key, fallback);
    };
    const auto rf = [&](const std::string_view key, const float fallback) {
        return runtime.ReadFloat(document, key, fallback);
    };
    const auto rs = [&](const std::string_view key,
                        const std::string_view fallback) {
        return runtime.ReadString(document, key, fallback);
    };
    const auto wi = [&](const std::uint32_t offset, const std::int32_t value) {
        runtime.WriteWord(resource + offset, static_cast<std::uint32_t>(value));
    };
    const auto wf = [&](const std::uint32_t offset, const float value) {
        runtime.WriteFloat(resource + offset, value);
    };
    const auto wb = [&](const std::uint32_t offset, const bool value) {
        runtime.WriteByte(resource + offset, static_cast<std::uint8_t>(value));
    };

    wi(200u, ri("Attr_Prt_MaxPrt", 100));
    const auto emit = ri("Attr_Prt_PrtEmit", 0);
    const auto emit_bias = ri("Attr_Prt_PrtEmit_Bias", 0);
    wi(260u, emit);
    wi(264u, emit_bias);
    const auto emit_gap = rf("Attr_Prt_EmitGap", 0.0f);
    const auto emit_gap_bias = rf("Attr_Prt_EmitGap_Bias", 0.0f);
    wf(272u, emit_gap_bias);
    wf(268u, emit_gap);
    wb(2u, Positive(ri("Attr_Prt_Alpha_AutoGradual", 1)));

    const auto particle_size_x = rf("Attr_PrtSize_x", 4.0f);
    const auto particle_size_y = rf("Attr_PrtSize_y", 4.0f);
    wf(228u, particle_size_x);
    wf(232u, particle_size_y);
    wf(236u, rf("Attr_Prt_2DRotate", 0.0f));
    const auto life = rf("Attr_Prt_PrtLife", 0.0f);
    const auto life_bias = rf("Attr_Prt_PrtLife_Bias", 0.0f);
    wf(280u, life_bias);
    wf(276u, life);
    const auto initial_velocity = rf("Attr_Prt_InitVel", 0.0f);
    const auto initial_velocity_bias = rf("Attr_Prt_initVel_Bias", 0.0f);
    wf(288u, initial_velocity_bias);
    wf(284u, initial_velocity);

    const auto direction_x = rf("Attr_Prt_InitDir_x", 0.0f);
    const auto direction_y = rf("Attr_Prt_InitDir_y", 0.0f);
    const auto direction_z = rf("Attr_Prt_InitDir_z", 0.0f);
    const auto direction_bias_x = rf("Attr_Prt_InitDir_Bias_x", 0.0f);
    const auto direction_bias_y = rf("Attr_Prt_InitDir_Bias_y", 0.0f);
    const auto direction_bias_z = rf("Attr_Prt_InitDir_Bias_z", 0.0f);
    wf(292u, direction_x);
    wf(296u, direction_y);
    wf(300u, direction_z);
    wf(304u, direction_bias_x);
    wf(308u, direction_bias_y);
    wf(312u, direction_bias_z);

    const auto red = ri("Attr_Prt_Color_Red", 255);
    const auto green = ri("Attr_Prt_Color_Green", 255);
    const auto blue = ri("Attr_Prt_Color_Blue", 255);
    const auto alpha = ri("Attr_Prt_Color_Alpha", 255);
    wf(212u, NormalizedColor(red));
    wf(216u, NormalizedColor(green));
    wf(220u, NormalizedColor(blue));
    wf(224u, NormalizedColor(alpha));
    state.words[24u] = static_cast<std::uint8_t>(red) |
                       (static_cast<std::uint32_t>(static_cast<std::uint8_t>(green)) << 8u) |
                       (static_cast<std::uint32_t>(static_cast<std::uint8_t>(blue)) << 16u) |
                       (static_cast<std::uint32_t>(static_cast<std::uint8_t>(alpha)) << 24u);

    const auto gravity = rf("Attr_Prt_Gravitate", 0.0f);
    const auto gravity_x = rf("Attr_Prt_Gravitate_Pos_x", 0.0f);
    const auto gravity_y = rf("Attr_Prt_Gravitate_Pos_y", 0.0f);
    const auto gravity_z = rf("Attr_Prt_Gravitate_Pos_z", 0.0f);
    wf(244u, gravity);
    wf(248u, gravity_x);
    wf(252u, gravity_y);
    wf(256u, gravity_z);
    wf(240u, rf("Attr_Prt_Force_x", 0.0f));
    // The native function reads Y/Z but only stores the X force component.
    (void)rf("Attr_Prt_Force_y", 0.0f);
    (void)rf("Attr_Prt_Force_z", 0.0f);

    const auto color_gradual = Positive(ri("Attr_Prt_ColorGradual", 1));
    constexpr std::array<std::string_view, 16> kGradientColors{
        "Attr_Prt_StartCol_Red", "Attr_Prt_StartCol_Green",
        "Attr_Prt_StartCol_Blue", "Attr_Prt_StartCol_Alpha",
        "Attr_Prt_StartColBias_Red", "Attr_Prt_StartColBias_Green",
        "Attr_Prt_StartColBias_Blue", "Attr_Prt_StartColBias_Alpha",
        "Attr_Prt_EndCol_Red", "Attr_Prt_EndCol_Green",
        "Attr_Prt_EndCol_Blue", "Attr_Prt_EndCol_Alpha",
        "Attr_Prt_EndColBias_Red", "Attr_Prt_EndColBias_Green",
        "Attr_Prt_EndColBias_Blue", "Attr_Prt_EndColBias_Alpha"};
    std::array<std::int32_t, 16> colors{};
    for (std::size_t i = 0u; i != colors.size(); ++i)
        colors[i] = ri(kGradientColors[i], 0);
    for (std::size_t i = 0u; i != colors.size(); ++i)
        wf(320u + static_cast<std::uint32_t>(i * 4u),
           NormalizedColor(colors[i]));
    wb(316u, color_gradual);

    const auto size_gradual = Positive(ri("Attr_Prt_SizeGradual", 1));
    constexpr std::array<std::string_view, 8> kGradientSizes{
        "Attr_Prt_StartSize_x", "Attr_Prt_StartSize_y",
        "Attr_Prt_StartSizeBias_x", "Attr_Prt_StartSizeBias_y",
        "Attr_Prt_EndSize_x", "Attr_Prt_EndSize_y",
        "Attr_Prt_EndSizeBias_x", "Attr_Prt_EndSizeBias_y"};
    std::array<float, 8> sizes{};
    for (std::size_t i = 0u; i != sizes.size(); ++i)
        sizes[i] = rf(kGradientSizes[i], 0.0f);
    for (std::size_t i = 0u; i != sizes.size(); ++i)
        wf(388u + static_cast<std::uint32_t>(i * 4u), sizes[i]);
    wb(384u, size_gradual);

    const auto rotation_gradual = Positive(ri("Attr_Prt_2DRotateGradual", 0));
    constexpr std::array<std::string_view, 4> kGradientRotations{
        "Attr_Prt_Start2DRotate", "Attr_Prt_Start2DRotateBias",
        "Attr_Prt_End2DRotate", "Attr_Prt_End2DRotateBias"};
    std::array<float, 4> rotations{};
    for (std::size_t i = 0u; i != rotations.size(); ++i)
        rotations[i] = rf(kGradientRotations[i], 0.0f);
    for (std::size_t i = 0u; i != rotations.size(); ++i)
        wf(424u + static_cast<std::uint32_t>(i * 4u), rotations[i]);
    wb(420u, rotation_gradual);

    const auto transform = [&](const std::array<std::string_view, 3>& keys,
                               const float fallback, const std::uint32_t offset) {
        std::array<float, 3> values{};
        for (std::size_t i = 0u; i != 3u; ++i)
            values[i] = rf(keys[i], fallback);
        for (std::size_t i = 0u; i != 3u; ++i)
            wf(offset + static_cast<std::uint32_t>(i * 4u), values[i]);
        runtime.UpdateFrame(resource);
        return values;
    };
    const auto atomic_rotation = transform({
        "Attr_Prt_Atomic_Rotate_x", "Attr_Prt_Atomic_Rotate_y",
        "Attr_Prt_Atomic_Rotate_z"}, 0.0f, 88u);
    const auto atomic_scale = transform({
        "Attr_Prt_Atomic_Scale_x", "Attr_Prt_Atomic_Scale_y",
        "Attr_Prt_Atomic_Scale_z"}, 1.0f, 100u);
    const auto atomic_position = transform({
        "Attr_Prt_Atomic_Position_x", "Attr_Prt_Atomic_Position_y",
        "Attr_Prt_Atomic_Position_z"}, 0.0f, 112u);

    const auto emitter = rs("Attr_Prt_EmitterType", kEmitterLong);
    std::uint32_t emitter_type = 0u;
    if (emitter == kEmitterCylinder)
        emitter_type = 1u;
    else if (emitter == kEmitterSphere)
        emitter_type = 2u;
    const auto emitter_size_x = rf("Attr_Prt_Size_x", 0.0f);
    const auto emitter_size_y = rf("Attr_Prt_Size_y", 0.0f);
    const auto emitter_size_z = rf("Attr_Prt_Size_z", 0.0f);

    const auto texture_name = rs("Attr_Prt_Texture", "");
    const std::string texture_path = "gamedata\\effect\\tex\\" + texture_name;
    auto texture = runtime.FindTexture(texture_path);
    if (texture == 0u)
        texture = runtime.LoadTexture(texture_path);

    StoreFloat(state, 88u, rf("Attr_PreRender_Time", 0.0f));
    StoreByte(state, 9u, Positive(ri("Attr_Render_BeforeClump", 0)));
    const auto billboard = Positive(ri("Attr_Prt_Is_Billboard", 0));
    const auto point = Positive(ri("Attr_Prt_Is_Point", 0));
    const auto axis_xz = Positive(ri("Attr_Prt_Axis_XZ", 0));
    const auto face_point = Positive(ri("Attr_Prt_Axis_FacePoint", 0));
    const auto alpha_blend = !Positive(ri("Attr_Prt_AlphaBlend_off", 0));
    wb(4u, Positive(ri("Attr_prt_Inherit_Frame", 0)));
    wb(53u, Positive(ri("Attr_Prt_Strip", 0)));
    wb(54u, Positive(ri("Attr_Prt_Strip_Twice", 1)));
    if (billboard)
        wi(124u, 1);
    runtime.SetProperties(resource, alpha_blend);

    const bool success = runtime.InitializeResource(
        resource, !point, texture, emitter_type,
        emitter_size_x, emitter_size_y, emitter_size_z, axis_xz);
    if (success) {
        runtime.SetCollectionState(resource, face_point);
        StoreByte(state, 4u, 1u);
        for (std::size_t i = 0u; i != 3u; ++i) {
            StoreFloat(state, 100u + static_cast<std::uint32_t>(i * 4u), atomic_rotation[i]);
            StoreFloat(state, 112u + static_cast<std::uint32_t>(i * 4u), atomic_scale[i]);
            StoreFloat(state, 124u + static_cast<std::uint32_t>(i * 4u), atomic_position[i]);
        }
    } else {
        StoreByte(state, 4u, 0u);
        runtime.DestroyResource(resource);
        runtime.FreeResource(resource);
        state.words[23u] = 0u;
    }
    if (texture != 0u)
        runtime.DestroyTexture(texture);
    StoreByte(state, 11u, 0u);
    return success;
}

}  // namespace pal4
