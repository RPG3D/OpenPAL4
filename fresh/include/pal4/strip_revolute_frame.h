#pragma once

#include "pal4/strip_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IStripRevoluteRuntime {
public:
    virtual ~IStripRevoluteRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetFrame(
        StripObjectState& state) = 0;
    virtual void TranslateMatrix(
        std::uint32_t matrix, const std::array<float, 3>& position,
        std::uint32_t mode) = 0;
    virtual void RotateMatrix(
        std::uint32_t matrix, const std::array<float, 3>& axis,
        float angle, std::uint32_t mode) = 0;
    virtual void UpdateFrameObjects(std::uint32_t frame) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t RefreshPosition(
        StripObjectState& state, const std::array<float, 3>& delta,
        std::uint32_t mode) = 0;
};

class StripRevoluteFrame final {
public:
    // IDA 0x619ec0.
    [[nodiscard]] static std::uint32_t Apply(
        StripObjectState& state, const std::array<float, 3>& pivot,
        const std::array<float, 3>& rotation,
        const std::array<float, 3>& step, std::uint32_t entry_eax,
        IStripRevoluteRuntime& runtime);
};

}  // namespace pal4
