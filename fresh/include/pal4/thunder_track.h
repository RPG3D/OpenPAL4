#pragma once

#include "pal4/thunder_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IThunderTrackRuntime {
public:
    virtual ~IThunderTrackRuntime() = default;
    [[nodiscard]] virtual std::uint32_t UpdateFrame(
        ThunderObjectState& state) = 0;
};

class ThunderTrack final {
public:
    // IDA 0x61c860: +36 TrackTask facet.
    [[nodiscard]] static std::uint32_t SetTargetPosition(
        ThunderObjectState& state, const std::array<std::uint32_t, 3u>& values,
        std::uint32_t entry_eax, IThunderTrackRuntime& runtime);
};

}  // namespace pal4
