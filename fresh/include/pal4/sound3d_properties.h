#pragma once

#include "pal4/sound3d_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

class ISound3DPropertyRuntime {
public:
    virtual ~ISound3DPropertyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetFrame(
        Sound3DObjectState& state) = 0;
    virtual void SetFrameIdentity(std::uint32_t frame) = 0;
    virtual void ScaleFrame(std::uint32_t frame,
                            const std::array<float, 3>& scale,
                            std::uint32_t mode) = 0;
    virtual void RotateFrame(std::uint32_t frame,
                             const std::array<float, 3>& axis,
                             float degrees, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t TranslateFrame(
        std::uint32_t frame, const std::array<float, 3>& position,
        std::uint32_t mode) = 0;
};

class Sound3DProperties final {
public:
    // IDA 0x6177d0, +20 position facet.
    [[nodiscard]] static std::uint8_t GetPosition(
        const Sound3DObjectState& state, std::uint32_t& x,
        std::uint32_t& y, std::uint32_t& z);
    // IDA 0x617800, +20 position facet.
    [[nodiscard]] static std::uint32_t SetPosition(
        Sound3DObjectState& state, float x, float y, float z,
        std::uint8_t relative, ISound3DPropertyRuntime& runtime);
};

}  // namespace pal4
