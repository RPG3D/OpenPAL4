#pragma once

#include "pal4/thunder_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IThunderRevoluteRuntime {
public:
    virtual ~IThunderRevoluteRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetFrame(
        ThunderObjectState& state) = 0;
    virtual void TranslateMatrix(
        std::uint32_t matrix, const std::array<float, 3u>& translation,
        std::uint32_t mode) = 0;
    virtual void RotateMatrix(
        std::uint32_t matrix, const std::array<float, 3u>& axis,
        float degrees, std::uint32_t mode) = 0;
    virtual void UpdateFrameObjects(std::uint32_t frame) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SetPosition(
        ThunderObjectState& state, const std::array<float, 3u>& position,
        std::uint32_t relative) = 0;
};

class ThunderRevoluteFrame final {
public:
    // IDA 0x61c8d0: +32 RevoluteTask facet.
    [[nodiscard]] static std::uint32_t Apply(
        ThunderObjectState& state, const std::array<float, 3u>& pivot,
        const std::array<float, 3u>& rotation,
        const std::array<float, 3u>& step, std::uint32_t entry_eax,
        IThunderRevoluteRuntime& runtime);
};

}  // namespace pal4
