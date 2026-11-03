#pragma once

#include "pal4/effect_system_object_f_queue.h"
#include "pal4/rtk_prt_std_atomic_object.h"

#include <cstdint>

namespace pal4 {

class IRtkPrtStdAtomicQueueRuntime {
public:
    virtual ~IRtkPrtStdAtomicQueueRuntime() = default;
    virtual void Dispatch(std::uint32_t system, std::uint32_t slot,
                          EffectSystemHandleWords handle,
                          std::uint32_t opaque) = 0;
    [[nodiscard]] virtual std::uint32_t Reset(
        EffectSystemHandleWords& handle) = 0;
};

class RtkPrtStdAtomicQueueRuntimeStub final
    : public IRtkPrtStdAtomicQueueRuntime {
public:
    void Dispatch(std::uint32_t, std::uint32_t,
                  EffectSystemHandleWords, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t Reset(
        EffectSystemHandleWords& handle) override {
        handle = {0xffffffffu, 0u};
        return 0u;
    }
};

class RtkPrtStdAtomicQueue final {
public:
    // IDA 0x614e90 / 0x614f20; owner +56 facet.
    [[nodiscard]] static std::uint32_t Add(
        const RtkPrtStdAtomicObjectState& state, std::uint32_t id,
        std::uint32_t opaque, IRtkPrtStdAtomicQueueRuntime& runtime);
    [[nodiscard]] static std::uint32_t Process(
        const RtkPrtStdAtomicObjectState& state, std::uint32_t id,
        std::uint32_t opaque, IRtkPrtStdAtomicQueueRuntime& runtime);
};

}  // namespace pal4
