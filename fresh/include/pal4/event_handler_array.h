#pragma once

#include <cstdint>

namespace pal4 {

class IEventHandlerArrayRuntime {
public:
    virtual ~IEventHandlerArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeVirtual(
        std::uint32_t object, std::uint32_t slot_offset) = 0;
    [[nodiscard]] virtual std::uint8_t InvokePredicate(
        std::uint32_t object, std::uint32_t slot_offset,
        std::uint32_t argument) = 0;
};

class EventHandlerArray final {
public:
    // IDA 0x4aebb0; array +4/+8/+12 hold begin/end/capacity end.
    [[nodiscard]] static std::uint32_t Insert(
        std::uint32_t array, std::uint32_t handler,
        IEventHandlerArrayRuntime& runtime);
    // IDA 0x4aed70; removal truncates the array at the matching slot.
    [[nodiscard]] static std::uint32_t Remove(
        std::uint32_t array, std::uint32_t handler,
        IEventHandlerArrayRuntime& runtime);
    // IDA 0x4aeff0: last handler slot +4, then reverse scan slot +8.
    [[nodiscard]] static bool Contains(
        std::uint32_t array, std::uint32_t event,
        IEventHandlerArrayRuntime& runtime);
    // IDA 0x4af060.
    [[nodiscard]] static std::uint32_t GetLast(
        std::uint32_t array, IEventHandlerArrayRuntime& runtime);
    // IDA 0x4af080: invokes slot +20 and reloads end after each element.
    [[nodiscard]] static std::uint32_t ForEach(
        std::uint32_t array, IEventHandlerArrayRuntime& runtime);
};

}  // namespace pal4
