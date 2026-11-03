#pragma once

#include "pal4/sound3d_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

class ISound3DControlRuntime {
public:
    virtual ~ISound3DControlRuntime() = default;
    [[nodiscard]] virtual bool AudioFacetActive(
        Sound3DObjectState& state, std::uint32_t facet_offset) = 0;
    virtual void HandleActiveAudio(
        Sound3DObjectState& state, std::uint32_t facet_offset) = 0;
    virtual void SetFrameIdentity(std::uint32_t frame) = 0;
    virtual void ScaleFrame(std::uint32_t frame,
                            const std::array<float, 3>& scale,
                            std::uint32_t mode) = 0;
    virtual void RotateFrame(std::uint32_t frame,
                             const std::array<float, 3>& axis,
                             float degrees, std::uint32_t mode) = 0;
    virtual void TranslateFrame(std::uint32_t frame,
                                const std::array<float, 3>& position,
                                std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void FreeText(std::uint32_t allocation) = 0;
    virtual void ReleaseSoundPath(Sound3DObjectState& state);
    virtual void DestroyFrame(std::uint32_t frame) = 0;
    virtual void ProcessEvent(std::uint32_t system,
                              Sound3DObjectState& state) = 0;
    [[nodiscard]] virtual std::uint32_t DeleteObject(
        Sound3DObjectState& state, std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t ObjectTransform(
        std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint8_t SetAudioPosition(
        Sound3DObjectState& state, std::uint32_t facet_offset,
        std::uint32_t position_address) = 0;
};

class Sound3DControl final {
public:
    // IDA 0x6175a0.
    [[nodiscard]] static std::uint8_t Reset(
        Sound3DObjectState& state, ISound3DControlRuntime& runtime);
    // IDA 0x617670.
    [[nodiscard]] static std::uint32_t Stop(
        Sound3DObjectState& state, ISound3DControlRuntime& runtime);
    // IDA 0x617780; the two native arguments are unused.
    [[nodiscard]] static std::uint8_t UpdateAudioPosition(
        Sound3DObjectState& state, std::uint32_t arg1,
        std::uint32_t arg2, ISound3DControlRuntime& runtime);
};

}  // namespace pal4
