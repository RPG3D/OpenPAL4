#include "pal4/effect_camera_configuration.h"

#include <array>
#include <bit>
#include <utility>

namespace pal4 {
namespace {

void WriteByte(EffectCameraObjectState& state, const std::uint32_t offset,
               const std::uint8_t value) {
    const auto shift = 8u * (offset % 4u);
    const auto mask = 0xffu << shift;
    state.words[offset / 4u] =
        (state.words[offset / 4u] & ~mask) |
        (static_cast<std::uint32_t>(value) << shift);
}

std::uint8_t ReadByte(const EffectCameraObjectState& state,
                      const std::uint32_t offset) {
    return static_cast<std::uint8_t>(
        state.words[offset / 4u] >> (8u * (offset % 4u)));
}

void WriteFloat(EffectCameraObjectState& state, const std::uint32_t offset,
                const float value) {
    state.words[offset / 4u] = std::bit_cast<std::uint32_t>(value);
}

void ReadFlagsAndTarget(EffectCameraObjectState& state,
                        const std::uint32_t document,
                        IEffectCameraConfigurationRuntime& runtime) {
    WriteByte(state, 44u, runtime.ReadInt(
        document, "Attr_Camera_Inherit_Current", 0) > 0 ? 1u : 0u);
    WriteByte(state, 47u, runtime.ReadInt(
        document, "Attr_Camera_Lock_LookAt", 0) > 0 ? 1u : 0u);
    WriteByte(state, 60u, runtime.ReadInt(
        document, "Attr_Camera_Target_Mode", 0) > 0 ? 1u : 0u);

    const auto name = runtime.ReadTargetName(
        document, "Attr_Camera_Target_Name", 0x8e425cu);
    try {
        runtime.AssignTargetName(state, name);
    } catch (...) {
        runtime.ReleaseTargetName(name);
        throw;
    }
    runtime.ReleaseTargetName(name);
    if (ReadByte(state, 44u) != 0u)
        WriteByte(state, 47u, 0u);
}

void ReadLookAt(EffectCameraObjectState& state,
                const std::uint32_t document,
                IEffectCameraConfigurationRuntime& runtime) {
    constexpr std::array<std::pair<std::string_view, std::uint32_t>, 3> keys{{
        {"Attr_Camera_LookAt_x", 48u},
        {"Attr_Camera_LookAt_y", 52u},
        {"Attr_Camera_LookAt_z", 56u},
    }};
    for (const auto& [key, offset] : keys)
        WriteFloat(state, offset, runtime.ReadFloat(document, key, 0.0f));
}

void ApplyModeOverrides(EffectCameraObjectState& state) {
    if (ReadByte(state, 47u) != 0u) {
        state.words[22u] = 0u;
        state.words[23u] = 0u;
        state.words[24u] = 0u;
    }
    if (ReadByte(state, 44u) != 0u) {
        for (std::uint32_t index = 22u; index <= 25u; ++index)
            state.words[index] = 0u;
        state.words[26u] = 0x3f800000u;
        state.words[27u] = 0x3f800000u;
    }
}

void ResetFrame(const std::uint32_t frame,
                IEffectCameraConfigurationRuntime& runtime) {
    runtime.SetFrameIdentity(frame);
    constexpr std::array<std::uint32_t, 3> axes{
        0x84a1e8u, 0x84a1f4u, 0x84a200u};
    constexpr std::array<float, 3> angles{360.0f, 0.0f, 360.0f};
    for (std::uint32_t index = 0u; index < axes.size(); ++index)
        runtime.RotateFrame(frame, axes[index], angles[index], 2u);
    runtime.UpdateFrameObjects(frame);
}

void FinishLoad(EffectCameraObjectState& state) {
    WriteByte(state, 80u, 0u);
    WriteByte(state, 4u, 1u);
    WriteByte(state, 11u, 0u);
}

}  // namespace

bool EffectCameraConfiguration::Load(
    EffectCameraObjectState& state, const std::uint32_t document,
    IEffectCameraConfigurationRuntime& runtime) {
    if (document == 0u)
        return false;

    ReadFlagsAndTarget(state, document, runtime);
    ReadLookAt(state, document, runtime);
    WriteFloat(state, 88u, runtime.ReadFloat(
        document, "Attr_Camera_Pitch", 0.0f));
    WriteFloat(state, 92u, runtime.ReadFloat(
        document, "Attr_Camera_Yaw", 0.0f));
    WriteFloat(state, 96u, runtime.ReadTreeFloat(
        document, "Attr_Camera_Roll", 0.0f));
    WriteFloat(state, 100u, runtime.ReadFloat(
        document, "Attr_Camera_Position_X", 0.0f));
    WriteFloat(state, 104u, runtime.ReadTreeFloat(
        document, "Attr_Camera_Position_Y", 0.0f));
    WriteFloat(state, 108u, runtime.ReadFloat(
        document, "Attr_Camera_Position_Z", 0.0f));
    WriteFloat(state, 84u, runtime.ReadFloat(
        document, "Attr_Camera_RandRang", 1.0f));

    ApplyModeOverrides(state);

    const auto frame = runtime.CreateFrame();
    state.words[31u] = frame;
    ResetFrame(frame, runtime);
    FinishLoad(state);
    runtime.AssignXml(state, document);
    return true;
}

bool EffectCameraConfiguration::Reload(
    EffectCameraObjectState& state,
    IEffectCameraConfigurationRuntime& runtime) {
    if (ReadByte(state, 4u) == 0u)
        return false;
    runtime.StartCameraFacet(state, 24u);
    const auto document = runtime.CachedDocument(state);
    ReadFlagsAndTarget(state, document, runtime);
    ReadLookAt(state, document, runtime);
    WriteFloat(state, 88u, runtime.ReadFloat(
        document, "Attr_Camera_Pitch", 0.0f));
    WriteFloat(state, 92u, runtime.ReadTreeFloat(
        document, "Attr_Camera_Yaw", 0.0f));
    WriteFloat(state, 96u, runtime.ReadFloat(
        document, "Attr_Camera_Roll", 0.0f));
    WriteFloat(state, 100u, runtime.ReadFloat(
        document, "Attr_Camera_Position_X", 0.0f));
    WriteFloat(state, 104u, runtime.ReadTreeFloat(
        document, "Attr_Camera_Position_Y", 0.0f));
    WriteFloat(state, 108u, runtime.ReadFloat(
        document, "Attr_Camera_Position_Z", 0.0f));
    WriteFloat(state, 84u, runtime.ReadTreeFloat(
        document, "Attr_Camera_RandRang", 1.0f));
    ApplyModeOverrides(state);
    ResetFrame(state.words[31u], runtime);
    FinishLoad(state);
    return true;
}

}  // namespace pal4
