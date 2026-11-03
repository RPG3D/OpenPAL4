#pragma once

#include "pal4/timer_queue_entry.h"

#include <cstdint>

namespace pal4 {

class ITimerQueueTaskRuntime {
public:
    virtual ~ITimerQueueTaskRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void CopyName(TimerQueueEntryState& entry,
                          std::uint32_t source) = 0;
    virtual void AssignTimerData(std::uint32_t destination,
                                 const TimerQueueEntryState& entry) = 0;
    virtual void ReleaseCallback(std::uint32_t callback,
                                 std::uint32_t deleting) = 0;
    [[nodiscard]] virtual ITimerQueueEntryRuntime& EntryRuntime() = 0;
};

class TimerQueueTask final {
public:
    static constexpr std::uint32_t kCallbackVtable = 0x843b20u;
    static constexpr std::uint32_t kPuzzleCallbackVtable = 0x843ed4u;

    static void Create(std::uint32_t queue,
                       std::uint32_t name_source,
                       std::uint32_t period_bits,
                       std::uint32_t callback_owner,
                       std::uint32_t callback_function,
                       std::uint32_t callback_argument,
                       ITimerQueueTaskRuntime& runtime);
    // IDA 0x4a1b40: same queue operation with the puzzle callback vtable.
    static void SchedulePuzzle(std::uint32_t queue,
                               std::uint32_t name_source,
                               std::uint32_t period_bits,
                               std::uint32_t callback_owner,
                               std::uint32_t callback_function,
                               std::uint32_t callback_argument,
                               ITimerQueueTaskRuntime& runtime);
    // IDA 0x54f820 inserts a caller-owned callback into the same queue.
    static void InsertOwnedCallback(std::uint32_t queue,
                                    std::uint32_t name_source,
                                    std::uint32_t period_bits,
                                    std::uint32_t callback,
                                    ITimerQueueTaskRuntime& runtime);
};

class ITimerCallbackInvocationRuntime {
public:
    virtual ~ITimerCallbackInvocationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeThiscall(
        std::uint32_t function, std::uint32_t context) = 0;
};

class TimerCallbackInvocation final {
public:
    // IDA 0x4a1cb0; the target receives context in ECX.
    [[nodiscard]] static std::uint32_t InvokeWithOffset(
        std::uint32_t callback, ITimerCallbackInvocationRuntime& runtime);
};

}  // namespace pal4
