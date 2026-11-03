#pragma once

#include "pal4/effect_system_object_f_queue.h"
#include "pal4/thunder_object.h"

#include <cstdint>

namespace pal4 {

class IThunderQueueRuntime {
public:
    virtual ~IThunderQueueRuntime() = default;
    virtual void Dispatch(std::uint32_t system, std::uint32_t slot,
                          EffectSystemHandleWords handle,
                          std::uint32_t opaque) = 0;
    [[nodiscard]] virtual std::uint32_t Reset(
        EffectSystemHandleWords& handle) = 0;
};

class ThunderQueue final {
public:
    // IDA 0x61cb40/0x61cbd0: +52 facet.
    [[nodiscard]] static std::uint32_t Add(
        const ThunderObjectState& state, std::uint32_t id,
        std::uint32_t opaque, IThunderQueueRuntime& runtime);
    [[nodiscard]] static std::uint32_t Process(
        const ThunderObjectState& state, std::uint32_t id,
        std::uint32_t opaque, IThunderQueueRuntime& runtime);
};

}  // namespace pal4
