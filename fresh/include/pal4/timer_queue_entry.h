#pragma once

#include "pal4/string_buffer.h"

#include <cstddef>
#include <cstdint>

namespace pal4 {

struct TimerQueueEntryState {
    std::uint8_t text_flag{};
    StringBufferState name{};
    std::uint32_t initial_period_bits{};
    std::uint32_t current_period_bits{};
    bool owns_callback{};
    std::uint32_t callback{};
};

static_assert(sizeof(TimerQueueEntryState) == 0x20u);
static_assert(offsetof(TimerQueueEntryState, name) == 0x4u);
static_assert(offsetof(TimerQueueEntryState, callback) == 0x1cu);

class ITimerQueueEntryRuntime {
public:
    virtual ~ITimerQueueEntryRuntime() = default;
    virtual void EraseName(TimerQueueEntryState& entry,
                           std::uint32_t offset,
                           std::uint32_t count) = 0;
    virtual void SetTextFlag(TimerQueueEntryState& entry,
                             std::uint8_t flag) = 0;
    [[nodiscard]] virtual std::uint8_t ReadReferenceCount(
        std::uint32_t data) = 0;
    virtual void WriteReferenceCount(std::uint32_t data,
                                     std::uint8_t count) = 0;
    [[nodiscard]] virtual bool AllocateName(TimerQueueEntryState& entry,
                                            std::uint32_t length,
                                            std::uint8_t mode) = 0;
    virtual void CopyName(std::uint32_t destination,
                          std::uint32_t source,
                          std::uint32_t length) = 0;
    virtual void SetNameSize(TimerQueueEntryState& entry,
                             std::uint32_t length) = 0;
    virtual void ReleaseCallback(std::uint32_t callback,
                                 std::uint32_t deleting) = 0;
    [[nodiscard]] virtual std::uint32_t FreeName(
        std::uint32_t header) = 0;
};

class ITimerQueueEntryDeletingRuntime : public ITimerQueueEntryRuntime {
public:
    [[nodiscard]] virtual TimerQueueEntryState& EntryAt(
        std::uint32_t address) = 0;
    virtual void FreeObject(std::uint32_t address) = 0;
};

class TimerQueueEntry final {
public:
    [[nodiscard]] static TimerQueueEntryState& Construct(
        TimerQueueEntryState& entry, const TimerQueueEntryState& source,
        std::uint32_t period_bits, std::uint32_t callback,
        std::uint32_t empty_string,
        ITimerQueueEntryRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        TimerQueueEntryState& entry, ITimerQueueEntryRuntime& runtime);

    // IDA 0x4c1800.
    [[nodiscard]] static std::uint32_t DestroyAndMaybeFree(
        std::uint32_t address, std::uint8_t deleting,
        ITimerQueueEntryDeletingRuntime& runtime);
};

}  // namespace pal4
