#include "pal4/effect_camera_object.h"

namespace pal4 {
namespace {

constexpr std::array<std::uint32_t, 6> kBaseVtables{
    0x84a1bcu, 0x84a2c0u, 0x84a1c8u,
    0x84a1d0u, 0x84a194u, 0x84a178u};
constexpr std::array<std::uint32_t, 6> kDerivedVtables{
    0x84a25cu, 0x84a238u, 0x84a230u,
    0x84a224u, 0x84a218u, 0x84a20cu};

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

void SetVtables(EffectCameraObjectState& state,
                const std::array<std::uint32_t, 6>& vtables) {
    for (std::uint32_t index = 0u; index < vtables.size(); ++index)
        state.words[5u + index] = vtables[index];
}

void UpdateInstanceCount(IEffectCameraObjectRuntime& runtime,
                         const std::uint32_t delta) {
    const auto address = runtime.SystemInstance() + 164u;
    runtime.WriteWord(address, runtime.ReadWord(address) + delta);
}

}  // namespace

EffectCameraObjectState& EffectCameraObject::Construct(
    EffectCameraObjectState& state, const std::uint8_t initial_byte,
    IEffectCameraObjectRuntime& runtime) {
    runtime.ResetBase(state);
    SetVtables(state, kBaseVtables);
    WriteByte(state, 44u, 0u);
    WriteByte(state, 47u, 0u);
    WriteByte(state, 60u, 0u);
    WriteByte(state, 64u, initial_byte);
    for (std::uint32_t index = 17u; index <= 19u; ++index)
        state.words[index] = 0u;
    state.words[31u] = 0u;
    state.words[32u] = 0u;
    bool xml_ready = false;
    try {
        runtime.InitializeXml(state, 132u);
        xml_ready = true;
        state.words[0u] = 0x84a268u;
        SetVtables(state, kDerivedVtables);
        for (std::uint32_t index = 22u; index <= 27u; ++index)
            state.words[index] = 0u;
        for (std::uint32_t index = 12u; index <= 14u; ++index)
            state.words[index] = 0u;
        UpdateInstanceCount(runtime, 1u);
    } catch (...) {
        if (xml_ready)
            runtime.DestroyXml(state, 132u);
        runtime.FinalizeField64(state, 64u, 1u);
        for (std::uint32_t index = 6u; index-- > 0u;)
            state.words[5u + index] = kBaseVtables[index];
        runtime.FinalizeBase(state);
        throw;
    }
    return state;
}

void EffectCameraObject::Destroy(EffectCameraObjectState& state,
                                 IEffectCameraObjectRuntime& runtime) {
    state.words[0u] = 0x84a268u;
    SetVtables(state, kDerivedVtables);
    UpdateInstanceCount(runtime, 0xffffffffu);
    runtime.DestroyXml(state, 132u);
    runtime.FinalizeField64(state, 64u, 1u);
    for (std::uint32_t index = 6u; index-- > 0u;)
        state.words[5u + index] = kBaseVtables[index];
    runtime.FinalizeBase(state);
}

std::uint32_t EffectCameraObject::CachedWord124(
    const EffectCameraObjectState& state) {
    return state.words[31u];
}

std::string_view EffectCameraObject::ClassName() {
    return "CameraObject";
}

std::uint8_t EffectCameraObject::SetStartTaskFlag(
    EffectCameraObjectState& state, const std::uint8_t value,
    IEffectCameraObjectRuntime& runtime) {
    if (ReadByte(state, 4u) == 0u)
        return 0u;
    if (ReadByte(state, 11u) != value) {
        WriteByte(state, 11u, value);
        if (value != 0u)
            return runtime.StartCameraFacet(state, 24u);
    }
    return value;
}

std::uint8_t EffectCameraObject::SetLookAtPosition(
    EffectCameraObjectState& state,
    const std::array<std::uint32_t, 3>& position) {
    if (ReadByte(state, 4u) == 0u && ReadByte(state, 80u) == 0u)
        return 0u;
    for (std::uint32_t index = 0u; index < position.size(); ++index)
        state.words[12u + index] = position[index];
    return static_cast<std::uint8_t>(position[2u]);
}

std::uint8_t EffectCameraObject::SetRotation(
    EffectCameraObjectState& state,
    const std::array<std::uint32_t, 3>& rotation) {
    if (ReadByte(state, 4u) == 0u)
        return 0u;
    if (ReadByte(state, 80u) == 0u)
        return 0u;
    for (std::uint32_t index = 0u; index < rotation.size(); ++index)
        state.words[22u + index] = rotation[index];
    return static_cast<std::uint8_t>(rotation[2u]);
}

std::uint8_t EffectCameraObject::InheritCurrentFlag(
    const EffectCameraObjectState& state) {
    return ReadByte(state, 44u);
}

std::uint8_t EffectCameraObject::LockLookAtFlag(
    const EffectCameraObjectState& state) {
    return ReadByte(state, 47u);
}

std::uint8_t EffectCameraObject::SetLockLookAtFlag(
    EffectCameraObjectState& state, const std::uint8_t value) {
    if (ReadByte(state, 4u) == 0u)
        return 0u;
    WriteByte(state, 47u, value);
    if (value == 0u)
        return value;
    for (std::uint32_t index = 0u; index < 3u; ++index)
        state.words[22u + index] = state.words[28u + index];
    return static_cast<std::uint8_t>(state.words[24u]);
}

}  // namespace pal4
