#pragma once

#include "pal4/effect_object.h"
#include "pal4/effect_system_object_f_queue.h"

#include <cstdint>

namespace pal4 {

class IEffectObjectQueueRuntime {
public:
    virtual ~IEffectObjectQueueRuntime() = default;
    virtual void Dispatch(std::uint32_t system, std::uint32_t slot,
                          EffectSystemHandleWords handle,
                          std::uint32_t opaque) = 0;
    [[nodiscard]] virtual std::uint32_t Reset(
        EffectSystemHandleWords& handle) = 0;
};

class EffectObjectQueueRuntimeStub final : public IEffectObjectQueueRuntime {
public:
    void Dispatch(std::uint32_t, std::uint32_t,
                  EffectSystemHandleWords, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t Reset(
        EffectSystemHandleWords& handle) override {
        handle = {0xffffffffu, 0u};
        return 0u;
    }
};

class EffectObjectQueue final {
public:
    // IDA 0x610fc0/0x611050; owner +44 facet.
    [[nodiscard]] static std::uint32_t Add(
        const EffectObjectState& state, std::uint32_t id,
        std::uint32_t opaque, IEffectObjectQueueRuntime& runtime);
    [[nodiscard]] static std::uint32_t Process(
        const EffectObjectState& state, std::uint32_t id,
        std::uint32_t opaque, IEffectObjectQueueRuntime& runtime);
};

}  // namespace pal4
