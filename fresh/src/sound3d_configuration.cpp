#include "pal4/sound3d_configuration.h"

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

std::uint8_t Sound3DConfiguration::Load(
    Sound3DObjectState& state, const std::uint32_t document,
    ISound3DConfigurationRuntime& runtime) {
    if (document == 0u)
        return 0u;

    const std::array<float, 3> position{
        runtime.ReadFloat(document, "Attr_Sound3D_SoundPos_x", 0.0f),
        runtime.ReadFloat(document, "Attr_Sound3D_SoundPos_y", 0.0f),
        runtime.ReadFloat(document, "Attr_Sound3D_SoundPos_z", 0.0f)};
    for (std::uint32_t i = 0u; i < position.size(); ++i)
        state.words[10u + i] = std::bit_cast<std::uint32_t>(position[i]);

    runtime.InitializeSoundPath(state, "gamedata\\PALSound\\");
    const auto filename = runtime.ReadString(document, "Attr_Sound3D_SoundFile");
    runtime.AppendSoundFile(state, filename);
    StoreByte(state, 56u,
              runtime.ReadInt(document, "Attr_Sound3D_2D", 0) > 0 ? 1u : 0u);

    const auto frame = runtime.CreateFrame();
    state.words[13u] = frame;
    for (std::uint32_t i = 0u; i < position.size(); ++i)
        state.words[20u + i] = state.words[10u + i];
    runtime.SetFrameIdentity(frame);
    runtime.ScaleFrame(frame, {1.0f, 1.0f, 1.0f}, 0u);
    runtime.RotateFrame(frame, {1.0f, 0.0f, 0.0f}, 0.0f, 2u);
    runtime.RotateFrame(frame, {0.0f, 1.0f, 0.0f}, 0.0f, 2u);
    runtime.RotateFrame(frame, {0.0f, 0.0f, 1.0f}, 0.0f, 2u);
    runtime.TranslateFrame(frame, position, 2u);
    StoreByte(state, 4u, 1u);
    StoreByte(state, 11u, 0u);
    return 1u;
}

}  // namespace pal4
