#pragma once

#include "pal4/sound3d_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

class ISound3DRevoluteRuntime {
public:
    virtual ~ISound3DRevoluteRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetFrame(
        Sound3DObjectState& state) = 0;
    virtual void TranslateMatrix(std::uint32_t matrix,
                                 const std::array<float, 3>& vector,
                                 std::uint32_t mode) = 0;
    virtual void RotateMatrix(std::uint32_t matrix,
                              const std::array<float, 3>& axis,
                              float angle, std::uint32_t mode) = 0;
    virtual void UpdateFrameObjects(std::uint32_t frame) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t RefreshPosition(
        Sound3DObjectState& state,
        const std::array<float, 3>& zero_delta,
        std::uint32_t mode) = 0;
};

class Sound3DRevoluteFrame final {
public:
    // IDA 0x6178d0, owner +24 facet.
    [[nodiscard]] static std::uint32_t Apply(
        Sound3DObjectState& state,
        const std::array<float, 3>& pivot,
        const std::array<float, 3>& rotation,
        const std::array<float, 3>& step,
        std::uint32_t entry_eax,
        ISound3DRevoluteRuntime& runtime);
};

}  // namespace pal4
