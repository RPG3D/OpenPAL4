#pragma once

#include "pal4/effect_system_object_f_queue.h"
#include "pal4/sequence_anim_object.h"

#include <cstdint>

namespace pal4 {

class ISequenceAnimQueueRuntime {
public:
    virtual ~ISequenceAnimQueueRuntime() = default;
    virtual void Dispatch(std::uint32_t system, std::uint32_t slot,
                          EffectSystemHandleWords handle,
                          std::uint32_t opaque) = 0;
    [[nodiscard]] virtual std::uint32_t Reset(
        EffectSystemHandleWords& handle) = 0;
};

class SequenceAnimQueueRuntimeStub final : public ISequenceAnimQueueRuntime {
public:
    void Dispatch(std::uint32_t, std::uint32_t,
                  EffectSystemHandleWords, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t Reset(
        EffectSystemHandleWords& handle) override {
        handle = {0xffffffffu, 0u};
        return 0u;
    }
};

class SequenceAnimQueue final {
public:
    // IDA 0x616950 / 0x6169e0; owner +48 facet.
    [[nodiscard]] static std::uint32_t Add(
        const SequenceAnimObjectState& state, std::uint32_t id,
        std::uint32_t opaque, ISequenceAnimQueueRuntime& runtime);
    [[nodiscard]] static std::uint32_t Process(
        const SequenceAnimObjectState& state, std::uint32_t id,
        std::uint32_t opaque, ISequenceAnimQueueRuntime& runtime);
};

}  // namespace pal4
