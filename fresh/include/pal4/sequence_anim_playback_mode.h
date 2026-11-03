#pragma once

#include "pal4/sequence_anim_object.h"

#include <cstdint>

namespace pal4 {

class ISequenceAnimPlaybackModeRuntime {
public:
    virtual ~ISequenceAnimPlaybackModeRuntime() = default;
    virtual void QueryPlayback(std::uint32_t facet_vtable,
                               std::uint32_t facet_offset,
                               std::int32_t& first,
                               std::int32_t& second,
                               std::int32_t& third,
                               std::int32_t& frame) = 0;
    [[nodiscard]] virtual double Duration(
        const SequenceAnimObjectState& state) = 0;
};

class SequenceAnimPlaybackMode final {
public:
    // IDA 0x615a10.
    static void Set(SequenceAnimObjectState& state, std::int32_t mode,
                    ISequenceAnimPlaybackModeRuntime& runtime);
};

}  // namespace pal4
