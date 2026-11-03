#pragma once

#include "pal4/sound3d_object.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class ISound3DPlaybackRuntime {
public:
    virtual ~ISound3DPlaybackRuntime() = default;
    [[nodiscard]] virtual bool AudioManagerInitialized() = 0;
    virtual void InitializeAudioManager(std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::string_view ReadSoundPath(
        std::uint32_t text_pointer) = 0;
    [[nodiscard]] virtual std::uint32_t PlayMusic(
        std::string_view path, std::uint8_t volume) = 0;
    [[nodiscard]] virtual std::uint32_t ObjectTransform(
        std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t Play3DSound(
        std::string_view path, std::uint32_t position_address,
        std::uint8_t volume, float max_distance,
        float min_distance) = 0;
    [[nodiscard]] virtual bool AudioActive(std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::uint8_t IsPlaying(std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::uint8_t PlayExisting(
        std::uint32_t handle) = 0;
};

class Sound3DPlaybackFacet final {
public:
    // IDA 0x617b00, owner +28 facet.
    [[nodiscard]] static std::uint8_t Play(
        Sound3DObjectState& state, std::uint8_t volume,
        ISound3DPlaybackRuntime& runtime);
    // IDA 0x617c30, owner +28 facet.
    [[nodiscard]] static std::uint8_t Stop(
        Sound3DObjectState& state, ISound3DPlaybackRuntime& runtime);
};

}  // namespace pal4
