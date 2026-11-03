#pragma once
#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {
// Borrowed audio owner. Handles remain owned by the audio manager after an
// effect is deleted, as in the original detached sample lifecycle.
class INativeEffectSound {
public:
    virtual ~INativeEffectSound() = default;
    virtual std::uint32_t Play(std::string_view path, bool spatial, bool once,
        const std::array<float,3>& position, float max_distance, float min_distance) = 0;
    virtual bool Exists(std::uint32_t handle) = 0;
    virtual bool Playing(std::uint32_t handle) = 0;
    virtual bool Stop(std::uint32_t handle) = 0;
    virtual bool SetPosition(std::uint32_t handle,const std::array<float,3>&) = 0;
};
class INativeEffectSoundFacets {
public:
    virtual ~INativeEffectSoundFacets() = default;
    virtual void StartSound(std::uint32_t facet,std::uint8_t once) = 0;
    virtual void StopSound(std::uint32_t facet) = 0;
};
}
