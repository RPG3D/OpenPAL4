#pragma once

#include <cstdint>

namespace pal4 {

class IAnimationEventQueueRuntime {
public:
    virtual ~IAnimationEventQueueRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    virtual void DeleteAnimation(std::uint32_t animation) = 0;
    virtual void AdvanceAnimation(std::uint32_t animation, float delta) = 0;
    virtual void CallAnimationEvent(std::uint32_t animation,
                                    std::uint32_t argument) = 0;
    [[nodiscard]] virtual std::uint32_t AnimationFinished(
        std::uint32_t animation) = 0;
};

class AnimationEventQueue final {
public:
    // IDA 0x4c2d20. seed is the uninitialized stack byte copied twice.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, std::uint8_t seed,
        IAnimationEventQueueRuntime& runtime);
    // IDA 0x4c2f80 and 0x4c2d80.
    [[nodiscard]] static std::uint32_t Clear(
        std::uint32_t object, IAnimationEventQueueRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IAnimationEventQueueRuntime& runtime);
    // IDA 0x4c3090 and 0x4c30f0.
    [[nodiscard]] static std::uint32_t Add(
        std::uint32_t object, std::uint8_t owned,
        std::uint32_t animation, IAnimationEventQueueRuntime& runtime);
    [[nodiscard]] static std::uint32_t AddWithDelay(
        std::uint32_t object, std::uint8_t owned,
        std::uint32_t animation, IAnimationEventQueueRuntime& runtime);
    // IDA 0x4c3170. Returns the final EAX value from the loop.
    [[nodiscard]] static std::uint32_t Update(
        std::uint32_t object, float delta,
        IAnimationEventQueueRuntime& runtime);
    // IDA 0x4c3210 and 0x4c3240.
    [[nodiscard]] static std::uint32_t Dispatch(
        std::uint32_t object, std::uint32_t argument,
        IAnimationEventQueueRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyList(
        std::uint32_t object, IAnimationEventQueueRuntime& runtime);
    // IDA 0x4c32b0 and 0x4c3300.
    [[nodiscard]] static std::uint32_t EraseAt(
        std::uint32_t object, std::uint32_t cursor_address,
        std::uint32_t node, IAnimationEventQueueRuntime& runtime);
    [[nodiscard]] static std::uint32_t AdvanceCursor(
        std::uint32_t cursor_address, std::uint32_t output_address,
        IAnimationEventQueueRuntime& runtime);

private:
    [[nodiscard]] static std::uint32_t RemoveNode(
        std::uint32_t node, IAnimationEventQueueRuntime& runtime);
};

}  // namespace pal4
