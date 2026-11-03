#include "pal4/sound3d_playback_facet.h"

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

std::uint8_t Sound3DPlaybackFacet::Play(
    Sound3DObjectState& state, const std::uint8_t volume,
    ISound3DPlaybackRuntime& runtime) {
    const auto active = static_cast<std::uint8_t>(state.words[1u] & 0xffu);
    if (active == 0u)
        return active;
    const auto playing = static_cast<std::uint8_t>(
        (state.words[14u] >> 8u) & 0xffu);
    if (playing != 0u)
        return playing;

    if (!runtime.AudioManagerInitialized())
        runtime.InitializeAudioManager(0u);
    const auto text = state.words[17u];
    const auto path = text == 0u ? std::string_view{} : runtime.ReadSoundPath(text);
    const auto two_dimensional = static_cast<std::uint8_t>(state.words[14u] & 0xffu);
    if (two_dimensional == 0u) {
        state.words[15u] = runtime.PlayMusic(path, volume);
    } else {
        const auto transform = runtime.ObjectTransform(state.words[13u]);
        state.words[15u] = runtime.Play3DSound(
            path, transform + 48u, volume, 600.0f, 300.0f);
    }
    const auto result = runtime.AudioActive(state.words[15u]) ? 1u : 0u;
    StoreByte(state, 57u, static_cast<std::uint8_t>(result));
    return static_cast<std::uint8_t>(result);
}

std::uint8_t Sound3DPlaybackFacet::Stop(
    Sound3DObjectState& state, ISound3DPlaybackRuntime& runtime) {
    const auto active = static_cast<std::uint8_t>(state.words[1u] & 0xffu);
    if (active == 0u)
        return active;
    const auto playing = static_cast<std::uint8_t>(
        (state.words[14u] >> 8u) & 0xffu);
    if (playing == 0u)
        return playing;
    auto result = runtime.IsPlaying(state.words[15u]);
    if (result != 0u)
        result = runtime.PlayExisting(state.words[15u]);
    StoreByte(state, 57u, 0u);
    return result;
}

}  // namespace pal4
