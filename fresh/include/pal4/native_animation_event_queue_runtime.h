#pragma once

#include "pal4/animation_event_queue.h"
#include "pal4/native_base_object_memory.h"

namespace pal4 {

class IAnimationEventQueueActivityRuntime {
public:
    virtual ~IAnimationEventQueueActivityRuntime() = default;
    virtual void DeleteAnimation(std::uint32_t animation) = 0;
    virtual void AdvanceAnimation(std::uint32_t animation, float delta) = 0;
    virtual void CallAnimationEvent(std::uint32_t animation,
                                    std::uint32_t argument) = 0;
    [[nodiscard]] virtual std::uint32_t AnimationFinished(
        std::uint32_t animation) = 0;
};

class NativeAnimationEventQueueRuntime final :
    public IAnimationEventQueueRuntime {
public:
    NativeAnimationEventQueueRuntime(
        NativeBaseObjectMemory& memory,
        IAnimationEventQueueActivityRuntime& activity)
        : memory_(memory), activity_(activity) {}

    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t size) override;
    [[nodiscard]] std::uint32_t Free(std::uint32_t address) override;
    void DeleteAnimation(std::uint32_t animation) override;
    void AdvanceAnimation(std::uint32_t animation, float delta) override;
    void CallAnimationEvent(std::uint32_t animation,
                            std::uint32_t argument) override;
    [[nodiscard]] std::uint32_t AnimationFinished(
        std::uint32_t animation) override;

private:
    NativeBaseObjectMemory& memory_;
    IAnimationEventQueueActivityRuntime& activity_;
};

}  // namespace pal4
