#pragma once

#include "pal4/effect_system_object_f.h"

#include <cstdint>

namespace pal4 {

struct EffectSystemHandleWords final {
    std::uint32_t id = 0u;
    std::uint32_t system = 0u;
};

class IEffectSystemObjectFQueueRuntime {
public:
    virtual ~IEffectSystemObjectFQueueRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Dispatch(
        std::uint32_t system, std::uint32_t vtable_byte_offset,
        EffectSystemHandleWords handle, std::uint32_t opaque) = 0;
    virtual void Reset(EffectSystemHandleWords& handle) = 0;
};

class EffectSystemObjectFQueueRuntimeStub final
    : public IEffectSystemObjectFQueueRuntime {
public:
    [[nodiscard]] std::uint32_t Dispatch(
        std::uint32_t, std::uint32_t, EffectSystemHandleWords,
        std::uint32_t) override { return 0u; }
    void Reset(EffectSystemHandleWords& handle) override {
        handle = {0xffffffffu, 0u};
    }
};

class EffectSystemObjectFQueue final {
public:
    // IDA 0x6067f0, owner facet +52, EffectSystem slot +12.
    [[nodiscard]] static std::uint32_t Add(
        const EffectSystemObjectFState& state, std::uint32_t id,
        std::uint32_t opaque, IEffectSystemObjectFQueueRuntime& runtime);
    // IDA 0x606880, same handle copy but virtual slot +16.
    [[nodiscard]] static std::uint32_t Process(
        const EffectSystemObjectFState& state, std::uint32_t id,
        std::uint32_t opaque, IEffectSystemObjectFQueueRuntime& runtime);
};

}  // namespace pal4
