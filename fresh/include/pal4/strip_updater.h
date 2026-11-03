#pragma once

#include "pal4/strip_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

struct StripPlaybackSample final {
    std::int32_t remaining = 0;
    std::uint8_t red = 0;
    std::uint8_t green = 0;
    std::uint8_t blue = 0;
};

class IStripUpdateRuntime {
public:
    virtual ~IStripUpdateRuntime() = default;
    [[nodiscard]] virtual StripPlaybackSample ReadPlayback(
        StripObjectState& state, std::uint32_t facet_offset) = 0;
    [[nodiscard]] virtual std::int32_t FloatToInt(double value) = 0;
    virtual void WritePlayback(StripObjectState& state,
                               std::uint32_t facet_offset,
                               const std::array<std::uint8_t, 4>& bytes) = 0;
    virtual void UpdateGeometryVertices(
        std::uint32_t model, const std::array<float, 4>& uv) = 0;
};

class StripUpdater final {
public:
    // IDA 0x619a00, ret8: double delta only; saved EDI is not an argument.
    static void Advance(StripObjectState& state, double delta,
                        IStripUpdateRuntime& runtime);
};

}  // namespace pal4
