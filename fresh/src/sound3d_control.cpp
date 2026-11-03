#include "pal4/sound3d_control.h"

#include <bit>

namespace pal4 {
namespace {

void StoreByte(Sound3DObjectState& state,
               const std::uint32_t offset, const std::uint8_t value) {
    const auto shift = (offset % 4u) * 8u;
    auto& word = state.words[offset / 4u];
    word = (word & ~(0xffu << shift)) |
           (static_cast<std::uint32_t>(value) << shift);
}

}  // namespace

std::uint8_t Sound3DControl::Reset(
    Sound3DObjectState& state, ISound3DControlRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return 0u;

    if (runtime.AudioFacetActive(state, 60u))
        runtime.HandleActiveAudio(state, 60u);
    state.words[2u] &= 0x00ffffffu;
    const std::array<float, 3> position{
        std::bit_cast<float>(state.words[20u]),
        std::bit_cast<float>(state.words[21u]),
        std::bit_cast<float>(state.words[22u])};
    for (std::uint32_t i = 0u; i < position.size(); ++i)
        state.words[10u + i] = state.words[20u + i];
    const auto frame = state.words[13u];
    runtime.SetFrameIdentity(frame);
    runtime.ScaleFrame(frame, {1.0f, 1.0f, 1.0f}, 0u);
    runtime.RotateFrame(frame, {1.0f, 0.0f, 0.0f}, 0.0f, 2u);
    runtime.RotateFrame(frame, {0.0f, 1.0f, 0.0f}, 0.0f, 2u);
    runtime.RotateFrame(frame, {0.0f, 0.0f, 1.0f}, 0.0f, 2u);
    runtime.TranslateFrame(frame, position, 2u);
    return 1u;
}

std::uint32_t Sound3DControl::Stop(
    Sound3DObjectState& state, ISound3DControlRuntime& runtime) {
    if ((state.words[1u] & 0xffu) != 0u) {
        StoreByte(state, 4u, 0u);
        StoreByte(state, 6u, 0u);
        state.words[10u] = 0u;
        state.words[11u] = std::bit_cast<std::uint32_t>(1.0f);
        state.words[12u] = std::bit_cast<std::uint32_t>(1.0f);
        StoreByte(state, 56u, 0u);
        StoreByte(state, 57u, 0u);

        runtime.ReleaseSoundPath(state);

        runtime.DestroyFrame(state.words[13u]);
        state.words[13u] = 0u;
        runtime.ProcessEvent(state.words[4u], state);
    }
    return runtime.DeleteObject(state, 1u);
}

void ISound3DControlRuntime::ReleaseSoundPath(Sound3DObjectState& state) {
        const auto text = state.words[17u];
        if (text != 0u) {
            const auto ref_address = text - 1u;
            const auto refs = ReadByte(ref_address);
            if (refs == 0u || refs == 0xffu)
                FreeText(ref_address);
            else
                WriteByte(ref_address,
                                  static_cast<std::uint8_t>(refs - 1u));
        }
        state.words[17u] = 0u;
        state.words[18u] = 0u;
        state.words[19u] = 0u;
}

std::uint8_t Sound3DControl::UpdateAudioPosition(
    Sound3DObjectState& state, [[maybe_unused]] const std::uint32_t arg1,
    [[maybe_unused]] const std::uint32_t arg2,
    ISound3DControlRuntime& runtime) {
    const auto active = static_cast<std::uint8_t>(state.words[1u] & 0xffu);
    if (active == 0u)
        return active;
    const auto playing = static_cast<std::uint8_t>((state.words[14u] >> 8u) & 0xffu);
    if (playing == 0u)
        return playing;
    const auto audio_active = runtime.AudioFacetActive(state, 60u);
    if (!audio_active)
        return 0u;
    const auto two_dimensional = static_cast<std::uint8_t>(state.words[14u] & 0xffu);
    if (two_dimensional != 0u)
        return two_dimensional;
    const auto transform = runtime.ObjectTransform(state.words[13u]);
    return runtime.SetAudioPosition(state, 60u, transform + 48u);
}

}  // namespace pal4
