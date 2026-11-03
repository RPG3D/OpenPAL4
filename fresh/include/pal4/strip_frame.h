#pragma once

#include "pal4/strip_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IStripFrameRuntime {
public:
    virtual ~IStripFrameRuntime() = default;
    virtual void SetIdentity(std::uint32_t frame) = 0;
    virtual void Scale(std::uint32_t frame,
                       const std::array<float, 3>& values,
                       std::uint32_t mode) = 0;
    virtual void Rotate(std::uint32_t frame,
                        const std::array<float, 3>& axis,
                        float angle, std::uint32_t mode) = 0;
    virtual void Translate(std::uint32_t frame,
                           const std::array<float, 3>& values,
                           std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint8_t UpdateObjects(
        std::uint32_t frame) = 0;
};

class StripFrame final {
public:
    // IDA 0x61a850.
    [[nodiscard]] static std::uint8_t Rebuild(
        const StripObjectState& state, IStripFrameRuntime& runtime);
};

}  // namespace pal4
