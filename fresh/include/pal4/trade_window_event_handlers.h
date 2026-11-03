#pragma once

#include "pal4/string_buffer.h"
#include "pal4/timer_queue_entry.h"

#include <cstddef>
#include <cstdint>

namespace pal4 {

struct TradeEventName {
    std::uint8_t text_flag{};
    std::uint8_t reserved[3]{};
    StringBufferState name{};
};

struct TradeEventData {
    TradeEventName text{};
    std::uint32_t delay_bits{};
    bool owns_callback{};
    std::uint8_t reserved[3]{};
    std::uint32_t callback{};
};

static_assert(sizeof(TradeEventName) == 0x10u);
static_assert(sizeof(TradeEventData) == 0x1cu);
static_assert(offsetof(TradeEventData, callback) == 0x18u);

class ITradeWindowEventRuntime {
public:
    virtual ~ITradeWindowEventRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void AssignName(TradeEventName& destination,
                            const TradeEventName& source) = 0;
    virtual void AssignTimerName(TimerQueueEntryState& destination,
                                 const TimerQueueEntryState& source) = 0;
    [[nodiscard]] virtual TradeEventData& EventDataAt(std::uint32_t address) = 0;
    [[nodiscard]] virtual TimerQueueEntryState& TimerDataAt(
        std::uint32_t address) = 0;
    virtual void ReleaseCallback(std::uint32_t callback,
                                 std::uint32_t deleting) = 0;
};

class TradeWindowEventHandlers final {
public:
    static constexpr std::uint32_t kCallbackVtable = 0x843d8cu;
    static constexpr std::uint32_t kHandlerOneVtable = 0x843d2cu;
    static constexpr std::uint32_t kHandlerTwoVtable = 0x843d80u;

    // IDA 0x48daa0, 0x48dc60.
    static void AddEventHandler(std::uint32_t queue,
                                const TradeEventName& name,
                                std::uint32_t delay_bits,
                                std::uint32_t frame, std::uint32_t handler,
                                std::uint32_t event, std::uint8_t flag,
                                ITradeWindowEventRuntime& runtime);
    static void AddTimerHandler(std::uint32_t queue,
                                const TradeEventName& name,
                                std::uint32_t delay_bits,
                                std::uint32_t frame, std::uint32_t handler,
                                std::uint32_t event, std::uint8_t flag,
                                ITradeWindowEventRuntime& runtime);

    // IDA 0x48dbe0, 0x48ddb0.
    [[nodiscard]] static std::uint32_t CleanupEventData(
        TradeEventData& data, ITradeWindowEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t CopyEventData(
        TradeEventData* destination, TradeEventData& source,
        ITradeWindowEventRuntime& runtime);

    // IDA 0x4c1020. Removes a linked event node and returns its successor.
    [[nodiscard]] static std::uint32_t RemoveEventNode(
        std::uint32_t queue, std::uint32_t output,
        std::uint32_t node, ITradeWindowEventRuntime& runtime);

    // IDA 0x48de20, 0x48de70.
    [[nodiscard]] static std::uint32_t AllocateEventHandlerOne(
        std::uint32_t source, ITradeWindowEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t AllocateEventHandlerTwo(
        std::uint32_t source, ITradeWindowEventRuntime& runtime);
};

}  // namespace pal4
