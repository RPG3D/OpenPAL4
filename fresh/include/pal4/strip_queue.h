#pragma once

#include "pal4/effect_system_object_f_queue.h"
#include "pal4/strip_object.h"

#include <cstdint>

namespace pal4 {

class IStripQueueRuntime {
public:
    virtual ~IStripQueueRuntime() = default;
    virtual void Dispatch(std::uint32_t system, std::uint32_t slot,
                          EffectSystemHandleWords handle,
                          std::uint32_t opaque) = 0;
    [[nodiscard]] virtual std::uint32_t Reset(
        EffectSystemHandleWords& handle) = 0;
};

class StripQueue final {
public:
    // IDA 0x61a950/0x61a9e0; owner +56 facet.
    [[nodiscard]] static std::uint32_t Add(
        const StripObjectState& state, std::uint32_t id,
        std::uint32_t opaque, IStripQueueRuntime& runtime);
    [[nodiscard]] static std::uint32_t Process(
        const StripObjectState& state, std::uint32_t id,
        std::uint32_t opaque, IStripQueueRuntime& runtime);
};

}  // namespace pal4
