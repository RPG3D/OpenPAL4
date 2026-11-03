#pragma once

#include "pal4/effect_system_object_f_queue.h"
#include "pal4/sound3d_object.h"

#include <cstdint>

namespace pal4 {

class ISound3DQueueRuntime {
public:
    virtual ~ISound3DQueueRuntime() = default;
    virtual void Dispatch(std::uint32_t system, std::uint32_t slot,
                          EffectSystemHandleWords handle,
                          std::uint32_t opaque) = 0;
    [[nodiscard]] virtual std::uint32_t Reset(
        EffectSystemHandleWords& handle) = 0;
};

class Sound3DQueueRuntimeStub final : public ISound3DQueueRuntime {
public:
    void Dispatch(std::uint32_t, std::uint32_t,
                  EffectSystemHandleWords, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t Reset(
        EffectSystemHandleWords& handle) override {
        handle = {0xffffffffu, 0u};
        return 0u;
    }
};

class Sound3DQueue final {
public:
    // IDA 0x617c60 / 0x617cf0; owner +36 facet.
    [[nodiscard]] static std::uint32_t Add(
        const Sound3DObjectState& state, std::uint32_t id,
        std::uint32_t opaque, ISound3DQueueRuntime& runtime);
    [[nodiscard]] static std::uint32_t Process(
        const Sound3DObjectState& state, std::uint32_t id,
        std::uint32_t opaque, ISound3DQueueRuntime& runtime);
};

}  // namespace pal4
