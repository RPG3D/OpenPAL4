#pragma once

#include "pal4/effect_system.h"

#include <cstdint>

namespace pal4 {

// Game memory and queue dispatch stay outside the SDK state adapter.
class IEffectSystemRenderQueueRuntime {
public:
    virtual ~IEffectSystemRenderQueueRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    // 0x660120 sorts, invokes each render callback, then empties the queue.
    virtual void SortAtomicGeometries(std::uint32_t geometry_data) = 0;
};

class IEffectSystemRenderStateRuntime : public IEffectSystemRenderQueueRuntime {
public:
    ~IEffectSystemRenderStateRuntime() override = default;
    [[nodiscard]] virtual std::uint32_t GetRenderState(
        std::uint32_t state) = 0;
    virtual void SetRenderState(std::uint32_t state,
                                std::uint32_t value) = 0;
};

}  // namespace pal4
