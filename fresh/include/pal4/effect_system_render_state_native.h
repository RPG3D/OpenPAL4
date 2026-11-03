#pragma once
#include "pal4/effect_system_render_state.h"

namespace pal4 {
// Get reads desired SDK state; Set queues it. Flushing belongs to SDK draw calls.
// The game queue runtime is borrowed and must outlive this adapter.
class NativeEffectSystemRenderStateRuntime final : public IEffectSystemRenderStateRuntime {
public:
    explicit NativeEffectSystemRenderStateRuntime(IEffectSystemRenderQueueRuntime& queue)
        : queue_(queue) {}
    std::uint8_t ReadByte(std::uint32_t address) override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    std::uint32_t GetRenderState(std::uint32_t state) override;
    void SetRenderState(std::uint32_t state, std::uint32_t value) override;
    void SortAtomicGeometries(std::uint32_t geometry_data) override;
private:
    IEffectSystemRenderQueueRuntime& queue_;
};
}
